/*-------------------------------------------------------------------------
 *
 * pg_strong_random_openssl.c
 *	  Strong random bytes without OpenSSL, for SCRAM nonce generation
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

#include <stddef.h>

#ifdef WIN32
#include <windows.h>
#include <wincrypt.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

/*
 * pg_strong_random
 *
 * Fill 'buf' with 'len' bytes of cryptographically strong random data.
 * Returns 1 on success, 0 on failure (matches PG convention: true/false).
 */
int
pg_strong_random(void *buf, size_t len)
{
#ifdef WIN32
	HCRYPTPROV prov = 0;
	BOOL ok;

	ok = CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL,
							 CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
	if (!ok)
		return 0;

	ok = CryptGenRandom(prov, (DWORD) len, (BYTE *) buf);
	CryptReleaseContext(prov, 0);
	return ok ? 1 : 0;
#else
	int fd;
	unsigned char *p = (unsigned char *) buf;
	size_t done = 0;

	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		return 0;

	while (done < len)
	{
		ssize_t nread = read(fd, p + done, len - done);

		if (nread < 0)
		{
			if (errno == EINTR)
				continue;
			close(fd);
			return 0;
		}
		if (nread == 0)
		{
			close(fd);
			return 0;
		}
		done += (size_t) nread;
	}

	close(fd);
	return 1;
#endif
}

#endif /* LIBPQ_PG_STRONG_RANDOM_OPENSSL_C */
