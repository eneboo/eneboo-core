/*-------------------------------------------------------------------------
 *
 * scram_crypto.h
 *	  Declarations for SCRAM crypto primitives used in libpq
 *
 * Ported from PG14 src/common/cryptohash.h, hmac.h to eneboo-core libpq
 * $PostgreSQL: pgsql/src/common/cryptohash.h, ported to eneboo-core libpq
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 *
 *-------------------------------------------------------------------------
 */
#ifndef SCRAM_CRYPTO_H
#define SCRAM_CRYPTO_H

#include <stddef.h>

/* Algorithm identifiers */
#define PG_MD5		1
#define PG_SHA224	2
#define PG_SHA256	3
#define PG_SHA384	4
#define PG_SHA512	5

/* SHA-256 digest length in bytes */
#define PG_SHA256_DIGEST_LENGTH		32
#define PG_SHA512_DIGEST_LENGTH		64
#define PG_MD5_DIGEST_LENGTH		16

/* Opaque hash context */
typedef struct pg_cryptohash_ctx pg_cryptohash_ctx;

extern pg_cryptohash_ctx *pg_cryptohash_create(int type);
extern int	pg_cryptohash_init(pg_cryptohash_ctx *ctx);
extern int	pg_cryptohash_update(pg_cryptohash_ctx *ctx, const unsigned char *data, size_t len);
extern int	pg_cryptohash_final(pg_cryptohash_ctx *ctx, unsigned char *dest, size_t len);
extern void pg_cryptohash_free(pg_cryptohash_ctx *ctx);

/* Opaque HMAC context */
typedef struct pg_hmac_ctx pg_hmac_ctx;

extern pg_hmac_ctx *pg_hmac_create(int type);
extern int	pg_hmac_init(pg_hmac_ctx *ctx, const unsigned char *key, size_t len);
extern int	pg_hmac_update(pg_hmac_ctx *ctx, const unsigned char *data, size_t len);
extern int	pg_hmac_final(pg_hmac_ctx *ctx, unsigned char *dest, size_t len);
extern void pg_hmac_free(pg_hmac_ctx *ctx);

/* base64 */
extern int	pg_b64_encode(const char *src, int len, char *dst, int dstlen);
extern int	pg_b64_decode(const char *src, int len, char *dst, int dstlen);
extern int	pg_b64_enc_len(int srclen);
extern int	pg_b64_dec_len(int srclen);

/* saslprep */
#define SASLPREP_SUCCESS	0
#define SASLPREP_ERROR		1
extern int	pg_saslprep(const char *input, char **output);

/* strong random */
extern int	pg_strong_random(void *buf, size_t len);

#endif /* SCRAM_CRYPTO_H */
