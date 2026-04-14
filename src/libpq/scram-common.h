/*-------------------------------------------------------------------------
 *
 * scram-common.h
 *	  Public declarations for SCRAM-SHA-256 authentication primitives
 *
 * Ported from PG14 src/common/scram-common.h to eneboo-core libpq
 * $PostgreSQL: pgsql/src/common/scram-common.h, ported to eneboo-core libpq
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 *
 *-------------------------------------------------------------------------
 */
#ifndef SCRAM_COMMON_H
#define SCRAM_COMMON_H

#include "scram_crypto.h"

/* SCRAM-SHA-256 specific constants */
#define SCRAM_SHA256_NAME				"SCRAM-SHA-256"
#define SCRAM_SHA256_PLUS_NAME			"SCRAM-SHA-256-PLUS"

#define SCRAM_KEY_LEN					PG_SHA256_DIGEST_LENGTH

/* Default iteration count (matches PG14 default) */
#define SCRAM_DEFAULT_ITERATIONS		4096

/* Default salt length in bytes */
#define SCRAM_DEFAULT_SALT_LEN			16

/* Maximum length of a verifier string (generous upper bound) */
#define SCRAM_MAX_SECRET_LENGTH			1024

/*
 * Public function declarations
 */
extern int	scram_SaltedPassword(const char *password,
								 const char *salt, int saltlen,
								 int iterations,
								 unsigned char *output);
extern int	scram_ClientKey(const unsigned char *salted_password, unsigned char *output);
extern int	scram_ServerKey(const unsigned char *salted_password, unsigned char *output);
extern int	scram_H(const unsigned char *input, int len, unsigned char *dest);

#ifndef FRONTEND
extern int	scram_build_secret(const char *salt, int saltlen, int iterations,
							   const char *password, char *buf, int buflen);
#endif

#endif /* SCRAM_COMMON_H */
