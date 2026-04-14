/*-------------------------------------------------------------------------
 *
 * cryptohash_openssl.c
 *	  Cryptographic hash functions for SCRAM, using OpenSSL EVP API
 *
 * Ported from PG14 src/common/cryptohash_openssl.c to eneboo-core libpq
 * $PostgreSQL: pgsql/src/common/cryptohash_openssl.c, ported to eneboo-core libpq
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 *
 *-------------------------------------------------------------------------
 */
#ifndef LIBPQ_CRYPTOHASH_OPENSSL_C
#define LIBPQ_CRYPTOHASH_OPENSSL_C

#include "postgres_fe.h"

#include <openssl/evp.h>
#include <string.h>
#include <stdlib.h>

#include "scram_crypto.h"

#ifndef HAVE_PG_CRYPTOHASH

/*
 * EVP_MD_CTX_new/free were introduced in OpenSSL 1.1.0.
 * For older versions, fall back to EVP_MD_CTX_create/destroy.
 */
#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define EVP_MD_CTX_new()	EVP_MD_CTX_create()
#define EVP_MD_CTX_free(c)	EVP_MD_CTX_destroy(c)
#endif

struct pg_cryptohash_ctx
{
	int			type;
	EVP_MD_CTX *evpctx;
};

/*
 * pg_cryptohash_create
 *
 * Allocate a hash context for the given algorithm type.
 */
pg_cryptohash_ctx *
pg_cryptohash_create(int type)
{
	pg_cryptohash_ctx *ctx;

	ctx = (pg_cryptohash_ctx *) malloc(sizeof(pg_cryptohash_ctx));
	if (ctx == NULL)
		return NULL;

	ctx->type = type;
	ctx->evpctx = EVP_MD_CTX_new();
	if (ctx->evpctx == NULL)
	{
		free(ctx);
		return NULL;
	}

	return ctx;
}

/*
 * pg_cryptohash_init
 *
 * Initialize a hash context.  Returns 0 on success, -1 on failure.
 */
int
pg_cryptohash_init(pg_cryptohash_ctx *ctx)
{
	const EVP_MD *md;

	if (ctx == NULL)
		return -1;

	switch (ctx->type)
	{
		case PG_MD5:
			md = EVP_md5();
			break;
		case PG_SHA224:
			md = EVP_sha224();
			break;
		case PG_SHA256:
			md = EVP_sha256();
			break;
		case PG_SHA384:
			md = EVP_sha384();
			break;
		case PG_SHA512:
			md = EVP_sha512();
			break;
		default:
			return -1;
	}

	if (EVP_DigestInit_ex(ctx->evpctx, md, NULL) <= 0)
		return -1;

	return 0;
}

/*
 * pg_cryptohash_update
 *
 * Update a hash context with new data.  Returns 0 on success, -1 on failure.
 */
int
pg_cryptohash_update(pg_cryptohash_ctx *ctx, const unsigned char *data, size_t len)
{
	if (ctx == NULL)
		return -1;

	if (EVP_DigestUpdate(ctx->evpctx, data, len) <= 0)
		return -1;

	return 0;
}

/*
 * pg_cryptohash_final
 *
 * Finalize a hash context and write the digest into dest.
 * Returns 0 on success, -1 on failure.
 */
int
pg_cryptohash_final(pg_cryptohash_ctx *ctx, unsigned char *dest, size_t len)
{
	unsigned int outlen;

	if (ctx == NULL)
		return -1;

	/*
	 * Verify output buffer is large enough.
	 */
	switch (ctx->type)
	{
		case PG_MD5:
			if (len < PG_MD5_DIGEST_LENGTH)
				return -1;
			break;
		case PG_SHA256:
			if (len < PG_SHA256_DIGEST_LENGTH)
				return -1;
			break;
		case PG_SHA512:
			if (len < PG_SHA512_DIGEST_LENGTH)
				return -1;
			break;
		default:
			break;
	}

	if (EVP_DigestFinal_ex(ctx->evpctx, dest, &outlen) <= 0)
		return -1;

	return 0;
}

/*
 * pg_cryptohash_free
 *
 * Free a hash context.
 */
void
pg_cryptohash_free(pg_cryptohash_ctx *ctx)
{
	if (ctx == NULL)
		return;

	EVP_MD_CTX_free(ctx->evpctx);
	free(ctx);
}

#endif /* HAVE_PG_CRYPTOHASH */

#endif /* LIBPQ_CRYPTOHASH_OPENSSL_C */
