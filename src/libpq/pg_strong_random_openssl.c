/*-------------------------------------------------------------------------
 *
 * pg_strong_random_openssl.c
 *	  Strong random bytes via OpenSSL RAND_bytes() for SCRAM nonce generation
 *
 * Ported from PG14 src/port/pg_strong_random.c to eneboo-core libpq
 * $PostgreSQL: pgsql/src/port/pg_strong_random.c, ported to eneboo-core libpq
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 *
 *-------------------------------------------------------------------------
 */
#ifndef LIBPQ_PG_STRONG_RANDOM_OPENSSL_C
#define LIBPQ_PG_STRONG_RANDOM_OPENSSL_C

#include <openssl/rand.h>
#include <stddef.h>

/*
 * pg_strong_random
 *
 * Fill 'buf' with 'len' bytes of cryptographically strong random data.
 * Returns 1 on success, 0 on failure (matches PG convention: true/false).
 */
int
pg_strong_random(void *buf, size_t len)
{
	if (RAND_bytes((unsigned char *) buf, (int) len) <= 0)
		return 0;
	return 1;
}

#endif /* LIBPQ_PG_STRONG_RANDOM_OPENSSL_C */
