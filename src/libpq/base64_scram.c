/*-------------------------------------------------------------------------
 *
 * base64_scram.c
 *	  base64 encoding/decoding functions for SCRAM authentication
 *
 * Ported from PG14 src/common/base64.c to eneboo-core libpq
 * $PostgreSQL: pgsql/src/common/base64.c, ported to eneboo-core libpq
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 *
 *-------------------------------------------------------------------------
 */
#ifndef LIBPQ_BASE64_SCRAM_C
#define LIBPQ_BASE64_SCRAM_C

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifndef HAVE_PG_B64_ENCODE

static const char _base64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const int8_t b64lookup[128] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
};

/*
 * pg_b64_encode
 *
 * Encode src into base64 and write into dst.  Returns the length of the
 * encoded string, or -1 on error.
 */
int
pg_b64_encode(const char *src, int len, char *dst, int dstlen)
{
	char	   *p;
	const char *s,
			   *end = src + len;
	int			pos = 2;
	uint32_t	buf = 0;

	fprintf(stderr, "[B64-ENTRY] pg_b64_encode: len=%d dstlen=%d\n", len, dstlen);

	s = src;
	p = dst;

	while (s < end)
	{
		buf |= (unsigned char) *s << (pos << 3);
		pos--;
		s++;

		/* write it out */
		if (pos < 0)
		{
			*p++ = _base64[(buf >> 18) & 0x3f];
			*p++ = _base64[(buf >> 12) & 0x3f];
			*p++ = _base64[(buf >> 6) & 0x3f];
			*p++ = _base64[buf & 0x3f];

			pos = 2;
			buf = 0;
		}
	}
	if (pos != 2)
	{
		*p++ = _base64[(buf >> 18) & 0x3f];
		*p++ = _base64[(buf >> 12) & 0x3f];
		*p++ = (pos == 0) ? _base64[(buf >> 6) & 0x3f] : '=';
		*p++ = '=';
	}

	return p - dst;
}

/*
 * pg_b64_decode
 *
 * Decode base64 string into dst.  Returns the length of the decoded string
 * on success, or -1 on error.
 */
int
pg_b64_decode(const char *src, int len, char *dst, int dstlen)
{
	const char *srcend = src + len,
			   *s = src;
	char	   *p = dst;
	char		c;
	int			b = 0;
	uint32_t	buf = 0;
	int			pos = 0,
				end = 0;

	while (s < srcend)
	{
		c = *s++;
		if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
			continue;

		if (c == '=')
		{
			/* Should not be more than 2 padding characters. */
			if (end++ > 1)
				return -1;
		}
		else
		{
			if (end != 0)
				return -1;
			b = ((unsigned char) c > 127) ? -1 : b64lookup[(unsigned char) c];
			if (b < 0)
				return -1;		/* invalid character */
		}

		/* add 6 bits to buffer */
		buf = (buf << 6) + (end ? 0 : b);
		pos++;

		if (pos == 4)
		{
			if ((p - dst + 3 - end) > dstlen)
				return -1;
			if (end == 0)
			{
				*p++ = (buf >> 16) & 0xff;
				*p++ = (buf >> 8) & 0xff;
				*p++ = buf & 0xff;
			}
			else if (end == 1)
			{
				*p++ = (buf >> 16) & 0xff;
				*p++ = (buf >> 8) & 0xff;
			}
			else
			{
				*p++ = (buf >> 16) & 0xff;
			}
			buf = 0;
			pos = 0;
		}
	}

	if (pos != 0)
		return -1;

	return p - dst;
}

/*
 * pg_b64_enc_len
 *
 * Returns the needed buffer size for encoding srclen bytes of data.
 */
int
pg_b64_enc_len(int srclen)
{
	/* ceil(srclen/3)*4, guaranteed >= actual output */
	return ((srclen + 2) / 3) * 4 + 4;
}

/*
 * pg_b64_dec_len
 *
 * Returns the needed buffer size for decoding srclen bytes of data.
 */
int
pg_b64_dec_len(int srclen)
{
	return (srclen * 3) / 4 + 3;
}

#endif /* HAVE_PG_B64_ENCODE */

#endif /* LIBPQ_BASE64_SCRAM_C */
