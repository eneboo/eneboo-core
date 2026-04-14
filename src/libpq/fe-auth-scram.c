/*-------------------------------------------------------------------------
 *
 * fe-auth-scram.c
 *	   The front-end (client) SCRAM-SHA-256 authentication routines
 *
 * Ported from PG14 src/interfaces/libpq/fe-auth-scram.c to eneboo-core
 * $PostgreSQL: pgsql/src/interfaces/libpq/fe-auth-scram.c (PG14), ported to eneboo-core libpq
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/libpq/fe-auth-scram.c
 *
 *-------------------------------------------------------------------------
 */

#ifndef LIBPQ_FE_AUTH_SCRAM
#define LIBPQ_FE_AUTH_SCRAM

#define _POSIX_C_SOURCE 200809L

#include "postgres_fe.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef SCRAM_DEBUG
#define SCRAM_LOG(fmt, ...) fprintf(stderr, "[SCRAM] " fmt "\n", ##__VA_ARGS__)
#else
#define SCRAM_LOG(fmt, ...) ((void)0)
#endif

#include "libpq-fe.h"
#include "libpq-int.h"
#include "scram-common.h"
#include "scram_crypto.h"

/*
 * Note: saslprep declarations (pg_saslprep, SASLPREP_SUCCESS, SASLPREP_ERROR)
 * come from scram_crypto.h — saslprep_stub.c provides the implementation.
 * base64 functions (pg_b64_encode, pg_b64_decode, etc.) are likewise
 * declared in scram_crypto.h and implemented in base64_scram.c.
 */

/* ========== Client state machine ========== */

typedef enum
{
	FE_SCRAM_INIT,
	FE_SCRAM_NONCE_SENT,
	FE_SCRAM_PROOF_SENT,
	FE_SCRAM_FINISHED
} fe_scram_state_enum;

typedef struct
{
	fe_scram_state_enum state;

	/* Connection info (not owned) */
	PGconn	   *conn;

	/* Password, SASLprep-normalised (owned, must free) */
	char	   *password;

	/* SCRAM mechanism name in use */
	const char *sasl_mechanism;

	/*
	 * TODO: enable channel binding with HAVE_PGTLS_GET_PEER_CERTIFICATE_HASH
	 * when TLS peer certificate hash support is added to eneboo libpq.
	 * For now channel_binding_type is always "n" (no binding).
	 */
	char	   *channel_binding_type;	/* "n", "y", or "p=..." */
	char	   *channel_binding_data;	/* NULL unless channel binding active */
	int			channel_binding_len;

	/* Nonces */
	char	   *client_nonce;			/* random nonce we generated */
	char	   *server_nonce;			/* nonce echoed by server */
	char	   *combined_nonce;			/* client_nonce + server_nonce */

	/* Server-supplied parameters from server-first-message */
	char	   *server_first_message;
	char	   *salt;					/* raw (decoded) salt */
	int			saltlen;
	int			iterations;

	/* Computed keys */
	unsigned char		SaltedPassword[SCRAM_KEY_LEN];
	unsigned char		ClientKey[SCRAM_KEY_LEN];
	unsigned char		StoredKey[SCRAM_KEY_LEN];
	unsigned char		ServerKey[SCRAM_KEY_LEN];

	/* Messages used in auth-message construction */
	char	   *client_first_message_bare;
	char	   *client_final_message_without_proof;

	/* Server signature for verification */
	unsigned char		ServerSignature[SCRAM_KEY_LEN];
} fe_scram_state;

/* ---------- forward declarations of static helpers ---------- */

static bool read_server_first_message(fe_scram_state *state, char *input);
static bool read_server_final_message(fe_scram_state *state, char *input);
static char *build_client_first_message(fe_scram_state *state);
static char *build_client_final_message(fe_scram_state *state);
static bool verify_server_signature(fe_scram_state *state, bool *match);
static bool calculate_client_proof(fe_scram_state *state,
								   const char *auth_message,
								   unsigned char *result);
static char *sanitize_char(char c);
static char *sanitize_str(const char *s);
static char *scram_b64_encode(const char *input, int inputlen);
static char *scram_b64_decode(const char *input, int inputlen, int *resultlen);
static char *read_attr_value(char **input, char attr);
static bool verify_nonce(fe_scram_state *state, const char *client_nonce,
						 const char *server_nonce);

/* ========================================================================
 * Public API
 * ========================================================================
 */

/*
 * pg_fe_scram_init
 *
 * Allocate a SCRAM authentication state and prepare the first message
 * to send to the server.
 *
 * Returns an opaque pointer to the state, or NULL on failure (with an
 * error message in *errormessage).
 */
void *
pg_fe_scram_init(PGconn *conn,
				 const char *password,
				 const char *sasl_mechanism)
{
	fe_scram_state *state;
	char	   *prep_password;

	state = (fe_scram_state *) malloc(sizeof(fe_scram_state));
	if (!state)
		return NULL;
	memset(state, 0, sizeof(fe_scram_state));
	state->conn = conn;
	state->sasl_mechanism = sasl_mechanism;
	state->state = FE_SCRAM_INIT;

	/*
	 * Normalize the password via SASLprep.  Our stub simply copies it, which
	 * is correct for ASCII passwords (RFC 5802 §3).
	 */
	if (pg_saslprep(password, &prep_password) != SASLPREP_SUCCESS)
	{
		free(state);
		return NULL;
	}
	state->password = prep_password;

	/*
	 * TODO: enable channel binding with HAVE_PGTLS_GET_PEER_CERTIFICATE_HASH
	 * Determine channel binding type.  For now always "n" (no binding).
	 */
	state->channel_binding_type = strdup("n");
	if (!state->channel_binding_type)
	{
		free(state->password);
		free(state);
		return NULL;
	}
	state->channel_binding_data = NULL;
	state->channel_binding_len = 0;

	return state;
}

/*
 * pg_fe_scram_free
 *
 * Free a SCRAM state allocated by pg_fe_scram_init().
 */
void
pg_fe_scram_free(void *opaque)
{
	fe_scram_state *state = (fe_scram_state *) opaque;

	if (!state)
		return;

	if (state->password)
		free(state->password);
	if (state->channel_binding_type)
		free(state->channel_binding_type);
	if (state->channel_binding_data)
		free(state->channel_binding_data);
	if (state->client_nonce)
		free(state->client_nonce);
	if (state->server_nonce)
		free(state->server_nonce);
	if (state->combined_nonce)
		free(state->combined_nonce);
	if (state->server_first_message)
		free(state->server_first_message);
	if (state->salt)
		free(state->salt);
	if (state->client_first_message_bare)
		free(state->client_first_message_bare);
	if (state->client_final_message_without_proof)
		free(state->client_final_message_without_proof);

	free(state);
}

/*
 * pg_fe_scram_exchange
 *
 * Drive the SCRAM exchange.  Called repeatedly with successive server
 * messages until done == true.
 *
 * input        - server message (NULL for initial call)
 * inputlen     - length of input
 * output       - pointer filled with the client message to send (caller frees)
 * outputlen    - length of *output
 * done         - set true when exchange is complete
 * success      - set true when authentication succeeded
 * errormessage - set to a palloc'd error string on failure
 */
void
pg_fe_scram_exchange(void *opaque, char *input, int inputlen,
					 char **output, int *outputlen,
					 bool *done, bool *success, char **errormessage)
{
	fe_scram_state *state = (fe_scram_state *) opaque;
	char	   *msg;

	*done = false;
	*success = false;
	*output = NULL;
	*outputlen = 0;
	*errormessage = NULL;

	switch (state->state)
	{
		case FE_SCRAM_INIT:
			/*
			 * Build and return the initial client-first-message.
			 */
			SCRAM_LOG("state=INIT: building client-first-message");
			msg = build_client_first_message(state);
			if (msg == NULL)
			{
				SCRAM_LOG("state=INIT: build_client_first_message failed");
				*errormessage = strdup(libpq_gettext("out of memory"));
				*done = true;
				return;
			}
			SCRAM_LOG("state=INIT: client-first-message = '%s'", msg);
			*output = msg;
			*outputlen = (int) strlen(msg);
			state->state = FE_SCRAM_NONCE_SENT;
			return;

		case FE_SCRAM_NONCE_SENT:
			/*
			 * We have received the server-first-message.  Parse it,
			 * then build and return client-final-message.
			 */
			SCRAM_LOG("state=NONCE_SENT: input=%s inputlen=%d",
					  input ? input : "(null)", inputlen);
			if (input == NULL || inputlen == 0)
			{
				SCRAM_LOG("state=NONCE_SENT: no server-first-message received");
				*errormessage = strdup(libpq_gettext("SCRAM: no server-first-message"));
				*done = true;
				return;
			}
			/* Null-terminate the input for convenience */
			{
				char	   *buf = (char *) malloc(inputlen + 1);

				if (!buf)
				{
					*errormessage = strdup(libpq_gettext("out of memory"));
					*done = true;
					return;
				}
				memcpy(buf, input, inputlen);
				buf[inputlen] = '\0';

				if (!read_server_first_message(state, buf))
				{
					free(buf);
					*errormessage = strdup(libpq_gettext("SCRAM: invalid server-first-message"));
					*done = true;
					return;
				}
				free(buf);
			}

			msg = build_client_final_message(state);
			if (msg == NULL)
			{
				SCRAM_LOG("state=NONCE_SENT: build_client_final_message failed");
				*errormessage = strdup(libpq_gettext("out of memory"));
				*done = true;
				return;
			}
			SCRAM_LOG("state=NONCE_SENT: client-final-message built OK");
			*output = msg;
			*outputlen = (int) strlen(msg);
			state->state = FE_SCRAM_PROOF_SENT;
			return;

		case FE_SCRAM_PROOF_SENT:
			/*
			 * We have received the server-final-message.  Verify the
			 * server signature.
			 */
			SCRAM_LOG("state=PROOF_SENT: input=%s inputlen=%d",
					  input ? input : "(null)", inputlen);
			if (input == NULL || inputlen == 0)
			{
				*errormessage = strdup(libpq_gettext("SCRAM: no server-final-message"));
				*done = true;
				return;
			}
			{
				char	   *buf = (char *) malloc(inputlen + 1);

				if (!buf)
				{
					*errormessage = strdup(libpq_gettext("out of memory"));
					*done = true;
					return;
				}
				memcpy(buf, input, inputlen);
				buf[inputlen] = '\0';

				if (!read_server_final_message(state, buf))
				{
					free(buf);
					*errormessage = strdup(libpq_gettext("SCRAM: invalid server-final-message"));
					*done = true;
					return;
				}
				free(buf);
			}

			{
				bool		match;

				if (!verify_server_signature(state, &match))
				{
					*errormessage = strdup(libpq_gettext("SCRAM: could not verify server signature"));
					*done = true;
					return;
				}
				if (!match)
				{
					*errormessage = strdup(libpq_gettext("SCRAM: server signature does not match"));
					*done = true;
					return;
				}
			}

			SCRAM_LOG("state=PROOF_SENT: server signature verified OK");
			*done = true;
			*success = true;
			state->state = FE_SCRAM_FINISHED;
			return;

		case FE_SCRAM_FINISHED:
			/* Should not be called again after finishing */
			*errormessage = strdup(libpq_gettext("SCRAM: exchange already complete"));
			*done = true;
			return;

		default:
			*errormessage = strdup(libpq_gettext("SCRAM: invalid state"));
			*done = true;
			return;
	}
}

/* ========================================================================
 * Helpers for building client messages
 * ========================================================================
 */

/*
 * scram_b64_encode
 *
 * Base64-encode 'inputlen' bytes of 'input'.  Returns a newly allocated
 * null-terminated string, or NULL on error.
 */
static char *
scram_b64_encode(const char *input, int inputlen)
{
	int		need;
	int		dstlen;
	int		actual;
	char   *result;

	need = pg_b64_enc_len(inputlen);
	dstlen = need + 4;		/* +4 extra safety margin beyond formula */
	result = (char *) malloc(dstlen + 1);
	SCRAM_LOG("scram_b64_encode: inputlen=%d need=%d dstlen=%d malloc=%d",
			  inputlen, need, dstlen, dstlen + 1);
	if (!result)
	{
		SCRAM_LOG("scram_b64_encode: malloc failed");
		return NULL;
	}

	actual = pg_b64_encode(input, inputlen, result, dstlen);
	SCRAM_LOG("scram_b64_encode: pg_b64_encode returned %d", actual);
	if (actual < 0)
	{
		free(result);
		return NULL;
	}
	result[actual] = '\0';
	return result;
}

/*
 * scram_b64_decode
 *
 * Base64-decode 'inputlen' bytes of 'input'.  Returns a newly allocated
 * buffer of length *resultlen, or NULL on error.
 */
static char *
scram_b64_decode(const char *input, int inputlen, int *resultlen)
{
	int			decoded_len;
	char	   *result;

	decoded_len = pg_b64_dec_len(inputlen);
	result = (char *) malloc(decoded_len + 1);
	if (!result)
		return NULL;

	decoded_len = pg_b64_decode(input, inputlen, result, decoded_len + 1);
	if (decoded_len < 0)
	{
		free(result);
		return NULL;
	}
	result[decoded_len] = '\0';
	*resultlen = decoded_len;
	return result;
}

/*
 * sanitize_char
 *
 * Return a printable representation of a character for error messages.
 * Returns a pointer to a static buffer.
 */
static char *
sanitize_char(char c)
{
	static char buf[5];

	if (c >= 0x21 && c <= 0x7e)
		snprintf(buf, sizeof(buf), "'%c'", c);
	else
		snprintf(buf, sizeof(buf), "0x%02x", (unsigned char) c);
	return buf;
}

/*
 * sanitize_str
 *
 * Return a sanitized copy of a string for inclusion in error messages.
 * Replaces non-printable characters.  Returns a pointer to a static buffer.
 */
static char *
sanitize_str(const char *s)
{
	static char buf[256];
	int			i;

	for (i = 0; i < (int) sizeof(buf) - 1 && s[i] != '\0'; i++)
	{
		if (s[i] >= 0x21 && s[i] <= 0x7e)
			buf[i] = s[i];
		else
			buf[i] = '?';
	}
	buf[i] = '\0';
	return buf;
}

/*
 * read_attr_value
 *
 * Read the next attribute from a SCRAM message.  The message pointer
 * *input is advanced past the separator.  Returns the attribute value
 * portion (everything after 'attr='), or NULL if the expected attribute
 * character is not found.
 */
static char *
read_attr_value(char **input, char attr)
{
	char	   *begin = *input;
	char	   *end;

	if (*begin != attr)
		return NULL;
	begin++;
	if (*begin != '=')
		return NULL;
	begin++;

	end = begin;
	while (*end && *end != ',')
		end++;

	if (*end == ',')
	{
		*end = '\0';
		*input = end + 1;
	}
	else
	{
		*input = end;
	}

	return begin;
}

/*
 * verify_nonce
 *
 * Verify that the server's combined nonce begins with our client nonce.
 */
static bool
verify_nonce(fe_scram_state *state, const char *client_nonce,
			 const char *server_nonce)
{
	int			client_nonce_len = (int) strlen(client_nonce);
	int			server_nonce_len = (int) strlen(server_nonce);

	if (server_nonce_len <= client_nonce_len)
		return false;
	if (memcmp(server_nonce, client_nonce, client_nonce_len) != 0)
		return false;
	return true;
}

/*
 * build_client_first_message
 *
 * Construct and return the client-first-message, e.g.:
 *
 *   n,,n=<username>,r=<client_nonce>
 *
 * The caller must free the returned string.
 */
static char *
build_client_first_message(fe_scram_state *state)
{
	/*
	 * Generate a 18-byte random client nonce and encode it as base64 to get
	 * a ~24-character nonce string that contains only printable ASCII.
	 */
	static const int NONCE_RAW_BYTES = 18;
	unsigned char		raw[18];			/* must match NONCE_RAW_BYTES */
	char	   *nonce_b64;
	PGconn	   *conn = state->conn;
	const char *username;
	char	   *result;
	int			msglen;

	if (pg_strong_random(raw, NONCE_RAW_BYTES) == 0)
	{
		SCRAM_LOG("pg_strong_random failed");
		return NULL;
	}

	nonce_b64 = scram_b64_encode((const char *) raw, NONCE_RAW_BYTES);
	if (!nonce_b64)
		return NULL;
	state->client_nonce = nonce_b64;

	/*
	 * The username is sent in the client-first-message-bare.
	 * RFC 5802 requires that ',' and '=' in the username be encoded as
	 * =2C and =3D respectively.  For simplicity we rely on the server
	 * to tolerate the raw username — real saslprep would handle this.
	 */
	username = conn->pguser ? conn->pguser : "";

	/*
	 * Format:
	 *   gs2-header  = channel_binding_type "," [authzid] ","
	 *   client-first-message-bare = "n=" username "," "r=" nonce
	 *   client-first-message = gs2-header client-first-message-bare
	 *
	 * We always use "n,," (no channel binding, no authzid).
	 * TODO: support "p=tls-server-end-point,," with HAVE_PGTLS_GET_PEER_CERTIFICATE_HASH
	 */
	msglen = (int) strlen("n,,n=") + (int) strlen(username) +
		(int) strlen(",r=") + (int) strlen(state->client_nonce) + 1;

	result = (char *) malloc(msglen);
	if (!result)
		return NULL;

	snprintf(result, msglen, "n,,n=%s,r=%s", username, state->client_nonce);

	/*
	 * Save the client-first-message-bare portion for later use in the
	 * auth-message (starts after "n,,").
	 */
	state->client_first_message_bare = strdup(result + strlen("n,,"));
	if (!state->client_first_message_bare)
	{
		free(result);
		return NULL;
	}

	return result;
}

/*
 * build_client_final_message
 *
 * Construct and return the client-final-message, e.g.:
 *
 *   c=biws,r=<combined_nonce>,p=<ClientProof-b64>
 *
 * biws = base64("n,,") — the channel binding data header.
 * The caller must free the returned string.
 */
static char *
build_client_final_message(fe_scram_state *state)
{
	/*
	 * channel-binding = base64(cbind-input)
	 * cbind-input = gs2-header [ cbind-data ]
	 * For "n,," (no binding), cbind-input = "n,,"
	 *
	 * TODO: for channel binding "p=...", cbind-input includes the
	 * TLS certificate hash appended after the gs2-header.
	 * Enable this with HAVE_PGTLS_GET_PEER_CERTIFICATE_HASH.
	 */
	const char *gs2_header = "n,,";
	char	   *channel_binding_b64;
	char	   *client_final_without_proof;
	unsigned char		proof[SCRAM_KEY_LEN];
	char	   *proof_b64;
	char	   *auth_message;
	char	   *result;
	int			msglen;

	SCRAM_LOG("build_client_final_message: start, combined_nonce='%s'",
			  state->combined_nonce ? state->combined_nonce : "(null)");

	/* Heap sanity check BEFORE PBKDF2 */
	{
		void *pre256 = malloc(256);
		SCRAM_LOG("PRE-PBKDF2 heap check: malloc(256)=%p", pre256);
		if (pre256) free(pre256);
	}

	channel_binding_b64 = scram_b64_encode(gs2_header, (int) strlen(gs2_header));
	if (!channel_binding_b64)
	{
		SCRAM_LOG("build_client_final_message: b64_encode gs2_header failed");
		return NULL;
	}
	SCRAM_LOG("build_client_final_message: channel_binding_b64='%s'", channel_binding_b64);

	/*
	 * client-final-message-without-proof =
	 *   "c=" channel-binding "," "r=" combined-nonce
	 */
	msglen = (int) strlen("c=") + (int) strlen(channel_binding_b64) +
		(int) strlen(",r=") + (int) strlen(state->combined_nonce) + 1;
	client_final_without_proof = (char *) malloc(msglen);
	if (!client_final_without_proof)
	{
		SCRAM_LOG("build_client_final_message: malloc client_final_without_proof failed");
		free(channel_binding_b64);
		return NULL;
	}
	snprintf(client_final_without_proof, msglen,
			 "c=%s,r=%s", channel_binding_b64, state->combined_nonce);
	free(channel_binding_b64);
	SCRAM_LOG("build_client_final_message: client_final_without_proof='%s'",
			  client_final_without_proof);

	state->client_final_message_without_proof = client_final_without_proof;

	/*
	 * AuthMessage = client-first-message-bare + "," +
	 *               server-first-message + "," +
	 *               client-final-message-without-proof
	 */
	msglen = (int) strlen(state->client_first_message_bare) + 1 +
		(int) strlen(state->server_first_message) + 1 +
		(int) strlen(client_final_without_proof) + 1;
	auth_message = (char *) malloc(msglen);
	if (!auth_message)
	{
		SCRAM_LOG("build_client_final_message: malloc auth_message failed");
		return NULL;
	}
	snprintf(auth_message, msglen, "%s,%s,%s",
			 state->client_first_message_bare,
			 state->server_first_message,
			 client_final_without_proof);
	SCRAM_LOG("build_client_final_message: auth_message='%s'", auth_message);

	/* Heap sanity check BEFORE PBKDF2 (after auth_message alloc) */
	{
		void *pre = malloc(256);
		SCRAM_LOG("PRE-PBKDF2 heap check 2: malloc(256)=%p", pre);
		if (pre) free(pre);
	}

	/* Compute client proof */
	SCRAM_LOG("build_client_final_message: calling calculate_client_proof");
	if (!calculate_client_proof(state, auth_message, proof))
	{
		SCRAM_LOG("build_client_final_message: calculate_client_proof failed");
		free(auth_message);
		return NULL;
	}
	free(auth_message);
	SCRAM_LOG("build_client_final_message: calculate_client_proof OK");

	/* Heap sanity check AFTER PBKDF2 */
	{
		void *post = malloc(256);
		SCRAM_LOG("POST-PBKDF2 heap check: malloc(256)=%p", post);
		if (post) free(post);
	}

	proof_b64 = scram_b64_encode((const char *) proof, SCRAM_KEY_LEN);
	if (!proof_b64)
	{
		SCRAM_LOG("build_client_final_message: b64_encode proof failed");
		return NULL;
	}

	/*
	 * client-final-message =
	 *   client-final-message-without-proof "," "p=" proof-b64
	 */
	{
		int n1 = (int) strlen(client_final_without_proof);
		int n3 = (int) strlen(proof_b64);
		void *t1 = malloc(1);
		void *t8 = malloc(8);
		msglen = n1 + 3 + n3 + 1;
		SCRAM_LOG("build_final: n1=%d n3=%d msglen=%d test_malloc1=%p test_malloc8=%p",
				  n1, n3, msglen, t1, t8);
		if (t1) free(t1);
		if (t8) free(t8);
	}
	result = (char *) malloc(msglen);
	SCRAM_LOG("build_final: malloc(%d)=%p", msglen, (void *) result);
	if (!result)
	{
		SCRAM_LOG("build_final: FAILED - heap corrupt");
		free(proof_b64);
		return NULL;
	}
	snprintf(result, msglen, "%s,p=%s", client_final_without_proof, proof_b64);
	SCRAM_LOG("build_final: msg='%.30s...'", result);
	free(proof_b64);

	return result;
}

/* ========================================================================
 * Helpers for parsing server messages
 * ========================================================================
 */

/*
 * read_server_first_message
 *
 * Parse the server-first-message and extract the server nonce, salt and
 * iteration count.  Returns true on success, false on parse error.
 *
 * server-first-message =
 *   [reserved-mext ","] "r=" nonce "," "s=" salt-b64 "," "i=" iter-count
 *
 * We keep a copy in state->server_first_message for use in auth-message.
 */
static bool
read_server_first_message(fe_scram_state *state, char *input)
{
	char	   *p = input;
	char	   *attr;
	char	   *server_nonce_start;
	char	   *salt_b64;
	char	   *iter_str;
	int		 iterations;

	/*
	 * RFC 5802 §7: server-first-message may start with reserved extensions
	 * of the form "m=<value>,".  Skip them.
	 */
	if (*p == 'm')
	{
		/* Skip the extension attribute */
		while (*p && *p != ',')
			p++;
		if (*p == ',')
			p++;
	}

	/* Save a copy of the full server-first-message before we mutate it */
	state->server_first_message = strdup(input);
	if (!state->server_first_message)
		return false;

	/*
	 * Parse "r=<combined-nonce>"
	 */
	attr = read_attr_value(&p, 'r');
	if (!attr)
		return false;
	server_nonce_start = attr;

	/*
	 * Verify that the combined nonce starts with our client nonce.
	 */
	if (!verify_nonce(state, state->client_nonce, server_nonce_start))
		return false;

	state->server_nonce = strdup(server_nonce_start);
	if (!state->server_nonce)
		return false;

	/* combined_nonce = the full "r=" value (client+server parts) */
	state->combined_nonce = strdup(server_nonce_start);
	if (!state->combined_nonce)
		return false;

	/*
	 * Parse "s=<salt-b64>"
	 */
	attr = read_attr_value(&p, 's');
	if (!attr)
		return false;
	salt_b64 = attr;

	state->salt = scram_b64_decode(salt_b64, (int) strlen(salt_b64),
								   &state->saltlen);
	if (!state->salt)
		return false;

	/*
	 * Parse "i=<iterations>"
	 */
	attr = read_attr_value(&p, 'i');
	if (!attr)
		return false;
	iter_str = attr;

	iterations = atoi(iter_str);
	if (iterations <= 0)
		return false;
	state->iterations = iterations;

	return true;
}

/*
 * read_server_final_message
 *
 * Parse the server-final-message.  Returns true on success.
 *
 * server-final-message = ("e=" server-error) / ("v=" verifier-b64)
 *
 * On success the server signature bytes are stored in state->ServerSignature.
 */
static bool
read_server_final_message(fe_scram_state *state, char *input)
{
	char	   *p = input;

	/* Check for server error */
	if (*p == 'e')
	{
		char	   *attr = read_attr_value(&p, 'e');

		(void) attr;				/* error text ignored; caller reports it */
		return false;
	}

	/* Expect "v=<verifier-b64>" */
	if (*p != 'v')
		return false;

	{
		char	   *attr = read_attr_value(&p, 'v');
		char	   *sig_raw;
		int			sig_len;

		if (!attr)
			return false;

		sig_raw = scram_b64_decode(attr, (int) strlen(attr), &sig_len);
		if (!sig_raw)
			return false;
		if (sig_len != SCRAM_KEY_LEN)
		{
			free(sig_raw);
			return false;
		}
		memcpy(state->ServerSignature, sig_raw, SCRAM_KEY_LEN);
		free(sig_raw);
	}

	return true;
}

/* ========================================================================
 * Cryptographic helpers
 * ========================================================================
 */

/*
 * calculate_client_proof
 *
 * Compute ClientProof = ClientKey XOR HMAC(StoredKey, AuthMessage)
 * and store in result (SCRAM_KEY_LEN bytes).
 *
 * Also precomputes and stores SaltedPassword, ClientKey, StoredKey, ServerKey
 * in the state for later use by verify_server_signature().
 */
static bool
calculate_client_proof(fe_scram_state *state,
					   const char *auth_message,
					   unsigned char *result)
{
	unsigned char		ClientSignature[SCRAM_KEY_LEN];
	int			i;

	SCRAM_LOG("calculate_client_proof: saltlen=%d iterations=%d",
			  state->saltlen, state->iterations);

	/*
	 * SaltedPassword := Hi(password, salt, i)
	 */
	if (scram_SaltedPassword(state->password,
							 state->salt, state->saltlen,
							 state->iterations,
							 state->SaltedPassword) < 0)
	{
		SCRAM_LOG("calculate_client_proof: scram_SaltedPassword failed");
		return false;
	}
	SCRAM_LOG("calculate_client_proof: SaltedPassword OK");

	/*
	 * ClientKey := HMAC(SaltedPassword, "Client Key")
	 */
	if (scram_ClientKey(state->SaltedPassword, state->ClientKey) < 0)
	{
		SCRAM_LOG("calculate_client_proof: scram_ClientKey failed");
		return false;
	}
	SCRAM_LOG("calculate_client_proof: ClientKey OK");

	/*
	 * StoredKey := H(ClientKey)
	 */
	if (scram_H(state->ClientKey, SCRAM_KEY_LEN, state->StoredKey) < 0)
	{
		SCRAM_LOG("calculate_client_proof: scram_H failed");
		return false;
	}
	SCRAM_LOG("calculate_client_proof: StoredKey OK");

	/*
	 * ServerKey := HMAC(SaltedPassword, "Server Key")
	 */
	if (scram_ServerKey(state->SaltedPassword, state->ServerKey) < 0)
	{
		SCRAM_LOG("calculate_client_proof: scram_ServerKey failed");
		return false;
	}
	SCRAM_LOG("calculate_client_proof: ServerKey OK");

	/*
	 * ClientSignature := HMAC(StoredKey, AuthMessage)
	 * We re-use scram_H's sister function via a local HMAC call.
	 * scram-common.c does not expose scram_HMAC directly, so we
	 * build the HMAC through the pg_hmac_ctx API.
	 */
	{
		pg_hmac_ctx *hctx = pg_hmac_create(PG_SHA256);

		if (!hctx)
		{
			SCRAM_LOG("calculate_client_proof: pg_hmac_create failed");
			return false;
		}
		if (pg_hmac_init(hctx, state->StoredKey, SCRAM_KEY_LEN) < 0)
		{
			SCRAM_LOG("calculate_client_proof: pg_hmac_init failed");
			pg_hmac_free(hctx);
			return false;
		}
		if (pg_hmac_update(hctx, (const unsigned char *) auth_message,
						   strlen(auth_message)) < 0)
		{
			SCRAM_LOG("calculate_client_proof: pg_hmac_update failed");
			pg_hmac_free(hctx);
			return false;
		}
		if (pg_hmac_final(hctx, ClientSignature, SCRAM_KEY_LEN) < 0)
		{
			SCRAM_LOG("calculate_client_proof: pg_hmac_final failed");
			pg_hmac_free(hctx);
			return false;
		}
		pg_hmac_free(hctx);
		SCRAM_LOG("calculate_client_proof: ClientSignature HMAC OK");
	}

	/*
	 * ClientProof := ClientKey XOR ClientSignature
	 */
	for (i = 0; i < SCRAM_KEY_LEN; i++)
		result[i] = state->ClientKey[i] ^ ClientSignature[i];

	return true;
}

/*
 * verify_server_signature
 *
 * Verify the server's signature against the expected value computed as:
 *   ServerSignature = HMAC(ServerKey, AuthMessage)
 *
 * AuthMessage is re-constructed identically to what was used in
 * calculate_client_proof().
 *
 * Sets *match = true if the signatures agree.
 */
static bool
verify_server_signature(fe_scram_state *state, bool *match)
{
	unsigned char		expected_ServerSignature[SCRAM_KEY_LEN];
	char	   *auth_message;
	int			msglen;
	pg_hmac_ctx *hctx;

	*match = false;

	/* Re-build AuthMessage */
	msglen = (int) strlen(state->client_first_message_bare) + 1 +
		(int) strlen(state->server_first_message) + 1 +
		(int) strlen(state->client_final_message_without_proof) + 1;
	auth_message = (char *) malloc(msglen);
	if (!auth_message)
		return false;
	snprintf(auth_message, msglen, "%s,%s,%s",
			 state->client_first_message_bare,
			 state->server_first_message,
			 state->client_final_message_without_proof);

	/* HMAC(ServerKey, AuthMessage) */
	hctx = pg_hmac_create(PG_SHA256);
	if (!hctx)
	{
		free(auth_message);
		return false;
	}
	if (pg_hmac_init(hctx, state->ServerKey, SCRAM_KEY_LEN) < 0 ||
		pg_hmac_update(hctx, (const unsigned char *) auth_message,
					   strlen(auth_message)) < 0 ||
		pg_hmac_final(hctx, expected_ServerSignature, SCRAM_KEY_LEN) < 0)
	{
		pg_hmac_free(hctx);
		free(auth_message);
		return false;
	}
	pg_hmac_free(hctx);
	free(auth_message);

	*match = (memcmp(state->ServerSignature, expected_ServerSignature,
					 SCRAM_KEY_LEN) == 0);
	return true;
}

#endif /* LIBPQ_FE_AUTH_SCRAM */
