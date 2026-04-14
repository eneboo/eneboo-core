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

#include <openssl/hmac.h>
#include <openssl/err.h>

#include "scram_crypto.h"

/* SCRAM-SHA-256 output length */
#define SCRAM_KEY_LEN				PG_SHA256_DIGEST_LENGTH

#ifdef SCRAM_DEBUG
#define SCRAM_COMMON_LOG(fmt, ...) fprintf(stderr, "[SCRAM-COMMON] " fmt "\n", ##__VA_ARGS__)
#else
#define SCRAM_COMMON_LOG(fmt, ...) ((void)0)
#endif

static void
scram_log_openssl_error(const char *where)
{
	unsigned long errcode;
	char errbuf[256];

	errcode = ERR_get_error();
	if (errcode == 0)
	{
		SCRAM_COMMON_LOG("%s: OpenSSL returned failure without ERR_get_error()", where);
		return;
	}

	ERR_error_string_n(errcode, errbuf, sizeof(errbuf));
	SCRAM_COMMON_LOG("%s: OpenSSL error=%s", where, errbuf);
}

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
	unsigned int		outlen;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	HMAC_CTX		   *hctx;
#else
	HMAC_CTX			hctx_buf;
	HMAC_CTX		   *hctx = &hctx_buf;
#endif

	if (saltlen > 1024)
		return -1;

	SCRAM_COMMON_LOG("scram_SaltedPassword: saltlen=%d iterations=%d plen=%d",
					 saltlen, iterations, plen);

	/* Build salt || INT(1) */
	memcpy(saltbuf, salt, saltlen);
	saltbuf[saltlen + 0] = 0;
	saltbuf[saltlen + 1] = 0;
	saltbuf[saltlen + 2] = 0;
	saltbuf[saltlen + 3] = 1;

	/* Allocate / initialise a single HMAC context */
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	hctx = HMAC_CTX_new();
	if (!hctx)
	{
		SCRAM_COMMON_LOG("scram_SaltedPassword: HMAC_CTX_new failed");
		scram_log_openssl_error("HMAC_CTX_new");
		return -1;
	}
#else
	HMAC_CTX_init(hctx);
#endif

	/* U1 = HMAC(password, salt || INT(1)) */
	if (HMAC_Init_ex(hctx, password, plen, EVP_sha256(), NULL) <= 0)
	{
		SCRAM_COMMON_LOG("scram_SaltedPassword: HMAC_Init_ex failed in U1");
		scram_log_openssl_error("HMAC_Init_ex U1");
		goto fail;
	}
	if (HMAC_Update(hctx, saltbuf, (size_t)(saltlen + 4)) <= 0)
	{
		SCRAM_COMMON_LOG("scram_SaltedPassword: HMAC_Update failed in U1");
		scram_log_openssl_error("HMAC_Update U1");
		goto fail;
	}
	if (HMAC_Final(hctx, Ui_prev, &outlen) <= 0)
	{
		SCRAM_COMMON_LOG("scram_SaltedPassword: HMAC_Final failed in U1");
		scram_log_openssl_error("HMAC_Final U1");
		goto fail;
	}
	if (outlen != SCRAM_KEY_LEN)
	{
		SCRAM_COMMON_LOG("scram_SaltedPassword: unexpected U1 outlen=%u expected=%d",
						 outlen, SCRAM_KEY_LEN);
		goto fail;
	}

	memcpy(output, Ui_prev, SCRAM_KEY_LEN);

	/* U2 ... Ui: HMAC(password, U_{i-1}), XOR into output */
	for (i = 2; i <= iterations; i++)
	{
		/* Reinitialise with same key: pass NULL for md to reuse algorithm */
		if (HMAC_Init_ex(hctx, password, plen, EVP_sha256(), NULL) <= 0)
		{
			SCRAM_COMMON_LOG("scram_SaltedPassword: HMAC_Init_ex failed at iteration=%d", i);
			scram_log_openssl_error("HMAC_Init_ex Ui");
			goto fail;
		}
		if (HMAC_Update(hctx, Ui_prev, SCRAM_KEY_LEN) <= 0)
		{
			SCRAM_COMMON_LOG("scram_SaltedPassword: HMAC_Update failed at iteration=%d", i);
			scram_log_openssl_error("HMAC_Update Ui");
			goto fail;
		}
		if (HMAC_Final(hctx, Ui, &outlen) <= 0)
		{
			SCRAM_COMMON_LOG("scram_SaltedPassword: HMAC_Final failed at iteration=%d", i);
			scram_log_openssl_error("HMAC_Final Ui");
			goto fail;
		}
		if (outlen != SCRAM_KEY_LEN)
		{
			SCRAM_COMMON_LOG("scram_SaltedPassword: unexpected outlen=%u at iteration=%d expected=%d",
							 outlen, i, SCRAM_KEY_LEN);
			goto fail;
		}

		for (j = 0; j < SCRAM_KEY_LEN; j++)
			output[j] ^= Ui[j];

		memcpy(Ui_prev, Ui, SCRAM_KEY_LEN);
	}

	SCRAM_COMMON_LOG("scram_SaltedPassword: completed successfully");

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	HMAC_CTX_free(hctx);
#else
	HMAC_CTX_cleanup(hctx);
#endif
	return 0;

fail:
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	HMAC_CTX_free(hctx);
#else
	HMAC_CTX_cleanup(hctx);
#endif
	return -1;
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
