/*-------------------------------------------------------------------------
 *
 * cryptohash_openssl.c
 *	  Cryptographic hash functions for SCRAM, using local SHA-256 code
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

#include <string.h>
#include <stdlib.h>

#include "scram_crypto.h"
#include "scram_sha256.h"

struct pg_cryptohash_ctx
{
	int			type;
	scram_sha256_ctx sha256ctx;
	int finalized;
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
	ctx->finalized = 0;

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
	if (ctx == NULL)
		return -1;

	switch (ctx->type)
	{
		case PG_SHA256:
			scram_sha256_init(&ctx->sha256ctx);
			ctx->finalized = 0;
			break;
		default:
			return -1;
	}

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

	if (ctx->finalized)
		return -1;

	if (ctx->type != PG_SHA256)
		return -1;

	scram_sha256_update(&ctx->sha256ctx, data, len);

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
	if (ctx == NULL)
		return -1;

	if (ctx->type != PG_SHA256 || len < PG_SHA256_DIGEST_LENGTH || ctx->finalized)
		return -1;

	scram_sha256_final(&ctx->sha256ctx, dest);
	ctx->finalized = 1;
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

	free(ctx);
}

#endif /* LIBPQ_CRYPTOHASH_OPENSSL_C */
