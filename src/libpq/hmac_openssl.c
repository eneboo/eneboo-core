/*-------------------------------------------------------------------------
 *
 * hmac_openssl.c
 *	  HMAC functions for SCRAM authentication, using local SHA-256 code
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

#include <string.h>
#include <stdlib.h>

#include "scram_crypto.h"
#include "scram_sha256.h"

struct pg_hmac_ctx
{
	int			type;
	scram_sha256_ctx inner_ctx;
	scram_sha256_ctx outer_ctx;
	unsigned char keybuf[SCRAM_SHA256_BLOCK_LENGTH];
	int finalized;
};

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
	memset(ctx->keybuf, 0, sizeof(ctx->keybuf));
	ctx->finalized = 0;

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
	unsigned char hashed_key[SCRAM_SHA256_DIGEST_LENGTH];
	unsigned char ipad[SCRAM_SHA256_BLOCK_LENGTH];
	unsigned char opad[SCRAM_SHA256_BLOCK_LENGTH];
	size_t i;

	if (ctx == NULL)
		return -1;

	if (ctx->type != PG_SHA256)
		return -1;

	if (len > SCRAM_SHA256_BLOCK_LENGTH)
	{
		scram_sha256_ctx keyctx;

		scram_sha256_init(&keyctx);
		scram_sha256_update(&keyctx, key, len);
		scram_sha256_final(&keyctx, hashed_key);
		memset(ctx->keybuf, 0, sizeof(ctx->keybuf));
		memcpy(ctx->keybuf, hashed_key, sizeof(hashed_key));
	}
	else
	{
		memset(ctx->keybuf, 0, sizeof(ctx->keybuf));
		memcpy(ctx->keybuf, key, len);
	}

	for (i = 0; i < SCRAM_SHA256_BLOCK_LENGTH; i++)
	{
		ipad[i] = (unsigned char) (ctx->keybuf[i] ^ 0x36);
		opad[i] = (unsigned char) (ctx->keybuf[i] ^ 0x5c);
	}

	scram_sha256_init(&ctx->inner_ctx);
	scram_sha256_update(&ctx->inner_ctx, ipad, sizeof(ipad));
	scram_sha256_init(&ctx->outer_ctx);
	scram_sha256_update(&ctx->outer_ctx, opad, sizeof(opad));
	ctx->finalized = 0;

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

	if (ctx->type != PG_SHA256 || ctx->finalized)
		return -1;

	scram_sha256_update(&ctx->inner_ctx, data, len);
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
	unsigned char inner_digest[SCRAM_SHA256_DIGEST_LENGTH];

	if (ctx == NULL)
		return -1;

	if (ctx->type != PG_SHA256 || len < SCRAM_SHA256_DIGEST_LENGTH || ctx->finalized)
		return -1;

	scram_sha256_final(&ctx->inner_ctx, inner_digest);
	scram_sha256_update(&ctx->outer_ctx, inner_digest, sizeof(inner_digest));
	scram_sha256_final(&ctx->outer_ctx, dest);
	ctx->finalized = 1;

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

	free(ctx);
}

#endif /* LIBPQ_HMAC_OPENSSL_C */
