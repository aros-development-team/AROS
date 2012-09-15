/* This file was automatically imported with 
   import_gcry.py. Please don't modify it */
#include <grub/dl.h>
GRUB_MOD_LICENSE ("GPLv3+");
/* camellia-glue.c - Glue for the Camellia cipher
 * Copyright (C) 2007 Free Software Foundation, Inc.
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/* I put all the libgcrypt-specific stuff in this file to keep the
   camellia.c/camellia.h files exactly as provided by NTT.  If they
   update their code, this should make it easier to bring the changes
   in. - dshaw

   There is one small change which needs to be done: Include the
   following code at the top of camellia.h: */
#if 0

/* To use Camellia with libraries it is often useful to keep the name
 * space of the library clean.  The following macro is thus useful:
 *
 *     #define CAMELLIA_EXT_SYM_PREFIX foo_
 *  
 * This prefixes all external symbols with "foo_".
 */
#ifdef HAVE_CONFIG_H
#endif
#ifdef CAMELLIA_EXT_SYM_PREFIX
#define CAMELLIA_PREFIX1(x,y) x ## y
#define CAMELLIA_PREFIX2(x,y) CAMELLIA_PREFIX1(x,y)
#define CAMELLIA_PREFIX(x)    CAMELLIA_PREFIX2(CAMELLIA_EXT_SYM_PREFIX,x)
#define Camellia_Ekeygen      CAMELLIA_PREFIX(Camellia_Ekeygen)
#define Camellia_EncryptBlock CAMELLIA_PREFIX(Camellia_EncryptBlock)
#define Camellia_DecryptBlock CAMELLIA_PREFIX(Camellia_DecryptBlock)
#define camellia_decrypt128   CAMELLIA_PREFIX(camellia_decrypt128)
#define camellia_decrypt256   CAMELLIA_PREFIX(camellia_decrypt256)
#define camellia_encrypt128   CAMELLIA_PREFIX(camellia_encrypt128)
#define camellia_encrypt256   CAMELLIA_PREFIX(camellia_encrypt256)
#define camellia_setup128     CAMELLIA_PREFIX(camellia_setup128)
#define camellia_setup192     CAMELLIA_PREFIX(camellia_setup192) 
#define camellia_setup256     CAMELLIA_PREFIX(camellia_setup256)
#endif /*CAMELLIA_EXT_SYM_PREFIX*/

#endif /* Code sample. */


#include "types.h"
#include "g10lib.h"
#include "cipher.h"
#include "camellia.h"

typedef struct
{
  int keybitlength;
  KEY_TABLE_TYPE keytable;
} CAMELLIA_context;


static gcry_err_code_t
camellia_setkey(void *c, const byte *key, unsigned keylen)
{
  CAMELLIA_context *ctx=c;
  static int initialized=0;
  static const char *selftest_failed=NULL;

  if(keylen!=16 && keylen!=24 && keylen!=32)
    return GPG_ERR_INV_KEYLEN;

  if(!initialized)
    {
      initialized=1;
      selftest_failed=selftest();
      if(selftest_failed)
	log_error("%s\n",selftest_failed);
    }

  if(selftest_failed)
    return GPG_ERR_SELFTEST_FAILED;

  ctx->keybitlength=keylen*8;
  Camellia_Ekeygen(ctx->keybitlength,key,ctx->keytable);
  _gcry_burn_stack
    ((19+34+34)*sizeof(u32)+2*sizeof(void*) /* camellia_setup256 */
     +(4+32)*sizeof(u32)+2*sizeof(void*)    /* camellia_setup192 */
     +0+sizeof(int)+2*sizeof(void*)         /* Camellia_Ekeygen */
     +3*2*sizeof(void*)                     /* Function calls.  */
     );  

  return 0;
}

static void
camellia_encrypt(void *c, byte *outbuf, const byte *inbuf)
{
  CAMELLIA_context *ctx=c;

  Camellia_EncryptBlock(ctx->keybitlength,inbuf,ctx->keytable,outbuf);
  _gcry_burn_stack
    (sizeof(int)+2*sizeof(unsigned char *)+sizeof(KEY_TABLE_TYPE)
     +4*sizeof(u32)
     +2*sizeof(u32*)+4*sizeof(u32)
     +2*2*sizeof(void*) /* Function calls.  */
    );
}

static void
camellia_decrypt(void *c, byte *outbuf, const byte *inbuf)
{
  CAMELLIA_context *ctx=c;

  Camellia_DecryptBlock(ctx->keybitlength,inbuf,ctx->keytable,outbuf);
  _gcry_burn_stack
    (sizeof(int)+2*sizeof(unsigned char *)+sizeof(KEY_TABLE_TYPE)
     +4*sizeof(u32)
     +2*sizeof(u32*)+4*sizeof(u32)
     +2*2*sizeof(void*) /* Function calls.  */
    );
}


/* These oids are from
   <http://info.isl.ntt.co.jp/crypt/eng/camellia/specifications_oid.html>,
   retrieved May 1, 2007. */

static gcry_cipher_oid_spec_t camellia128_oids[] =
  {
    {"1.2.392.200011.61.1.1.1.2", GCRY_CIPHER_MODE_CBC},
    {"0.3.4401.5.3.1.9.1", GCRY_CIPHER_MODE_ECB},
    {"0.3.4401.5.3.1.9.3", GCRY_CIPHER_MODE_OFB},
    {"0.3.4401.5.3.1.9.4", GCRY_CIPHER_MODE_CFB},
    { NULL }
  };

static gcry_cipher_oid_spec_t camellia192_oids[] =
  {
    {"1.2.392.200011.61.1.1.1.3", GCRY_CIPHER_MODE_CBC},
    {"0.3.4401.5.3.1.9.21", GCRY_CIPHER_MODE_ECB},
    {"0.3.4401.5.3.1.9.23", GCRY_CIPHER_MODE_OFB},
    {"0.3.4401.5.3.1.9.24", GCRY_CIPHER_MODE_CFB},
    { NULL }
  };

static gcry_cipher_oid_spec_t camellia256_oids[] =
  {
    {"1.2.392.200011.61.1.1.1.4", GCRY_CIPHER_MODE_CBC},
    {"0.3.4401.5.3.1.9.41", GCRY_CIPHER_MODE_ECB},
    {"0.3.4401.5.3.1.9.43", GCRY_CIPHER_MODE_OFB},
    {"0.3.4401.5.3.1.9.44", GCRY_CIPHER_MODE_CFB},
    { NULL }
  };

gcry_cipher_spec_t _gcry_cipher_spec_camellia128 =
  {
    "CAMELLIA128",NULL,camellia128_oids,CAMELLIA_BLOCK_SIZE,128,
    sizeof(CAMELLIA_context),camellia_setkey,camellia_encrypt,camellia_decrypt
    ,
#ifdef GRUB_UTIL
    .modname = "gcry_camellia",
#endif
  };

gcry_cipher_spec_t _gcry_cipher_spec_camellia192 =
  {
    "CAMELLIA192",NULL,camellia192_oids,CAMELLIA_BLOCK_SIZE,192,
    sizeof(CAMELLIA_context),camellia_setkey,camellia_encrypt,camellia_decrypt
    ,
#ifdef GRUB_UTIL
    .modname = "gcry_camellia",
#endif
  };

gcry_cipher_spec_t _gcry_cipher_spec_camellia256 =
  {
    "CAMELLIA256",NULL,camellia256_oids,CAMELLIA_BLOCK_SIZE,256,
    sizeof(CAMELLIA_context),camellia_setkey,camellia_encrypt,camellia_decrypt
    ,
#ifdef GRUB_UTIL
    .modname = "gcry_camellia",
#endif
  };


GRUB_MOD_INIT(gcry_camellia)
{
  grub_cipher_register (&_gcry_cipher_spec_camellia128);
  grub_cipher_register (&_gcry_cipher_spec_camellia192);
  grub_cipher_register (&_gcry_cipher_spec_camellia256);
}

GRUB_MOD_FINI(gcry_camellia)
{
  grub_cipher_unregister (&_gcry_cipher_spec_camellia128);
  grub_cipher_unregister (&_gcry_cipher_spec_camellia192);
  grub_cipher_unregister (&_gcry_cipher_spec_camellia256);
}
