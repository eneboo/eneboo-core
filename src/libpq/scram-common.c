/*-------------------------------------------------------------------------
 *
 * scram-common.c
 *	  Shared frontend/backend code for SCRAM authentication
 *
 * Ported from PG14 src/common/scram-common.c to eneboo-core libpq
 * $PostgreSQL: pgsql/src/common/scram-common.c, ported to eneboo-core libpq
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 *
 *-------------------------------------------------------------------------
 */
#ifndef LIBPQ_SCRAM_COMMON_C
#define LIBPQ_SCRAM_COMMON_C

#include "postgres_fe.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "scram_crypto.h"

/* SCRAM-SHA-256 output length */
#define SCRAM_KEY_LEN				PG_SHA256_DIGEST_LENGTH

/*
 * scram_H
 *
 * Apply the SHA-256 hash function H() to the given string.
 * Output is written into 'buf' which must be SCRAM_KEY_LEN bytes.
 * Returns 0 on success, -1 on error.
 */
int
scram_H(const unsigned char *input, int len, unsigned char *dest)
{
	pg_cryptohash_ctx *ctx;

	ctx = pg_cryptohash_create(PG_SHA256);
	if (ctx == NULL)
		return -1;

	if (pg_cryptohash_init(ctx) < 0 ||
		pg_cryptohash_update(ctx, input, len) < 0 ||
		pg_cryptohash_final(ctx, dest, SCRAM_KEY_LEN) < 0)
	{
		pg_cryptohash_free(ctx);
		return -1;
	}

	pg_cryptohash_free(ctx);
	return 0;
}

/*
 * scram_HMAC
 *
 * Apply HMAC-SHA-256 with given key to str.  Output is written into 'dest'
 * which must be SCRAM_KEY_LEN bytes.  Returns 0 on success, -1 on error.
 */
static int
scram_HMAC(const unsigned char *key, int keylen, const unsigned char *str, int slen, unsigned char *dest)
{
	pg_hmac_ctx *ctx;

	ctx = pg_hmac_create(PG_SHA256);
	if (ctx == NULL)
		return -1;

	if (pg_hmac_init(ctx, key, keylen) < 0 ||
		pg_hmac_update(ctx, str, slen) < 0 ||
		pg_hmac_final(ctx, dest, SCRAM_KEY_LEN) < 0)
	{
		pg_hmac_free(ctx);
		return -1;
	}

	pg_hmac_free(ctx);
	return 0;
}

/*
 * scram_SaltedPassword
 *
 * Compute SaltedPassword(password, salt, iterations) as defined in RFC 5802:
 *
 *   SaltedPassword := Hi(Normalize(password), salt, i)
 *
 * Uses a single HMAC_CTX reused across all PBKDF2 iterations to avoid
 * thousands of malloc/free cycles that can corrupt the heap.
 *
 * 'output' must be SCRAM_KEY_LEN bytes.
 * Returns 0 on success, -1 on error.
 */
int
scram_SaltedPassword(const char *password,
					 const char *salt, int saltlen,
					 int iterations,
					 unsigned char *output)
{
	unsigned char		Ui[SCRAM_KEY_LEN];
	unsigned char		Ui_prev[SCRAM_KEY_LEN];
	unsigned char		saltbuf[1024 + 4];
	int					plen = (int) strlen(password);
	int					i,
						j;

	if (saltlen > 1024)
		return -1;

	/* Build salt || INT(1) */
	memcpy(saltbuf, salt, saltlen);
	saltbuf[saltlen + 0] = 0;
	saltbuf[saltlen + 1] = 0;
	saltbuf[saltlen + 2] = 0;
	saltbuf[saltlen + 3] = 1;

	/* U1 = HMAC(password, salt || INT(1)) */
	if (scram_HMAC((const unsigned char *) password, plen,
				   saltbuf, saltlen + 4,
				   Ui_prev) < 0)
		return -1;

	memcpy(output, Ui_prev, SCRAM_KEY_LEN);

	/* U2 ... Ui: HMAC(password, U_{i-1}), XOR into output */
	for (i = 2; i <= iterations; i++)
	{
		if (scram_HMAC((const unsigned char *) password, plen,
					   Ui_prev, SCRAM_KEY_LEN,
					   Ui) < 0)
			return -1;

		for (j = 0; j < SCRAM_KEY_LEN; j++)
			output[j] ^= Ui[j];

		memcpy(Ui_prev, Ui, SCRAM_KEY_LEN);
	}
	return 0;
}

/*
 * scram_ClientKey
 *
 * ClientKey := HMAC(SaltedPassword, "Client Key")
 * 'output' must be SCRAM_KEY_LEN bytes.
 * Returns 0 on success, -1 on error.
 */
int
scram_ClientKey(const unsigned char *salted_password, unsigned char *output)
{
	static const unsigned char client_key_str[] = "Client Key";

	return scram_HMAC(salted_password, SCRAM_KEY_LEN,
					  client_key_str, sizeof(client_key_str) - 1,
					  output);
}

/*
 * scram_ServerKey
 *
 * ServerKey := HMAC(SaltedPassword, "Server Key")
 * 'output' must be SCRAM_KEY_LEN bytes.
 * Returns 0 on success, -1 on error.
 */
int
scram_ServerKey(const unsigned char *salted_password, unsigned char *output)
{
	static const unsigned char server_key_str[] = "Server Key";

	return scram_HMAC(salted_password, SCRAM_KEY_LEN,
					  server_key_str, sizeof(server_key_str) - 1,
					  output);
}

/*
 * scram_build_secret
 *
 * Build a SCRAM secret (verifier) string for storage.  Format:
 *
 *   SCRAM-SHA-256$<iterations>:<salt-b64>$<StoredKey-b64>:<ServerKey-b64>
 *
 * 'buf' must be at least SCRAM_MAX_SECRET_LENGTH bytes.
 * Returns 0 on success, -1 on error.
 *
 * NOTE: This function is only used by the server side (pg_backend).
 * In the libpq frontend path it is compiled but never called.
 */
#ifndef FRONTEND
int
scram_build_secret(const char *salt, int saltlen, int iterations,
				   const char *password, char *buf, int buflen)
{
	unsigned char		salted_password[SCRAM_KEY_LEN];
	unsigned char		stored_key[SCRAM_KEY_LEN];
	unsigned char		server_key[SCRAM_KEY_LEN];
	unsigned char		client_key[SCRAM_KEY_LEN];
	unsigned char		H_client_key[SCRAM_KEY_LEN];
	char		salt_b64[512];
	char		stored_key_b64[512];
	char		server_key_b64[512];
	int			salt_b64_len;
	int			stored_key_b64_len;
	int			server_key_b64_len;
	char	   *prep_password = NULL;
	int			ret;

	/* Normalize the password (saslprep; we have a stub) */
	if (pg_saslprep(password, &prep_password) != SASLPREP_SUCCESS)
		return -1;

	/* Compute SaltedPassword */
	ret = scram_SaltedPassword(prep_password, salt, saltlen, iterations,
							   salted_password);
	free(prep_password);
	if (ret < 0)
		return -1;

	/* ClientKey, H(ClientKey) => StoredKey */
	if (scram_ClientKey(salted_password, client_key) < 0)
		return -1;
	if (scram_H(client_key, SCRAM_KEY_LEN, H_client_key) < 0)
		return -1;
	memcpy(stored_key, H_client_key, SCRAM_KEY_LEN);

	/* ServerKey */
	if (scram_ServerKey(salted_password, server_key) < 0)
		return -1;

	/* base64-encode salt, StoredKey, ServerKey */
	salt_b64_len = pg_b64_encode(salt, saltlen,
								 salt_b64, sizeof(salt_b64));
	if (salt_b64_len < 0)
		return -1;
	salt_b64[salt_b64_len] = '\0';

	stored_key_b64_len = pg_b64_encode((char *) stored_key, SCRAM_KEY_LEN,
									   stored_key_b64, sizeof(stored_key_b64));
	if (stored_key_b64_len < 0)
		return -1;
	stored_key_b64[stored_key_b64_len] = '\0';

	server_key_b64_len = pg_b64_encode((char *) server_key, SCRAM_KEY_LEN,
									   server_key_b64, sizeof(server_key_b64));
	if (server_key_b64_len < 0)
		return -1;
	server_key_b64[server_key_b64_len] = '\0';

	ret = snprintf(buf, buflen,
				   "SCRAM-SHA-256$%d:%s$%s:%s",
				   iterations,
				   salt_b64,
				   stored_key_b64,
				   server_key_b64);

	if (ret < 0 || ret >= buflen)
		return -1;

	return 0;
}
#endif /* !FRONTEND */

#endif /* LIBPQ_SCRAM_COMMON_C */
