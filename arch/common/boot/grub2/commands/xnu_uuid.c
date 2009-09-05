/* xnu_uuid.c - transform 64-bit serial number 
   to 128-bit uuid suitable for xnu. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1995,1996,1998,1999,2001,2002,
 *                2003, 2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/fs.h>
#include <grub/file.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/command.h>

struct tohash
{
  grub_uint8_t prefix[16];
  grub_uint64_t serial;
} __attribute__ ((packed));

/* This prefix is used by xnu and boot-132 to hash 
   together with volume serial. */
static grub_uint8_t hash_prefix[16] 
  = {0xB3, 0xE2, 0x0F, 0x39, 0xF2, 0x92, 0x11, 0xD6, 
     0x97, 0xA4, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC};

#define rol(x,n) ( ((x) << (n)) | ((x) >> (32-(n))) )
#define ror(x,n) ( ((x) >> (n)) | ((x) << (32-(n))) )

typedef struct {
  grub_uint32_t A,B,C,D;	  /* chaining variables */
  grub_uint32_t  nblocks;
  grub_uint8_t buf[64];
  int  count;
} MD5_CONTEXT;

static void
md5_init( void *context )
{
  MD5_CONTEXT *ctx = context;

  ctx->A = 0x67452301;
  ctx->B = 0xefcdab89;
  ctx->C = 0x98badcfe;
  ctx->D = 0x10325476;

  ctx->nblocks = 0;
  ctx->count = 0;
}

/* These are the four functions used in the four steps of the MD5 algorithm
   and defined in the RFC 1321.  The first function is a little bit optimized
   (as found in Colin Plumbs public domain implementation).  */
/* #define FF(b, c, d) ((b & c) | (~b & d)) */
#define FF(b, c, d) (d ^ (b & (c ^ d)))
#define FG(b, c, d) FF (d, b, c)
#define FH(b, c, d) (b ^ c ^ d)
#define FI(b, c, d) (c ^ (b | ~d))


/****************
 * transform n*64 grub_uint8_ts
 */
static void
transform ( MD5_CONTEXT *ctx, const unsigned char *data )
{
  grub_uint32_t correct_words[16];
  register grub_uint32_t A = ctx->A;
  register grub_uint32_t B = ctx->B;
  register grub_uint32_t C = ctx->C;
  register grub_uint32_t D = ctx->D;
  grub_uint32_t *cwp = correct_words;

#ifdef GRUB_CPU_WORDS_BIGENDIAN
  {
    int i;
    const grub_uint32_t *p = (const grub_uint32_t *) data;

    for (i = 0; i < 16; i++)
      correct_words[i] = grub_le_to_cpu32 (p[i]);
  }
#else
  grub_memcpy (correct_words, data, 64);
#endif

#define OP(a, b, c, d, s, T) \
  do			         	   \
  {					   \
    a += FF (b, c, d) + (*cwp++) + T;    \
    a = rol(a, s);			   \
    a += b;				   \
  }					   \
  while (0)

  /* Before we start, one word about the strange constants.
     They are defined in RFC 1321 as

     T[i] = (int) (4294967296.0 * fabs (sin (i))), i=1..64
     */

  /* Round 1.  */
  OP (A, B, C, D,  7, 0xd76aa478);
  OP (D, A, B, C, 12, 0xe8c7b756);
  OP (C, D, A, B, 17, 0x242070db);
  OP (B, C, D, A, 22, 0xc1bdceee);
  OP (A, B, C, D,  7, 0xf57c0faf);
  OP (D, A, B, C, 12, 0x4787c62a);
  OP (C, D, A, B, 17, 0xa8304613);
  OP (B, C, D, A, 22, 0xfd469501);
  OP (A, B, C, D,  7, 0x698098d8);
  OP (D, A, B, C, 12, 0x8b44f7af);
  OP (C, D, A, B, 17, 0xffff5bb1);
  OP (B, C, D, A, 22, 0x895cd7be);
  OP (A, B, C, D,  7, 0x6b901122);
  OP (D, A, B, C, 12, 0xfd987193);
  OP (C, D, A, B, 17, 0xa679438e);
  OP (B, C, D, A, 22, 0x49b40821);

#undef OP
#define OP(f, a, b, c, d, k, s, T)  \
  do								      \
  { 							      \
    a += f (b, c, d) + correct_words[k] + T;		      \
    a = rol(a, s);						      \
    a += b; 						      \
  } 							      \
  while (0)

  /* Round 2.  */
  OP (FG, A, B, C, D,  1,  5, 0xf61e2562);
  OP (FG, D, A, B, C,  6,  9, 0xc040b340);
  OP (FG, C, D, A, B, 11, 14, 0x265e5a51);
  OP (FG, B, C, D, A,  0, 20, 0xe9b6c7aa);
  OP (FG, A, B, C, D,  5,  5, 0xd62f105d);
  OP (FG, D, A, B, C, 10,  9, 0x02441453);
  OP (FG, C, D, A, B, 15, 14, 0xd8a1e681);
  OP (FG, B, C, D, A,  4, 20, 0xe7d3fbc8);
  OP (FG, A, B, C, D,  9,  5, 0x21e1cde6);
  OP (FG, D, A, B, C, 14,  9, 0xc33707d6);
  OP (FG, C, D, A, B,  3, 14, 0xf4d50d87);
  OP (FG, B, C, D, A,  8, 20, 0x455a14ed);
  OP (FG, A, B, C, D, 13,  5, 0xa9e3e905);
  OP (FG, D, A, B, C,  2,  9, 0xfcefa3f8);
  OP (FG, C, D, A, B,  7, 14, 0x676f02d9);
  OP (FG, B, C, D, A, 12, 20, 0x8d2a4c8a);

  /* Round 3.  */
  OP (FH, A, B, C, D,  5,  4, 0xfffa3942);
  OP (FH, D, A, B, C,  8, 11, 0x8771f681);
  OP (FH, C, D, A, B, 11, 16, 0x6d9d6122);
  OP (FH, B, C, D, A, 14, 23, 0xfde5380c);
  OP (FH, A, B, C, D,  1,  4, 0xa4beea44);
  OP (FH, D, A, B, C,  4, 11, 0x4bdecfa9);
  OP (FH, C, D, A, B,  7, 16, 0xf6bb4b60);
  OP (FH, B, C, D, A, 10, 23, 0xbebfbc70);
  OP (FH, A, B, C, D, 13,  4, 0x289b7ec6);
  OP (FH, D, A, B, C,  0, 11, 0xeaa127fa);
  OP (FH, C, D, A, B,  3, 16, 0xd4ef3085);
  OP (FH, B, C, D, A,  6, 23, 0x04881d05);
  OP (FH, A, B, C, D,  9,  4, 0xd9d4d039);
  OP (FH, D, A, B, C, 12, 11, 0xe6db99e5);
  OP (FH, C, D, A, B, 15, 16, 0x1fa27cf8);
  OP (FH, B, C, D, A,  2, 23, 0xc4ac5665);

  /* Round 4.  */
  OP (FI, A, B, C, D,  0,  6, 0xf4292244);
  OP (FI, D, A, B, C,  7, 10, 0x432aff97);
  OP (FI, C, D, A, B, 14, 15, 0xab9423a7);
  OP (FI, B, C, D, A,  5, 21, 0xfc93a039);
  OP (FI, A, B, C, D, 12,  6, 0x655b59c3);
  OP (FI, D, A, B, C,  3, 10, 0x8f0ccc92);
  OP (FI, C, D, A, B, 10, 15, 0xffeff47d);
  OP (FI, B, C, D, A,  1, 21, 0x85845dd1);
  OP (FI, A, B, C, D,  8,  6, 0x6fa87e4f);
  OP (FI, D, A, B, C, 15, 10, 0xfe2ce6e0);
  OP (FI, C, D, A, B,  6, 15, 0xa3014314);
  OP (FI, B, C, D, A, 13, 21, 0x4e0811a1);
  OP (FI, A, B, C, D,  4,  6, 0xf7537e82);
  OP (FI, D, A, B, C, 11, 10, 0xbd3af235);
  OP (FI, C, D, A, B,  2, 15, 0x2ad7d2bb);
  OP (FI, B, C, D, A,  9, 21, 0xeb86d391);

  /* Put checksum in context given as argument.  */
  ctx->A += A;
  ctx->B += B;
  ctx->C += C;
  ctx->D += D;
}

/* The routine updates the message-digest context to
 * account for the presence of each of the characters inBuf[0..inLen-1]
 * in the message whose digest is being computed.
 */
static void
md5_write( void *context, const void *inbuf_arg , grub_size_t inlen)
{
  const unsigned char *inbuf = inbuf_arg;
  MD5_CONTEXT *hd = context;

  if( hd->count == 64 )  /* flush the buffer */
  {
    transform( hd, hd->buf );
    //      _gcry_burn_stack (80+6*sizeof(void*));
    hd->count = 0;
    hd->nblocks++;
  }
  if( !inbuf )
    return;

  if( hd->count )
  {
    for( ; inlen && hd->count < 64; inlen-- )
      hd->buf[hd->count++] = *inbuf++;
    md5_write( hd, NULL, 0 );
    if( !inlen )
      return;
  }
  //  _gcry_burn_stack (80+6*sizeof(void*));

  while( inlen >= 64 ) 
  {
    transform( hd, inbuf );
    hd->count = 0;
    hd->nblocks++;
    inlen -= 64;
    inbuf += 64;
  }
  for( ; inlen && hd->count < 64; inlen-- )
    hd->buf[hd->count++] = *inbuf++;

}



/* The routine final terminates the message-digest computation and
 * ends with the desired message digest in mdContext->digest[0...15].
 * The handle is prepared for a new MD5 cycle.
 * Returns 16 grub_uint8_ts representing the digest.
 */
static void
md5_final( void *context)
{
  MD5_CONTEXT *hd = context;
  grub_uint32_t t, msb, lsb;
  grub_uint32_t *p;

  md5_write(hd, NULL, 0); /* flush */;

  t = hd->nblocks;
  /* multiply by 64 to make a grub_uint8_t count */
  lsb = t << 6;
  msb = t >> 26;
  /* add the count */
  t = lsb;
  if( (lsb += hd->count) < t )
    msb++;
  /* multiply by 8 to make a bit count */
  t = lsb;
  lsb <<= 3;
  msb <<= 3;
  msb |= t >> 29;

  if( hd->count < 56 )  /* enough room */
  {
    hd->buf[hd->count++] = 0x80; /* pad */
    while( hd->count < 56 )
      hd->buf[hd->count++] = 0;  /* pad */
  }
  else  /* need one extra block */
  {
    hd->buf[hd->count++] = 0x80; /* pad character */
    while( hd->count < 64 )
      hd->buf[hd->count++] = 0;
    md5_write(hd, NULL, 0);  /* flush */;
    grub_memset(hd->buf, 0, 56 ); /* fill next block with zeroes */
  }
  /* append the 64 bit count */
  hd->buf[56] = lsb	   ;
  hd->buf[57] = lsb >>  8;
  hd->buf[58] = lsb >> 16;
  hd->buf[59] = lsb >> 24;
  hd->buf[60] = msb	   ;
  hd->buf[61] = msb >>  8;
  hd->buf[62] = msb >> 16;
  hd->buf[63] = msb >> 24;
  transform( hd, hd->buf );
  //  _gcry_burn_stack (80+6*sizeof(void*));

  p = (grub_uint32_t *) hd->buf;
#define X(a) do { *p = grub_le_to_cpu32 (hd->a); p++; } while (0)
  X(A);
  X(B);
  X(C);
  X(D);
#undef X

}

/**
 * GRUB2 Crypto Interface
 * Written by Michael Gorven
 */
static grub_err_t
md5 (const char *in, grub_size_t insize, char *out)
{
  MD5_CONTEXT hd;

  md5_init (&hd);
  md5_write (&hd, in, insize);
  md5_final (&hd);
  grub_memcpy (out, hd.buf, 16);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_xnu_uuid (grub_command_t cmd __attribute__ ((unused)),
		   int argc, char **args)
{
  struct tohash hashme;
  grub_uint8_t xnu_uuid[16];
  char uuid_string[sizeof ("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx")];
  char *ptr;

  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "UUID required");

  hashme.serial = grub_cpu_to_be64 (grub_strtoull (args[0], 0, 16));
  grub_memcpy (hashme.prefix, hash_prefix, sizeof (hashme.prefix));

  md5 ((char *) &hashme, sizeof (hashme), (char *) xnu_uuid);
  xnu_uuid[6] = (xnu_uuid[6] & 0xf) | 0x30;
  xnu_uuid[8] = (xnu_uuid[8] & 0x3f) | 0x80;
  grub_sprintf (uuid_string,
		"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		(unsigned int) xnu_uuid[0], (unsigned int) xnu_uuid[1],
		(unsigned int) xnu_uuid[2], (unsigned int) xnu_uuid[3],
		(unsigned int) xnu_uuid[4], (unsigned int) xnu_uuid[5],
		(unsigned int) ((xnu_uuid[6] & 0xf) | 0x30),
		(unsigned int) xnu_uuid[7],
		(unsigned int) ((xnu_uuid[8] & 0x3f) | 0x80),
		(unsigned int) xnu_uuid[9],
		(unsigned int) xnu_uuid[10], (unsigned int) xnu_uuid[11],
		(unsigned int) xnu_uuid[12], (unsigned int) xnu_uuid[13],
		(unsigned int) xnu_uuid[14], (unsigned int) xnu_uuid[15]);
  for (ptr = uuid_string; *ptr; ptr++)
    *ptr = grub_toupper (*ptr);
  if (argc == 1)
    grub_printf ("%s", uuid_string);
  if (argc > 1)
    grub_env_set (args[1], uuid_string);

  return GRUB_ERR_NONE;
}

static grub_command_t cmd;


GRUB_MOD_INIT (xnu_uuid)
{
  cmd = grub_register_command ("xnu_uuid", grub_cmd_xnu_uuid,
			       "xnu_uuid GRUBUUID [VARNAME]",
			       "Transform 64-bit UUID to format "
			       "suitable for xnu.");
}

GRUB_MOD_FINI (xnu_uuid)
{
  grub_unregister_command (cmd);
}
