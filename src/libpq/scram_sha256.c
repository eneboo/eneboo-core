/*
 * Minimal SHA-256 implementation for SCRAM support in libpq.
 */

#ifndef LIBPQ_SCRAM_SHA256_C
#define LIBPQ_SCRAM_SHA256_C

#include "postgres_fe.h"

#include <string.h>

#include "scram_sha256.h"

#define SHR(x, n) ((x) >> (n))
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SIGMA0(x) (ROTR((x), 2) ^ ROTR((x), 13) ^ ROTR((x), 22))
#define SIGMA1(x) (ROTR((x), 6) ^ ROTR((x), 11) ^ ROTR((x), 25))
#define sigma0(x) (ROTR((x), 7) ^ ROTR((x), 18) ^ SHR((x), 3))
#define sigma1(x) (ROTR((x), 17) ^ ROTR((x), 19) ^ SHR((x), 10))

static const unsigned int k256[64] = {
	0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
	0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
	0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
	0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
	0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
	0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
	0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
	0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
	0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
	0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
	0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
	0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
	0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
	0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
	0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
	0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

static void
scram_sha256_transform(scram_sha256_ctx *ctx, const unsigned char *data)
{
	unsigned int a, b, c, d, e, f, g, h, t1, t2, w[64];
	int i;

	for (i = 0; i < 16; i++)
	{
		w[i] = ((unsigned int) data[i * 4] << 24) |
			   ((unsigned int) data[i * 4 + 1] << 16) |
			   ((unsigned int) data[i * 4 + 2] << 8) |
			   ((unsigned int) data[i * 4 + 3]);
	}

	for (i = 16; i < 64; i++)
		w[i] = sigma1(w[i - 2]) + w[i - 7] + sigma0(w[i - 15]) + w[i - 16];

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for (i = 0; i < 64; i++)
	{
		t1 = h + SIGMA1(e) + CH(e, f, g) + k256[i] + w[i];
		t2 = SIGMA0(a) + MAJ(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}

void
scram_sha256_init(scram_sha256_ctx *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->state[0] = 0x6a09e667U;
	ctx->state[1] = 0xbb67ae85U;
	ctx->state[2] = 0x3c6ef372U;
	ctx->state[3] = 0xa54ff53aU;
	ctx->state[4] = 0x510e527fU;
	ctx->state[5] = 0x9b05688cU;
	ctx->state[6] = 0x1f83d9abU;
	ctx->state[7] = 0x5be0cd19U;
}

void
scram_sha256_update(scram_sha256_ctx *ctx, const unsigned char *data, size_t len)
{
	size_t used = (size_t) ((ctx->bitcount >> 3) & 0x3f);
	size_t free_bytes = SCRAM_SHA256_BLOCK_LENGTH - used;

	ctx->bitcount += ((unsigned long long) len) << 3;

	if (len >= free_bytes)
	{
		if (used)
		{
			memcpy(ctx->buffer + used, data, free_bytes);
			scram_sha256_transform(ctx, ctx->buffer);
			data += free_bytes;
			len -= free_bytes;
			used = 0;
		}

		while (len >= SCRAM_SHA256_BLOCK_LENGTH)
		{
			scram_sha256_transform(ctx, data);
			data += SCRAM_SHA256_BLOCK_LENGTH;
			len -= SCRAM_SHA256_BLOCK_LENGTH;
		}
	}

	if (len > 0)
		memcpy(ctx->buffer + used, data, len);
}

void
scram_sha256_final(scram_sha256_ctx *ctx, unsigned char *digest)
{
	static const unsigned char pad[64] = {0x80};
	unsigned char lenbuf[8];
	size_t used = (size_t) ((ctx->bitcount >> 3) & 0x3f);
	size_t padlen;
	unsigned long long bitcount = ctx->bitcount;
	int i;

	for (i = 0; i < 8; i++)
		lenbuf[7 - i] = (unsigned char) (bitcount >> (i * 8));

	padlen = (used < 56) ? (56 - used) : (120 - used);
	scram_sha256_update(ctx, pad, padlen);
	scram_sha256_update(ctx, lenbuf, sizeof(lenbuf));

	for (i = 0; i < 8; i++)
	{
		digest[i * 4] = (unsigned char) (ctx->state[i] >> 24);
		digest[i * 4 + 1] = (unsigned char) (ctx->state[i] >> 16);
		digest[i * 4 + 2] = (unsigned char) (ctx->state[i] >> 8);
		digest[i * 4 + 3] = (unsigned char) ctx->state[i];
	}

	memset(ctx, 0, sizeof(*ctx));
}

#endif /* LIBPQ_SCRAM_SHA256_C */
