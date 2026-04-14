/*-------------------------------------------------------------------------
 *
 * hmac_openssl.c
 *	  HMAC functions for SCRAM authentication, using OpenSSL HMAC API
 *
 * Ported from PG14 src/common/hmac_openssl.c to eneboo-core libpq
 * $PostgreSQL: pgsql/src/common/hmac_openssl.c, ported to eneboo-core libpq
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 *
 *-------------------------------------------------------------------------
 */
#ifndef LIBPQ_HMAC_OPENSSL_C
#define LIBPQ_HMAC_OPENSSL_C

#include "postgres_fe.h"

#include <openssl/hmac.h>
#include <string.h>
#include <stdlib.h>

#include "scram_crypto.h"

/*
 * HMAC_CTX_new/free were introduced in OpenSSL 1.1.0.
 * For older versions, fall back to stack allocation with HMAC_CTX_init/cleanup.
 */
#if OPENSSL_VERSION_NUMBER < 0x10100000L

struct pg_hmac_ctx
{
	int			type;
	HMAC_CTX	evpctx;		/* stack-allocated for OpenSSL < 1.1 */
};

#define PG_HMAC_CTX_NEW(ctx)	HMAC_CTX_init(&(ctx)->evpctx)
#define PG_HMAC_CTX_FREE(ctx)	HMAC_CTX_cleanup(&(ctx)->evpctx)
#define PG_HMAC_EVPCTX(ctx)		(&(ctx)->evpctx)

#else /* OpenSSL >= 1.1.0 */

struct pg_hmac_ctx
{
	int			type;
	HMAC_CTX   *evpctx;		/* heap-allocated for OpenSSL >= 1.1 */
};

#define PG_HMAC_CTX_NEW(ctx)	((ctx)->evpctx = HMAC_CTX_new())
#define PG_HMAC_CTX_FREE(ctx)	HMAC_CTX_free((ctx)->evpctx)
#define PG_HMAC_EVPCTX(ctx)		((ctx)->evpctx)

#endif /* OPENSSL_VERSION_NUMBER */

/*
 * pg_hmac_create
 *
 * Allocate an HMAC context for the given algorithm type.
 */
pg_hmac_ctx *
pg_hmac_create(int type)
{
	pg_hmac_ctx *ctx;

	ctx = (pg_hmac_ctx *) malloc(sizeof(pg_hmac_ctx));
	if (ctx == NULL)
		return NULL;

	ctx->type = type;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	ctx->evpctx = HMAC_CTX_new();
	if (ctx->evpctx == NULL)
	{
		free(ctx);
		return NULL;
	}
#else
	HMAC_CTX_init(&ctx->evpctx);
#endif

	return ctx;
}

/*
 * pg_hmac_init
 *
 * Initialize an HMAC context with the given key.  Returns 0 on success,
 * -1 on failure.
 */
int
pg_hmac_init(pg_hmac_ctx *ctx, const unsigned char *key, size_t len)
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

	if (HMAC_Init_ex(PG_HMAC_EVPCTX(ctx), key, (int) len, md, NULL) <= 0)
		return -1;

	return 0;
}

/*
 * pg_hmac_update
 *
 * Update an HMAC context with new data.  Returns 0 on success, -1 on failure.
 */
int
pg_hmac_update(pg_hmac_ctx *ctx, const unsigned char *data, size_t len)
{
	if (ctx == NULL)
		return -1;

	if (HMAC_Update(PG_HMAC_EVPCTX(ctx), data, len) <= 0)
		return -1;

	return 0;
}

/*
 * pg_hmac_final
 *
 * Finalize an HMAC context and write the result into dest.
 * Returns 0 on success, -1 on failure.
 */
int
pg_hmac_final(pg_hmac_ctx *ctx, unsigned char *dest, size_t len)
{
	unsigned int outlen;

	if (ctx == NULL)
		return -1;

	if (HMAC_Final(PG_HMAC_EVPCTX(ctx), dest, &outlen) <= 0)
		return -1;

	if ((size_t) outlen > len)
		return -1;

	return 0;
}

/*
 * pg_hmac_free
 *
 * Free an HMAC context.
 */
void
pg_hmac_free(pg_hmac_ctx *ctx)
{
	if (ctx == NULL)
		return;

	PG_HMAC_CTX_FREE(ctx);
	free(ctx);
}

#endif /* LIBPQ_HMAC_OPENSSL_C */
