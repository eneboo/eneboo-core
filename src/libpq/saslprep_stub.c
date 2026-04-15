/*-------------------------------------------------------------------------
 *
 * saslprep_stub.c
 *	  Minimal stub for pg_saslprep() — passes input through unchanged.
 *
 * Eneboo does not have a Unicode SASLprep implementation.  RFC 5802
 * allows using the password as-is for ASCII passwords, which covers
 * the vast majority of real-world deployments.
 *
 * $PostgreSQL: pgsql/src/common/saslprep.c, stub for eneboo-core libpq
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 *
 *-------------------------------------------------------------------------
 */
#ifndef LIBPQ_SASLPREP_STUB_C
#define LIBPQ_SASLPREP_STUB_C

#include <string.h>
#include <stdlib.h>

#include "scram_crypto.h"

/*
 * pg_saslprep
 *
 * Stub: copy input to a newly allocated string.  The caller must free it.
 * Returns SASLPREP_SUCCESS on success, SASLPREP_ERROR on allocation failure.
 */
int
pg_saslprep(const char *input, char **output)
{
	*output = (char *) malloc(strlen(input) + 1);
	if (*output == NULL)
		return SASLPREP_ERROR;
	strcpy(*output, input);
	return SASLPREP_SUCCESS;
}

#endif /* LIBPQ_SASLPREP_STUB_C */
