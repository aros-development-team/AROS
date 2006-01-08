/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Version CLI command
    Lang: english
*/

/******************************************************************************

    NAME

        Version [<library|device|file>] [<version #>] [<revision #>] [FILE] [FULL] [RES] 

    SYNOPSIS

        NAME/M,MD5SUM/S,VERSION/N,REVISION/N,FILE/S,FULL/S,RES/S

    LOCATION

        Workbench:C

    FUNCTION

	Prints or checks the version and revision information of a file, library or device.
	  
    INPUTS

	NAME      -- name of file, library or device to check. If not given it
	             prints version and revision of Kickstart.
	MD5SUM    -- #FIXME what is that?
	VERSION   -- checks for version and returns error code 5 (warn) if the
	             version of the file is lower.
	REVISION  -- checks for revision and returns error code 5 (warn) if the
	             revision of the file is lower.
	FILE      -- reads from file and ignores currently loaded libraries and devices
	FULL      -- prints additional information
	RES       -- gets version of resident commands
	
    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <aros/arosbase.h>
#include <aros/config.h>
#include <aros/inquire.h>
#include <proto/aros.h>

#define ENABLE_RT 1
#include <aros/rt.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <proto/exec.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <dos/datetime.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

/*===[md5.h]==============================================================*/

/* Data structure for MD5 (Message-Digest) computation */
typedef struct {
	ULONG buf[4];         /* scratch buffer */
	ULONG i[2];           /* number of _bits_ handled mod 2^64 */
	unsigned char in[64]; /* input buffer */
} MD5_CTX;

void MD5Init(MD5_CTX *mdContext);
void MD5Update(MD5_CTX *mdContext, unsigned char *inBuf, unsigned int inLen);
void MD5Final(unsigned char digest[16], MD5_CTX *mdContext);

/*==[md5.c]===============================================================*/

/*
 * Used in 'version' as is, just removed the #include "ambient.h"
 *
 * Harry Sintonen <sintonen@iki.fi>
 */

/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * (c) 2001-2003 by David Gerber <zapek@meanmachine.ch>
 * All Rights Reserved
 *
 * $Id$
 */

//#include "ambient.h"
//#include <exec/types.h>

/*
 ***********************************************************************
 ** md5.c -- the source code for MD5 routines                         **
 ** RSA Data Security, Inc. MD5 Message-Digest Algorithm              **
 ** Created: 2/17/90 RLR                                              **
 ** Revised: 1/91 SRD,AJ,BSK,JT Reference C Version                   **
 ***********************************************************************
 */

/*
 * Edited 7 May 93 by CP to change the interface to match that
 * of the MD5 routines in RSAREF.  Due to this alteration, this
 * code is "derived from the RSA Data Security, Inc. MD5 Message-
 * Digest Algorithm".  (See below.)
 */

/*
 ***********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved.  **
 **                                                                   **
 ** License to copy and use this software is granted provided that    **
 ** it is identified as the "RSA Data Security, Inc. MD5 Message-     **
 ** Digest Algorithm" in all material mentioning or referencing this  **
 ** software or this function.                                        **
 **                                                                   **
 ** License is also granted to make and use derivative works          **
 ** provided that such works are identified as "derived from the RSA  **
 ** Data Security, Inc. MD5 Message-Digest Algorithm" in all          **
 ** material mentioning or referencing the derived work.              **
 **                                                                   **
 ** RSA Data Security, Inc. makes no representations concerning       **
 ** either the merchantability of this software or the suitability    **
 ** of this software for any particular purpose.  It is provided "as  **
 ** is" without express or implied warranty of any kind.              **
 **                                                                   **
 ** These notices must be retained in any copies of any part of this  **
 ** documentation and/or software.                                    **
 ***********************************************************************
 */

//#include "md5.h"

/*
 ***********************************************************************
 **  Message-digest routines:                                         **
 **  To form the message digest for a message M                       **
 **    (1) Initialize a context buffer mdContext using MD5Init        **
 **    (2) Call MD5Update on mdContext and M                          **
 **    (3) Call MD5Final on mdContext                                 **
 **  The message digest is now in the bugffer passed to MD5Final      **
 ***********************************************************************
 */

static unsigned char PADDING[64] = {
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* F, G, H and I are basic MD5 functions */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4 */
/* Rotation is separate from addition to prevent recomputation */
#define FF(a, b, c, d, x, s, ac) \
	{(a) += F ((b), (c), (d)) + (x) + (ULONG)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}
#define GG(a, b, c, d, x, s, ac) \
	{(a) += G ((b), (c), (d)) + (x) + (ULONG)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}
#define HH(a, b, c, d, x, s, ac) \
	{(a) += H ((b), (c), (d)) + (x) + (ULONG)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}
#define II(a, b, c, d, x, s, ac) \
	{(a) += I ((b), (c), (d)) + (x) + (ULONG)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}

static void Transform(register ULONG *buf,register ULONG *in);

/* The routine MD5Init initializes the message-digest context
   mdContext. All fields are set to zero.
 */
void MD5Init ( MD5_CTX *mdContext)
{
	mdContext->i[0] = mdContext->i[1] = (ULONG)0;

	/* Load magic initialization constants.
	 */
	mdContext->buf[0] = (ULONG)0x67452301L;
	mdContext->buf[1] = (ULONG)0xefcdab89L;
	mdContext->buf[2] = (ULONG)0x98badcfeL;
	mdContext->buf[3] = (ULONG)0x10325476L;
}

/* The routine MD5Update updates the message-digest context to
   account for the presence of each of the characters inBuf[0..inLen-1]
   in the message whose digest is being computed.
 */
void MD5Update (MD5_CTX *mdContext, unsigned char *inBuf,
					   unsigned int inLen)
{
	register int i, ii;
	int mdi;
	ULONG in[16];

	/* compute number of bytes mod 64 */
	mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

	/* update number of bits */
	if ((mdContext->i[0] + ((ULONG)inLen << 3)) < mdContext->i[0])
		mdContext->i[1]++;
	mdContext->i[0] += ((ULONG)inLen << 3);
	mdContext->i[1] += ((ULONG)inLen >> 29);

	while (inLen--)
	{
		/* add new character to buffer, increment mdi */
		mdContext->in[mdi++] = *inBuf++;

		/* transform if necessary */
		if (mdi == 0x40)
		{
			for (i = 0, ii = 0; i < 16; i++, ii += 4)
				in[i] = (((ULONG)mdContext->in[ii+3]) << 24) |
				        (((ULONG)mdContext->in[ii+2]) << 16) |
				        (((ULONG)mdContext->in[ii+1]) << 8) |
				        ((ULONG)mdContext->in[ii]);
			Transform (mdContext->buf, in);
			mdi = 0;
		}
	}
}

/* The routine MD5Final terminates the message-digest computation and
   ends with the desired message digest in mdContext->digest[0...15].
 */
void MD5Final (unsigned char digest[16], MD5_CTX *mdContext)
{
	ULONG in[16];
	int mdi;
	unsigned int i, ii;
	unsigned int padLen;

	/* save number of bits */
	in[14] = mdContext->i[0];
	in[15] = mdContext->i[1];

	/* compute number of bytes mod 64 */
	mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

	/* pad out to 56 mod 64 */
	padLen = (mdi < 56) ? (56 - mdi) : (120 - mdi);
	MD5Update (mdContext, PADDING, padLen);

	/* append length in bits and transform */
	for (i = 0, ii = 0; i < 14; i++, ii += 4)
		in[i] = (((ULONG)mdContext->in[ii+3]) << 24) |
		        (((ULONG)mdContext->in[ii+2]) << 16) |
		        (((ULONG)mdContext->in[ii+1]) << 8) |
		        ((ULONG)mdContext->in[ii]);
	Transform (mdContext->buf, in);

	/* store buffer in digest */
	for (i = 0, ii = 0; i < 4; i++, ii += 4)
	{
		digest[ii]   = (unsigned char) (mdContext->buf[i]        & 0xFF);
		digest[ii+1] = (unsigned char)((mdContext->buf[i] >> 8)  & 0xFF);
		digest[ii+2] = (unsigned char)((mdContext->buf[i] >> 16) & 0xFF);
		digest[ii+3] = (unsigned char)((mdContext->buf[i] >> 24) & 0xFF);
	}
}

/* Basic MD5 step. Transforms buf based on in.  Note that if the Mysterious
   Constants are arranged backwards in little-endian order and decrypted with
   the DES they produce OCCULT MESSAGES!
 */
void Transform(register ULONG *buf,register ULONG *in)
{
	register ULONG a = buf[0], b = buf[1], c = buf[2], d = buf[3];

	/* Round 1 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
	FF ( a, b, c, d, in[ 0], S11, 0xD76AA478L); /* 1 */
	FF ( d, a, b, c, in[ 1], S12, 0xE8C7B756L); /* 2 */
	FF ( c, d, a, b, in[ 2], S13, 0x242070DBL); /* 3 */
	FF ( b, c, d, a, in[ 3], S14, 0xC1BDCEEEL); /* 4 */
	FF ( a, b, c, d, in[ 4], S11, 0xF57C0FAFL); /* 5 */
	FF ( d, a, b, c, in[ 5], S12, 0x4787C62AL); /* 6 */
	FF ( c, d, a, b, in[ 6], S13, 0xA8304613L); /* 7 */
	FF ( b, c, d, a, in[ 7], S14, 0xFD469501L); /* 8 */
	FF ( a, b, c, d, in[ 8], S11, 0x698098D8L); /* 9 */
	FF ( d, a, b, c, in[ 9], S12, 0x8B44F7AFL); /* 10 */
	FF ( c, d, a, b, in[10], S13, 0xFFFF5BB1L); /* 11 */
	FF ( b, c, d, a, in[11], S14, 0x895CD7BEL); /* 12 */
	FF ( a, b, c, d, in[12], S11, 0x6B901122L); /* 13 */
	FF ( d, a, b, c, in[13], S12, 0xFD987193L); /* 14 */
	FF ( c, d, a, b, in[14], S13, 0xA679438EL); /* 15 */
	FF ( b, c, d, a, in[15], S14, 0x49B40821L); /* 16 */

	/* Round 2 */
#define S21 5
#define S22 9
#define S23 14
#define S24 20
	GG ( a, b, c, d, in[ 1], S21, 0xF61E2562L); /* 17 */
	GG ( d, a, b, c, in[ 6], S22, 0xC040B340L); /* 18 */
	GG ( c, d, a, b, in[11], S23, 0x265E5A51L); /* 19 */
	GG ( b, c, d, a, in[ 0], S24, 0xE9B6C7AAL); /* 20 */
	GG ( a, b, c, d, in[ 5], S21, 0xD62F105DL); /* 21 */
	GG ( d, a, b, c, in[10], S22, 0x02441453L); /* 22 */
	GG ( c, d, a, b, in[15], S23, 0xD8A1E681L); /* 23 */
	GG ( b, c, d, a, in[ 4], S24, 0xE7D3FBC8L); /* 24 */
	GG ( a, b, c, d, in[ 9], S21, 0x21E1CDE6L); /* 25 */
	GG ( d, a, b, c, in[14], S22, 0xC33707D6L); /* 26 */
	GG ( c, d, a, b, in[ 3], S23, 0xF4D50D87L); /* 27 */
	GG ( b, c, d, a, in[ 8], S24, 0x455A14EDL); /* 28 */
	GG ( a, b, c, d, in[13], S21, 0xA9E3E905L); /* 29 */
	GG ( d, a, b, c, in[ 2], S22, 0xFCEFA3F8L); /* 30 */
	GG ( c, d, a, b, in[ 7], S23, 0x676F02D9L); /* 31 */
	GG ( b, c, d, a, in[12], S24, 0x8D2A4C8AL); /* 32 */

	/* Round 3 */
#define S31 4
#define S32 11
#define S33 16
#define S34 23
	HH ( a, b, c, d, in[ 5], S31, 0xFFFA3942L); /* 33 */
	HH ( d, a, b, c, in[ 8], S32, 0x8771F681L); /* 34 */
	HH ( c, d, a, b, in[11], S33, 0x6D9D6122L); /* 35 */
	HH ( b, c, d, a, in[14], S34, 0xFDE5380CL); /* 36 */
	HH ( a, b, c, d, in[ 1], S31, 0xA4BEEA44L); /* 37 */
	HH ( d, a, b, c, in[ 4], S32, 0x4BDECFA9L); /* 38 */
	HH ( c, d, a, b, in[ 7], S33, 0xF6BB4B60L); /* 39 */
	HH ( b, c, d, a, in[10], S34, 0xBEBFBC70L); /* 40 */
	HH ( a, b, c, d, in[13], S31, 0x289B7EC6L); /* 41 */
	HH ( d, a, b, c, in[ 0], S32, 0xEAA127FAL); /* 42 */
	HH ( c, d, a, b, in[ 3], S33, 0xD4EF3085L); /* 43 */
	HH ( b, c, d, a, in[ 6], S34, 0x04881D05L); /* 44 */
	HH ( a, b, c, d, in[ 9], S31, 0xD9D4D039L); /* 45 */
	HH ( d, a, b, c, in[12], S32, 0xE6DB99E5L); /* 46 */
	HH ( c, d, a, b, in[15], S33, 0x1FA27CF8L); /* 47 */
	HH ( b, c, d, a, in[ 2], S34, 0xC4AC5665L); /* 48 */

	/* Round 4 */
#define S41 6
#define S42 10
#define S43 15
#define S44 21
	II ( a, b, c, d, in[ 0], S41, 0xF4292244L); /* 49 */
	II ( d, a, b, c, in[ 7], S42, 0x432AFF97L); /* 50 */
	II ( c, d, a, b, in[14], S43, 0xAB9423A7L); /* 51 */
	II ( b, c, d, a, in[ 5], S44, 0xFC93A039L); /* 52 */
	II ( a, b, c, d, in[12], S41, 0x655B59C3L); /* 53 */
	II ( d, a, b, c, in[ 3], S42, 0x8F0CCC92L); /* 54 */
	II ( c, d, a, b, in[10], S43, 0xFFEFF47DL); /* 55 */
	II ( b, c, d, a, in[ 1], S44, 0x85845DD1L); /* 56 */
	II ( a, b, c, d, in[ 8], S41, 0x6FA87E4FL); /* 57 */
	II ( d, a, b, c, in[15], S42, 0xFE2CE6E0L); /* 58 */
	II ( c, d, a, b, in[ 6], S43, 0xA3014314L); /* 59 */
	II ( b, c, d, a, in[13], S44, 0x4E0811A1L); /* 60 */
	II ( a, b, c, d, in[ 4], S41, 0xF7537E82L); /* 61 */
	II ( d, a, b, c, in[11], S42, 0xBD3AF235L); /* 62 */
	II ( c, d, a, b, in[ 2], S43, 0x2AD7D2BBL); /* 63 */
	II ( b, c, d, a, in[ 9], S44, 0xEB86D391L); /* 64 */

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

/*==[end md5.c]============================================================*/


static const char version[] = "$VER: Version 42.0 (18.10.2005)\n";

static const char ERROR_HEADER[] = "Version";

#define TEMPLATE "NAME/M,MD5SUM/S,VERSION/N,REVISION/N,FILE/S,FULL/S,RES/S"
struct
{
	CONST_STRPTR *arg_name;
	IPTR    arg_md5sum;
	LONG   *arg_version;
	LONG   *arg_revision;
	IPTR    arg_file;
	IPTR    arg_full;
	IPTR    arg_res;
}
args;

LONG mversion, mrevision;

struct
{
	STRPTR pv_name;
	ULONG  pv_flags;
	LONG   pv_version;
	LONG   pv_revision;
	//LONG   pv_days;
	STRPTR pv_vername;
	STRPTR pv_revname;
	STRPTR pv_datestr;
	STRPTR pv_extralf;
	STRPTR pv_extrastr;
	UBYTE  pv_md5sum[16];
}
parsedver = { NULL, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, {0}};

#define PVF_MD5SUM		(1 << 0)
#define PVF_NOVERSION	(1 << 1)

static
int makeverstring(CONST_STRPTR name);
static
void printverstring(void);
static
void freeverstring(void);
static
int makesysver(void);
static
int cmpargsparsed(void);


/**************************** support functions ************************/

/* Duplicate string, by given length or -1 for full length
 */
static
STRPTR dupstr(CONST_STRPTR buffer, LONG len)
{
	STRPTR ret = NULL;

	if (buffer)
	{
		if (len == -1)
			len = strlen(buffer);

		ret = AllocVec(len + 1, MEMF_ANY);
		if (ret)
		{
			CopyMem((STRPTR) buffer, ret, len);
			ret[len] = '\0';
		}
	}

	return ret;
}


static
inline int myisspace(int c)
{
	return (c == ' ' || c == '\t');
}


/* Return a pointer to a string, stripped by all leading whitespace characters
 * (SPACE, TAB).
 */
static
STRPTR skipwhites(CONST_STRPTR buffer)
{
	for (;; buffer++)
	{
		if (buffer[0] == '\0' || !isspace(buffer[0]))
		{
			return (STRPTR) buffer;
		}
	}
}


/* Return a pointer to a string, stripped by all leading space characters
 * (SPACE).
 */
static
STRPTR skipspaces(CONST_STRPTR buffer)
{
	for (;; buffer++)
	{
		if (buffer[0] == '\0' || buffer[0] != ' ')
		{
			return (STRPTR) buffer;
		}
	}
}


/* Strip all whitespace-characters from the end of a string. Note that the
 * buffer passed in will be modified!
 */
static
void stripwhites(STRPTR buffer)
{
	int len = strlen(buffer);

	while (len > 0)
	{
		if (!isspace(buffer[len-1]))
		{
			buffer[len] = '\0';
			return;
		}
		len--;
	}
	buffer[len] = '\0';
}


/* Searches for a given string in a file and stores up to *lenptr characters
 * into the buffer beginning with the first character after the given string.
 */
static
int findinfile(BPTR file, CONST_STRPTR string, STRPTR buffer, int *lenptr, unsigned char digest[16])
{
	int error = RETURN_OK;
	int buflen = *lenptr, len = 0, pos, stringlen;
	BOOL ready = FALSE;
	MD5_CTX md5ctx;
	STRPTR bufpos;
	STRPTR tmp;

	tmp = AllocMem(buflen, MEMF_PUBLIC);
	if (!tmp)
	{
		return RETURN_FAIL;
	}

	stringlen = strlen(string);
	*lenptr = -1;

	if (args.arg_md5sum)
	{
		MD5Init(&md5ctx);
	}

	bufpos = tmp;
	while ((len = Read(file, &tmp[len], buflen - len)) > 0)
	{
		pos = 0;

		if (args.arg_md5sum)
		{
			MD5Update(&md5ctx, bufpos, len);
		}

		if (ready)
		{
			/* If we get here we're scanning the rest of the file for md5sum. - Piru */
			len = 0;
		}
		else
		{
			while ((len - pos) >= stringlen)
			{
				/* Compare the current buffer position with the supplied string. */
				if (strncmp(&tmp[pos], string, stringlen) == 0)
				{
					/* It is equal! Now move the rest of the buffer to the top of
					 * the buffer and fill it up.
					 */
					int findstrlen = len - pos;

					memcpy(buffer, &tmp[pos + stringlen], findstrlen);

					len = Read(file, &buffer[findstrlen], buflen - findstrlen);
					if (len >= 0)
					{
						if (args.arg_md5sum)
						{
							MD5Update(&md5ctx, &buffer[findstrlen], len);
						}

						*lenptr = findstrlen + len;
					}
					else
					{
						error = RETURN_FAIL;
					}
					ready = TRUE;
					break;
				}
				pos++;
			}
			/* Move the rest of the buffer that could not be compared (because it
			 * is smaller than the string to compare) to the top of the buffer.
			 */
			if (!ready)
			{
				memmove(tmp, &tmp[len - stringlen], stringlen);
			}
			else
			{
				/* If we're not md5summing, stop file scanning now. - Piru */
				if (!args.arg_md5sum)
				{
					break;
				}
			}
			len = stringlen;
		}

		bufpos = &tmp[len];
	}

	FreeMem(tmp, buflen);

	if (len == -1)
	{
		error = RETURN_FAIL;
	}

	if (args.arg_md5sum)
	{
		memset(digest, 0, 16);
		MD5Final(digest, &md5ctx);
	}

	return error;
}


/*************************** parsing functions *************************/

/* Convert a date in the form DD.MM.YY or DD.MM.YYYY into a numerical
 * value. Return FALSE, if buffer doesn't contain a valid date.
 */
static
BOOL makedatefromstring(CONST_STRPTR *bufptr)
{
	CONST_STRPTR buffer = *bufptr;
	struct DateTime dt;
	CONST_STRPTR headerstart, end;
	STRPTR newbuf;
	LONG res;
	int len, i;
	UBYTE c;

	//if (isspace(buffer[0]))
	//  buffer++;

	headerstart = buffer;

	buffer = strchr(buffer, '(');
	if (!buffer)
	{
		return FALSE;
	}
	buffer++;
	end = strchr(buffer, ')');
	if (!end)
	{
		return FALSE;
	}
	len = (int)(end - buffer);
	newbuf = dupstr(buffer, len);
	if (!newbuf)
	{
		return FALSE;
	}
	for (i = 0; i < len; i++)
	{
		c = newbuf[i];

		if (c == '.' || c == '/')
			newbuf[i] = '-';
		else if (!isalnum(c))
		{
			end = buffer + i;
			newbuf[i] = '\0';
			break;
		}
	}

	//Printf("date: \"%s\"\n", (LONG) newbuf);

	dt.dat_Format  = FORMAT_CDN;
	dt.dat_Flags   = 0;
	dt.dat_StrDay  = NULL;
	dt.dat_StrDate = newbuf;
	dt.dat_StrTime = NULL;
	res = StrToDate(&dt);
	if (!res)
	{
		dt.dat_Format  = FORMAT_DOS;
		res = StrToDate(&dt);
		if (!res)
		{
			//Printf("StrToDate failed!\n");
			FreeVec(newbuf);
			return FALSE;
		}
	}
	FreeVec(newbuf);

	//parsedver.pv_days = dt.dat_Stamp.ds_Days;

	parsedver.pv_datestr = AllocVec(buffer - headerstart + LEN_DATSTRING + 2, MEMF_ANY);
	if (!parsedver.pv_datestr)
	{
		return FALSE;
	}

	dt.dat_Stamp.ds_Minute = 0;
	dt.dat_Stamp.ds_Tick   = 0;

	dt.dat_StrDate = parsedver.pv_datestr + (buffer - headerstart);
	dt.dat_Format  = FORMAT_DEF;
	if (!DateToStr(&dt))
	{
		//Printf("DateToStr failed!\n");
		return FALSE;
	}

	CopyMem((STRPTR) headerstart, parsedver.pv_datestr, buffer - headerstart);
	res = strlen(parsedver.pv_datestr);
	parsedver.pv_datestr[res++] = ')';
	parsedver.pv_datestr[res] = '\0';

	*bufptr = end + 1;

	return TRUE;
}


/* Check whether the given string contains a version in the form
 * <version>.<revision> . If not return FALSE, otherwise fill in parsedver and
 * return TRUE.
 */
static
BOOL makeversionfromstring(CONST_STRPTR *bufptr)
{
	LONG pos, ver, rev;
	CONST_STRPTR buffer = *bufptr;
	CONST_STRPTR verstart, revstart;

	//Printf("makeversionfromstring: buffer \"%s\"\n", (LONG) buffer);

	/* Do version */

	verstart = buffer;
	pos = StrToLong((STRPTR) buffer, &ver);
	if (pos == -1)
	{
		return FALSE;
	}
	parsedver.pv_version  = ver;
	parsedver.pv_revision = -1;

	parsedver.pv_vername = dupstr(verstart, pos);
	if (!parsedver.pv_vername)
	{
		return FALSE;
	}

	/* Do revision */

	buffer += pos;
	revstart = buffer;
	buffer = skipspaces(buffer);  /* NOTE: skipspaces, not skipwhites! */
	if (*buffer != '.')
	{
		*bufptr = buffer;
		return TRUE;
	}

	buffer++;
	pos = StrToLong((STRPTR) buffer, &rev);
	if (pos == -1)
	{
		*bufptr = buffer;
		return TRUE;
	}

	parsedver.pv_revision = rev;

	/* calc the revision string len */
	pos = buffer + pos - revstart;
	parsedver.pv_revname = dupstr(revstart, pos);
	if (!parsedver.pv_revname)
	{
		return FALSE;
	}

	*bufptr = revstart + pos;

	return TRUE;
}


static
void printverstring(void)
{
	if (args.arg_md5sum)
	{
		if (parsedver.pv_flags & PVF_MD5SUM)
		{
		#ifdef __AROS__
		    	/* Endianess safe version */
			
			Printf("%02lX%02lX%02lX%02lX"
			       "%02lX%02lX%02lX%02lX"
			       "%02lX%02lX%02lX%02lX"
			       "%02lX%02lX%02lX%02lX  ",
			       parsedver.pv_md5sum[0],
			       parsedver.pv_md5sum[1],
			       parsedver.pv_md5sum[2],
			       parsedver.pv_md5sum[3],
			       parsedver.pv_md5sum[4],
			       parsedver.pv_md5sum[5],
			       parsedver.pv_md5sum[6],
			       parsedver.pv_md5sum[7],
			       parsedver.pv_md5sum[8],
			       parsedver.pv_md5sum[9],
			       parsedver.pv_md5sum[10],
			       parsedver.pv_md5sum[11],
			       parsedver.pv_md5sum[12],
			       parsedver.pv_md5sum[13],
			       parsedver.pv_md5sum[14],
			       parsedver.pv_md5sum[15]);
			       
    	    	#else		
			VPrintf("%08lX%08lX%08lX%08lX  ", parsedver.pv_md5sum);
    	    	#endif			
		}
		else
		{
			/*     "01234567012345670123456701234567: " */
			PutStr("<no md5sum available>             ");
		}
	}

	if (parsedver.pv_flags & PVF_NOVERSION)
	{
		Printf("%s\n", (LONG) parsedver.pv_name);
	}
	else
	{
		if (args.arg_full)
		{
			/* If md5sum output was there, avoid linefeed to allow parsing the output - Piru */
			if (args.arg_md5sum)
			{
				parsedver.pv_extralf = " ";
			}

			Printf("%s%s%s%s%s%s%s\n",
			       (LONG) parsedver.pv_name, (LONG) (*parsedver.pv_name ? " " : ""),
			       (LONG) parsedver.pv_vername, (LONG) parsedver.pv_revname,
			       (LONG) parsedver.pv_datestr, (LONG) parsedver.pv_extralf, (LONG) parsedver.pv_extrastr);
		}
		else
		{
			Printf("%s%s%s%s\n",
			       (LONG) parsedver.pv_name, (LONG) (*parsedver.pv_name ? " " : ""),
			       (LONG) parsedver.pv_vername, (LONG) parsedver.pv_revname);
		}
	}
}


static
int makedata(CONST_STRPTR buffer, CONST_STRPTR ptr, int pos)
{
	//Printf("makedata: buffer \"%s\" prt \"%s\"\n", (LONG) buffer, (LONG) ptr);

	if (makeversionfromstring(&ptr))
	{
		CONST_STRPTR endp;
		BOOL doskip;

		/* It is! */
		/* Copy the program-name into a buffer. */
		parsedver.pv_name = dupstr(buffer, pos);
		if (!parsedver.pv_name)
		{
			PrintFault(ERROR_NO_FREE_STORE, (STRPTR) ERROR_HEADER);
			return RETURN_FAIL;
		}

		/* Now find the date */
		//Printf("makedatafromstring: ptr #1: \"%s\"\n", (LONG) ptr);
		doskip = strchr(ptr, '(') ? TRUE : FALSE;
		(void) makedatefromstring(&ptr);

		//Printf("makedatafromstring: ptr #2: \"%s\"\n", (LONG) ptr);
		if (doskip)
			ptr = skipspaces(ptr);	/* NOTE: not skipwhites! */
		for (endp = ptr; *endp != '\0' && *endp != '\r' && *endp != '\n'; endp++)
			;
		pos = endp - ptr;
		if (pos)
		{
			parsedver.pv_extrastr = dupstr(ptr, pos);
			if (!parsedver.pv_extrastr)
			{
				PrintFault(ERROR_NO_FREE_STORE, (STRPTR) ERROR_HEADER);
				return RETURN_FAIL;
			}

			if (doskip)
				parsedver.pv_extralf = "\n";
		}

		return 1;
	}

	return 0;
}


/* Retrieves version information from string. The data is stored in the
 * global struct parsedver.pv_
 */

static
int makedatafromstring(CONST_STRPTR buffer)
{
	int error = RETURN_OK;
	int pos = 0;
	LONG add, dummy;

	while (buffer[pos] && buffer[pos] != '\r' && buffer[pos] != '\n')
	{
		/* NOTE: Not isspace()! - Piru */
		if (myisspace(buffer[pos]) &&
		    (add = StrToLong((STRPTR) buffer + pos + 1, &dummy)) != -1)
		{
			CONST_STRPTR ptr;

			/* Found something, which looks like a version. Now check, if it
			 * really is.
			 */

			//Printf("makedatafromstring: buffer + %ld: \"%s\"\n", pos, (LONG) buffer + pos);

			ptr = buffer + pos + 1;

			if (makedata(buffer, ptr, pos))
			{
				break;
			}
		}
		pos++;
	}

	if (!buffer[pos] || buffer[pos] == '\r' || buffer[pos] == '\n')
	{
		CONST_STRPTR endp;

		/* use the whatever is after ver tag as name */
		for (endp = buffer; *endp != '\0' && *endp != '\r' && *endp != '\n'; endp++)
			;
		pos = endp - buffer;

		parsedver.pv_name = dupstr(buffer, pos);
		if (!parsedver.pv_name)
		{
			PrintFault(ERROR_NO_FREE_STORE, (STRPTR) ERROR_HEADER);
			return RETURN_FAIL;
		}
	}

	/* Strip any whitespaces from the tail of the program-name.
	 */
	if (parsedver.pv_name)
	{
		stripwhites(parsedver.pv_name);
	}

	return error;
}


/* Case-insensitive FindResident()
 */
static
struct Resident *findresident(CONST_STRPTR name)
{
	struct Resident **rp;
	struct Resident *resident;

	rp = (struct Resident **) SysBase->ResModules;

	while ((resident = *rp++))
	{
		if (((LONG) resident) > 0)
		{
			if (!Stricmp(resident->rt_Name, (STRPTR) name))
			{
				break;
			}
		}
		else
		{
			rp = (struct Resident **) (((ULONG) resident) & (ULONG) ~(1 << 31));
		}
	}

	return resident;
}


/* Case-insensitive FindName()
 */
static
struct Node *findname(struct List *list, CONST_STRPTR name)
{
	struct Node *node;

	ForeachNode(list, node)
	{
		if (!Stricmp(node->ln_Name, (STRPTR) name))
		{
			return node;
		}
	}

	return NULL;
}


/* Retrieve information from resident modules. Returns 0 for success.
 */
static
int createresidentver(struct Resident *MyResident)
{
	STRPTR buffer = NULL, tmpbuffer;
	int error, pos = 0, foundver = FALSE;

	if (MyResident->rt_IdString)
	{
		buffer = skipwhites(MyResident->rt_IdString);
		//Printf("createresidentver: buffer \"%s\"\n", (LONG) buffer);

		/* Locate version part */
		while (buffer[pos])
		{
			LONG dummy;

			/* NOTE: Not isspace()! - Piru */
			if (myisspace(buffer[pos]) &&
			    StrToLong(buffer + pos + 1, &dummy) != -1)
			{
				buffer += pos;
				foundver = TRUE;
				break;
			}
			pos++;
		}
		//Printf("createresidentver: buffer: \"%s\"\n", (LONG) buffer);
	}


	/* If could not find any version info, use the resident rt_Name */
	if (!foundver)
		buffer = "";

	//Printf("createresidentver: buffer: \"%s\"\n", (LONG) buffer);

	tmpbuffer = AllocVec(strlen(MyResident->rt_Name) + strlen(buffer) + 1, MEMF_ANY);
	if (!tmpbuffer)
	{
		PrintFault(ERROR_NO_FREE_STORE, (STRPTR) ERROR_HEADER);
		return RETURN_FAIL;
	}

	strcpy(tmpbuffer, MyResident->rt_Name);
	strcat(tmpbuffer, buffer);
	//Printf("createresidentver: tmpbuffer: \"%s\"\n", (LONG) tmpbuffer);
	error = makedatafromstring(tmpbuffer);

	FreeVec(tmpbuffer);

	return error;
}


/* Retrieve version information from library. Returns 0 for success.
 */
static
int createlibraryver(struct Library *MyLibrary)
{
	STRPTR buffer, tmpbuffer;
	int error, foundver = FALSE, pos;

	if (MyLibrary->lib_IdString)
	{
		//Printf("createlibraryver: lib_IdString \"%s\"\n", (LONG) MyLibrary->lib_IdString);
		buffer = skipwhites(MyLibrary->lib_IdString);

		//Printf("createlibraryver: buffer \"%s\"\n", (LONG) buffer);

		/* Find full 'ver.rev' version info
		 */
		pos = 0;
		while (buffer[pos])
		{
			LONG dummy, add;

			/* NOTE: Not isspace()! - Piru */
			if (myisspace(buffer[pos]) &&
			    (add = StrToLong(buffer + pos + 1, &dummy)) != -1 &&
				buffer[pos + 1 + add] == '.')
			{
				buffer += pos;
				pos = 0;
				foundver = TRUE;
				break;
			}
			pos++;
		}

		/* If could not find 'ver.rev', find any numeric */
		if (!foundver)
		{
			pos = 0;
			while (buffer[pos])
			{
				LONG dummy;

				/* NOTE: Not isspace()! - Piru */
				if (myisspace(buffer[pos]) &&
				    StrToLong(buffer + pos + 1, &dummy) != -1)
				{
					buffer += pos;
					pos = 0;
					foundver = TRUE;
					break;
				}
				pos++;
			}
		}
	}

	/* If could not find any version info, use the resident rt_Name */
	if (!foundver)
	{
		buffer = "";
		error = RETURN_WARN;
	}

	tmpbuffer = AllocVec(strlen(MyLibrary->lib_Node.ln_Name) + strlen(buffer) + 1, MEMF_ANY);
	if (!tmpbuffer)
	{
		PrintFault(ERROR_NO_FREE_STORE, (STRPTR) ERROR_HEADER);
		return RETURN_FAIL;
	}

	strcpy(tmpbuffer, MyLibrary->lib_Node.ln_Name);
	strcat(tmpbuffer, buffer);
	//Printf("createlibraryver: tmpbuffer: \"%s\"\n", (LONG) tmpbuffer);
	pos = makedatafromstring(tmpbuffer);
	if (pos > error)
	{
		error = pos;
	}

	FreeVec(tmpbuffer);

	return error;
}


/* Create default strings
 */
static
int createdefvers(CONST_STRPTR name)
{
	FreeVec(parsedver.pv_revname);
	FreeVec(parsedver.pv_vername);
	FreeVec(parsedver.pv_name);

	parsedver.pv_name    = dupstr(name, -1);
	parsedver.pv_vername = AllocVec(14, MEMF_ANY);
	parsedver.pv_revname = AllocVec(15, MEMF_ANY);

	if (parsedver.pv_name &&
	    parsedver.pv_vername &&
	    parsedver.pv_revname)
	{
		sprintf(parsedver.pv_vername, "%ld", parsedver.pv_version);
		sprintf(parsedver.pv_revname, ".%ld", parsedver.pv_revision);

		return RETURN_OK;
	}

	return RETURN_FAIL;
}


/* Create version info from named resident
 */
static
int makeresidentver(CONST_STRPTR name)
{
	struct Resident *MyResident;
	int error = -1;

	if ((MyResident = findresident(name)))
	{
		error = createresidentver(MyResident);
		if (error != RETURN_OK)
		{
			/* get values from residenttag */
			parsedver.pv_version  = MyResident->rt_Version;
			parsedver.pv_revision = -1;
			error = createdefvers(MyResident->rt_Name);
		}
	}

	return error;
}


/* Create version info from named list node
*/
static
int makeexeclistver(struct List *list, CONST_STRPTR name)
{
	struct Library *MyLibrary;
	int error = -1;

	Forbid();

	MyLibrary = (struct Library *) findname(list, name);
	if (MyLibrary)
	{
		/* get values from library */
		ULONG ver = MyLibrary->lib_Version;
		ULONG rev = MyLibrary->lib_Revision;

		error = createlibraryver(MyLibrary);
		if (error != RETURN_OK ||
		    parsedver.pv_version != ver ||
		    parsedver.pv_revision != rev)
		{
			/* special case where createlibraryrev was successful, but
			 * version or revision don't match.
			 */
			if (error == RETURN_OK)
			{
				/* If there is extrastr, make sure there's linefeed, too.
				 */
				if (parsedver.pv_extrastr)
				{
					parsedver.pv_extralf = "\n";
				}
			}

			/* get values from library */
			parsedver.pv_version  = ver;
			parsedver.pv_revision = rev;

			error = createdefvers(MyLibrary->lib_Node.ln_Name);
		}
	}

	Permit();

	return error;
}


/* Find resident from seglist, return NULL if none found
*/
static
struct Resident *FindLibResident(BPTR	Segment)
{
	while (Segment)
	{
		ULONG		*MySegment;
		UWORD		*MyBuffer;
		UWORD		*EndBuffer;

		MySegment	= (ULONG*) BADDR(Segment);
		MyBuffer	= (UWORD*) &MySegment[1];
		EndBuffer	= (UWORD*) &MySegment[(MySegment[-1] - sizeof(ULONG) * 4) / sizeof(ULONG)];

		while (MyBuffer < EndBuffer)
		{
			struct Resident	*MyResident;

			MyResident = (struct Resident*) MyBuffer;
			if (MyResident->rt_MatchWord == RTC_MATCHWORD &&
			    MyResident->rt_MatchTag == MyResident)
			{
				return MyResident;
			}

			MyBuffer++;
		}

		Segment	=(BPTR) MySegment[0];
	}

	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	return NULL;
}


/* Find $VER: tag from seglist, return NULL if none found
   Returns dupstr()d string or NULL.
*/
static
STRPTR FindSegmentVER(BPTR	Segment)
{
	while (Segment)
	{
		ULONG		*MySegment;
		CONST_STRPTR	MyBuffer;
		CONST_STRPTR	EndBuffer;
		CONST_STRPTR	SegmentEnd;

		MySegment	= (ULONG*) BADDR(Segment);
		MyBuffer	= (CONST_STRPTR) &MySegment[1];
		SegmentEnd	= ((CONST_STRPTR) MySegment) + MySegment[-1] - sizeof(ULONG) * 2;
		EndBuffer	= SegmentEnd - 5;

		while (MyBuffer < EndBuffer)
		{
			if (MyBuffer[0] == '$' &&
			    MyBuffer[1] == 'V' &&
			    MyBuffer[2] == 'E' &&
			    MyBuffer[3] == 'R' &&
			    MyBuffer[4] == ':')
			{
				CONST_STRPTR EndPtr;

				MyBuffer += 5;
				/* Required because some smartass could end his $VER: tag
				 * without '\0' in the segment to save space. - Piru
				 */
				for (EndPtr = MyBuffer; EndPtr < SegmentEnd && *EndPtr; EndPtr++)
					;
				if (EndPtr - MyBuffer)
				{
					return dupstr(MyBuffer, EndPtr - MyBuffer);
				}
			}

			MyBuffer++;
		}

		Segment	=(BPTR) MySegment[0];
	}

	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	return NULL;
}


/* Create version info from named device
*/
static
int makedevicever(CONST_STRPTR name)
{
	struct DevProc *MyDevProc;
	int error = -1;

	MyDevProc = GetDeviceProc((STRPTR) name, NULL);
	if (MyDevProc)
	{
		if (MyDevProc->dvp_DevNode->dol_Type == DLT_DEVICE)
		{
		#ifdef __AROS__
		#warning "FIXME: AROS specific version info for devices"		
		    	error = RETURN_FAIL;
    	    	#else			
			BPTR SegList;

			SegList = MyDevProc->dvp_DevNode->dol_misc.dol_handler.dol_SegList;
			if (SegList)
			{
				struct Resident *MyResident;

				MyResident = FindLibResident(SegList);
				if (MyResident)
				{
					error = createresidentver(MyResident);
				}
				else
					error = RETURN_FAIL;
			}
		#endif
		}
		FreeDeviceProc(MyDevProc);
	}

	if (error != RETURN_OK && error != -1)
	{
		Printf("Could not find version information for '%s'\n", (LONG) name);
	}

	return error;
}


/* Retrieve version information from file. Return 0 for success.
 */
#define BUFFERSIZE (16384 + 1)
static
int makefilever(CONST_STRPTR name)
{
	BPTR file;
	int error; // = RETURN_OK;

	file = Open((STRPTR) name, MODE_OLDFILE);
	if (file)
	{
		UBYTE *buffer;

		buffer = AllocMem(BUFFERSIZE, MEMF_PUBLIC);
		if (buffer)
		{
			int len = BUFFERSIZE - 1;

			error = findinfile(file, "$VER:", buffer, &len, parsedver.pv_md5sum);
			if (error == RETURN_OK)
			{
				parsedver.pv_flags |= PVF_MD5SUM;

				if (len >= 0)
				{
					STRPTR startbuffer;

					buffer[len] = '\0';
					startbuffer = skipwhites(buffer);

					//Printf("startbuffer \"%s\"\n", startbuffer);
					error = makedatafromstring(startbuffer);
				}
				else
				{
					/* Try LoadSeg
					 */
					error = RETURN_ERROR;

					Close(file);

					file = LoadSeg((STRPTR) name);
					if (file)
					{
						struct Resident *MyResident;

						MyResident = FindLibResident(file);
						if (MyResident /*&&
						    (MyResident->rt_Type == NT_LIBRARY ||
						     MyResident->rt_Type == NT_DEVICE)*/)
						{
							error = createresidentver(MyResident);
						}

						UnLoadSeg(file);
					}
					file = NULL;

					if (error != RETURN_OK)
					{
						/* If user didn't ask for md5sum or we could not calculate it.
						*/
						if (!args.arg_md5sum || (!(parsedver.pv_flags & PVF_MD5SUM)))
						{
							Printf("Could not find version information for '%s'\n", (LONG) name);
						}
					}
				}
			}
			else
			{
				PrintFault(IoErr(), (STRPTR) ERROR_HEADER);
			}

			FreeMem(buffer, BUFFERSIZE);
		}
		else
		{
			error = RETURN_FAIL;
			PrintFault(IoErr(), (STRPTR) ERROR_HEADER);
		}

		if (file)
			Close(file);
	}
	else
	{
		LONG ioerr = IoErr();

		if (ioerr == ERROR_OBJECT_NOT_FOUND ||
		    ioerr == ERROR_OBJECT_WRONG_TYPE)
		{
			error = -1;
		}
		else
		{
			PrintFault(IoErr(), (STRPTR) ERROR_HEADER);
			error = RETURN_FAIL;
		}
	}

	return error;
}


static
int makerescmdver(CONST_STRPTR name)
{
	int error = -1;
	struct Segment *segment;

	Forbid();

	segment = FindSegment((STRPTR) name, NULL, 0);
	if (!segment)
	{
		segment = FindSegment((STRPTR) name, NULL, 1);
	}

	if (segment)
	{
		if (segment->seg_UC == CMD_INTERNAL ||
		    segment->seg_UC == CMD_DISABLED)
		{
			Permit();
			error = makeresidentver("shell");
			Forbid();
		}
		else
		{
			STRPTR buffer = FindSegmentVER(segment->seg_Seg);
			if (buffer)
			{
				STRPTR startbuffer;

				startbuffer = skipwhites(buffer);

				//Printf("startbuffer \"%s\"\n", (LONG) startbuffer);
				error = makedatafromstring(startbuffer);

				FreeVec(buffer);
			}
		}
	}

	Permit();

	return error;
}

static
int setvervar(CONST_STRPTR name, LONG ver, LONG rev)
{
	UBYTE buf[32];

	sprintf(buf, "%ld.%ld", (LONG) ver, (LONG) rev);

	return SetVar((STRPTR) name, buf, -1, GVF_LOCAL_ONLY | LV_VAR) ? RETURN_OK : -1;
}


static
int makekickversion(void)
{
	parsedver.pv_version  = SysBase->LibNode.lib_Version;
	parsedver.pv_revision = SysBase->SoftVer;

	setvervar("Kickstart",
	          parsedver.pv_version,
	          parsedver.pv_revision);

	Printf("Kickstart %ld.%ld",
	       (LONG) parsedver.pv_version, (LONG) parsedver.pv_revision);

	return RETURN_OK;
}


static
int makewbversion(void)
{
	int error = -1;
	struct Library *VersionBase;

	VersionBase = OpenLibrary("version.library", 0);
	if (VersionBase)
	{
		error = makeexeclistver(&SysBase->LibList, "version.library");

		if (error == RETURN_OK)
		{
			STRPTR newname = dupstr("Workbench", -1);
			if (newname)
			{
				FreeVec(parsedver.pv_name);
				parsedver.pv_name = newname;
			}
			setvervar("Workbench", parsedver.pv_version, parsedver.pv_revision);
		}

		CloseLibrary(VersionBase);
	}

	return error;
}


static
int makesysver(void)
{
	int error;

	error = makekickversion();
	if (error == RETURN_OK)
	{
		error = makewbversion();

		if (error == RETURN_OK)
		{
			PutStr(", ");
		}
		else
		{
			/* prevent silly errormsg if no version.library */
			PutStr("\n");
			error = RETURN_WARN;
		}
	}

	return error;
}


/* Determine, by which means to get the version-string.
 */
static
int makeverstring(CONST_STRPTR name)
{
	int error; // = RETURN_OK;
	BOOL volume = name[strlen(name) - 1] == ':';
	CONST_STRPTR filepart = FilePart(name);

	error = -1;

	if (!volume && !args.arg_file)
	{
		if (*filepart)
		{
			error = makeresidentver(filepart);
			if (error != RETURN_OK)
			{

				/* Try libraries
				 */
				error = makeexeclistver(&SysBase->LibList, filepart);
				if (error != RETURN_OK)
				{
					STRPTR namebuf;
					ULONG namelen = strlen(filepart);

					/* 12 is "MOSSYS:LIBS/" */
					if ((namebuf = AllocVec(12 + namelen + 4 + 1, MEMF_PUBLIC)))
					{
						strcpy(namebuf, "LIBS:");
						strcat(namebuf, filepart);
						error = makefilever(namebuf);

						/* Try devices
						*/
						if (error != RETURN_OK)
						{
							error = makeexeclistver(&SysBase->DeviceList, filepart);
							if (error != RETURN_OK)
							{
								strcpy(namebuf, "DEVS:");
								strcat(namebuf, filepart);
								error = makefilever(namebuf);
							}
						}
						FreeVec(namebuf);
					}
				}
			}
		}
	}

	if (!args.arg_res && error == -1)
	{
		if (volume)
		{
			error = makedevicever(name);
		}
		else
		{
			if (*filepart)
			{
				error = makefilever(name);
			}
		}
	}

	if (!args.arg_file && error == -1)
	{
		error = makerescmdver(name);
	}

	if (error)
	{
		/* If user asked for md5sum, and we could calculate it, don't print error
		 *  but the md5sum + file.
		 */
		if (args.arg_md5sum && (parsedver.pv_flags & PVF_MD5SUM))
		{
			parsedver.pv_name = dupstr(name, -1);
			parsedver.pv_flags |= PVF_NOVERSION;
			error = RETURN_OK;
		}
	}

	if (error == -1)
	{
		PrintFault(ERROR_OBJECT_NOT_FOUND, (STRPTR) ERROR_HEADER);
		error = RETURN_FAIL;
	}

	return error;
}


static
void freeverstring(void)
{
	parsedver.pv_flags    = 0;
	parsedver.pv_version  = 0;
	parsedver.pv_revision = 0;

	FreeVec(parsedver.pv_extrastr);
	parsedver.pv_extrastr = NULL;
	parsedver.pv_extralf = NULL;
	FreeVec(parsedver.pv_datestr);
	parsedver.pv_datestr  = NULL;
	FreeVec(parsedver.pv_revname);
	parsedver.pv_revname  = NULL;
	FreeVec(parsedver.pv_vername);
	parsedver.pv_vername  = NULL;
	FreeVec(parsedver.pv_name);
	parsedver.pv_name     = NULL;
}

/* Compare the version given as argument with the version from the object.
 * Return RETURN_WARN, if args-v>object-v, otherwise return RETURN_OK.
 */
static
int cmpargsparsed(void)
{
	if (args.arg_version)
	{
		if (*(args.arg_version) > parsedver.pv_version)
		{
			return RETURN_WARN;
		}
		else if (*(args.arg_version) == parsedver.pv_version && args.arg_revision)
		{
			if (*(args.arg_revision) > parsedver.pv_revision)
			{
				return RETURN_WARN;
			}
		}
	}
	else if (args.arg_revision)
	{
		if (*(args.arg_revision) > parsedver.pv_revision)
		{
			return RETURN_WARN;
		}
	}
	return RETURN_OK;
}

/******************************* main program ****************************/

int __nocommandline;

int main (void)
{
	LONG error = RETURN_FAIL;

	struct RDArgs *rda;

	rda = ReadArgs(TEMPLATE, (IPTR *) &args, NULL);
	if (rda)
	{
		if (!args.arg_name || !*args.arg_name)
		{
			/* No args, make system version */
			error = makesysver();
			if (error == RETURN_OK)
			{
				printverstring();
					if (parsedver.pv_flags & PVF_NOVERSION)
				{
					error = RETURN_FAIL;
				}
				if (error == RETURN_OK)
				{
					error = cmpargsparsed();
				}
			}
			freeverstring();
		}
		else
		{
			CONST_STRPTR *name;
			BOOL multifile;
#if 1
			/* Workaround for:
			 * version file ver
			 * version file ver rev
			 */
			if (!args.arg_version && !args.arg_revision)
			{
				LONG narg = 1;
				while (args.arg_name[narg]) { narg++; }
				if (narg == 2 || narg == 3)
				{
					if (StrToLong(args.arg_name[1], &mversion) > 0)
					{
						args.arg_version = &mversion;
							args.arg_name[1] = args.arg_name[2];
						if (narg == 3)
						{
							args.arg_name[2] = NULL;
						}
							if (narg == 3)
						{
							if (StrToLong(args.arg_name[1], &mrevision) > 0)
							{
								args.arg_revision = &mrevision;
								args.arg_name[1] = NULL;
							}
						}
					}
				}
			}
#endif
			multifile = args.arg_name[1] != NULL;

			for (name = args.arg_name; *name; name++)
			{
				error = makeverstring(*name);
				if (error == RETURN_OK)
				{
					printverstring();

					if (!multifile)
					{
						/* Single args, do compare stuff also */
						if (parsedver.pv_flags & PVF_NOVERSION)
						{
							error = RETURN_FAIL;
						}
						if (error == RETURN_OK)
						{
							error = cmpargsparsed();
						}
					}
				}
				freeverstring();

			}
		}

		FreeArgs(rda);
	}
	else
	{
		PrintFault(IoErr(), (STRPTR) ERROR_HEADER);
		error = RETURN_FAIL;
	}

        RT_Exit();

        return(error);
}
