#ifndef LIBPQ_SCRAM_SHA256_H
#define LIBPQ_SCRAM_SHA256_H

#include <stddef.h>

#define SCRAM_SHA256_BLOCK_LENGTH 64
#define SCRAM_SHA256_DIGEST_LENGTH 32

typedef struct
{
	unsigned int	state[8];
	unsigned long long bitcount;
	unsigned char	buffer[SCRAM_SHA256_BLOCK_LENGTH];
} scram_sha256_ctx;

extern void scram_sha256_init(scram_sha256_ctx *ctx);
extern void scram_sha256_update(scram_sha256_ctx *ctx, const unsigned char *data, size_t len);
extern void scram_sha256_final(scram_sha256_ctx *ctx, unsigned char *digest);

#endif /* LIBPQ_SCRAM_SHA256_H */
