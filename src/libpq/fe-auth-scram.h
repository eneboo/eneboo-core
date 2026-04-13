/*-------------------------------------------------------------------------
 *
 * fe-auth-scram.h
 *	  Declarations for SCRAM-SHA-256 authentication
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 *
 *-------------------------------------------------------------------------
 */

#ifndef LIBPQ_FE_AUTH_SCRAM_H
#define LIBPQ_FE_AUTH_SCRAM_H

#include "libpq-int.h"

/* Initialize SCRAM authentication state */
extern void *pg_fe_scram_init(PGconn *conn,
							  const char *password,
							  const char *sasl_mechanism);

/* Drive SCRAM exchange with server */
extern void pg_fe_scram_exchange(void *opaque, char *input, int inputlen,
								 char **output, int *outputlen,
								 bool *done, bool *success, char **errormessage);

/* Free SCRAM authentication state */
extern void pg_fe_scram_free(void *opaque);

#endif   /* LIBPQ_FE_AUTH_SCRAM_H */
