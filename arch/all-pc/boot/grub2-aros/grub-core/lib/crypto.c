/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2006
 *                2007, 2008, 2009  Free Software Foundation, Inc.
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

#include <grub/crypto.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/term.h>
#include <grub/dl.h>
#include <grub/i18n.h>

#ifdef GRUB_UTIL
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#endif

GRUB_MOD_LICENSE ("GPLv3+");

struct grub_crypto_hmac_handle
{
  const struct gcry_md_spec *md;
  void *ctx;
  void *opad;
};

static gcry_cipher_spec_t *grub_ciphers = NULL;
static gcry_md_spec_t *grub_digests = NULL;

void (*grub_crypto_autoload_hook) (const char *name) = NULL;

/* Based on libgcrypt-1.4.4/src/misc.c.  */
void
grub_burn_stack (grub_size_t size)
{
  char buf[64];

  grub_memset (buf, 0, sizeof (buf));
  if (size > sizeof (buf))
    grub_burn_stack (size - sizeof (buf));
}


void 
grub_cipher_register (gcry_cipher_spec_t *cipher)
{
  cipher->next = grub_ciphers;
  grub_ciphers = cipher;
}

void
grub_cipher_unregister (gcry_cipher_spec_t *cipher)
{
  gcry_cipher_spec_t **ciph;
  for (ciph = &grub_ciphers; *ciph; ciph = &((*ciph)->next))
    if (*ciph == cipher)
      {
	*ciph = (*ciph)->next;
	break;
      }
}

void 
grub_md_register (gcry_md_spec_t *digest)
{
  digest->next = grub_digests;
  grub_digests = digest;
}

void 
grub_md_unregister (gcry_md_spec_t *cipher)
{
  gcry_md_spec_t **ciph;
  for (ciph = &grub_digests; *ciph; ciph = &((*ciph)->next))
    if (*ciph == cipher)
      {
	*ciph = (*ciph)->next;
	break;
      }
}

void
grub_crypto_hash (const gcry_md_spec_t *hash, void *out, const void *in,
		  grub_size_t inlen)
{
  grub_uint8_t ctx[hash->contextsize];
  hash->init (&ctx);
  hash->write (&ctx, in, inlen);
  hash->final (&ctx);
  grub_memcpy (out, hash->read (&ctx), hash->mdlen);
}

const gcry_md_spec_t *
grub_crypto_lookup_md_by_name (const char *name)
{
  const gcry_md_spec_t *md;
  int first = 1;
  while (1)
    {
      for (md = grub_digests; md; md = md->next)
	if (grub_strcasecmp (name, md->name) == 0)
	  return md;
      if (grub_crypto_autoload_hook && first)
	grub_crypto_autoload_hook (name);
      else
	return NULL;
      first = 0;
    }
}

const gcry_cipher_spec_t *
grub_crypto_lookup_cipher_by_name (const char *name)
{
  const gcry_cipher_spec_t *ciph;
  int first = 1;
  while (1)
    {
      for (ciph = grub_ciphers; ciph; ciph = ciph->next)
	{
	  const char **alias;
	  if (grub_strcasecmp (name, ciph->name) == 0)
	    return ciph;
	  if (!ciph->aliases)
	    continue;
	  for (alias = ciph->aliases; *alias; alias++)
	    if (grub_strcasecmp (name, *alias) == 0)
	      return ciph;
	}
      if (grub_crypto_autoload_hook && first)
	grub_crypto_autoload_hook (name);
      else
	return NULL;
      first = 0;
    }
}


grub_crypto_cipher_handle_t
grub_crypto_cipher_open (const struct gcry_cipher_spec *cipher)
{
  grub_crypto_cipher_handle_t ret;
  ret = grub_malloc (sizeof (*ret) + cipher->contextsize);
  if (!ret)
    return NULL;
  ret->cipher = cipher;
  return ret;
}

gcry_err_code_t
grub_crypto_cipher_set_key (grub_crypto_cipher_handle_t cipher,
			    const unsigned char *key,
			    unsigned keylen)
{
  return cipher->cipher->setkey (cipher->ctx, key, keylen);
}

gcry_err_code_t
grub_crypto_ecb_decrypt (grub_crypto_cipher_handle_t cipher,
			 void *out, const void *in, grub_size_t size)
{
  const grub_uint8_t *inptr;
  grub_uint8_t *outptr, *end;
  if (!cipher->cipher->decrypt)
    return GPG_ERR_NOT_SUPPORTED;
  if (size % cipher->cipher->blocksize != 0)
    return GPG_ERR_INV_ARG;
  end = (grub_uint8_t *) in + size;
  for (inptr = in, outptr = out; inptr < end;
       inptr += cipher->cipher->blocksize, outptr += cipher->cipher->blocksize)
    cipher->cipher->decrypt (cipher->ctx, outptr, inptr);
  return GPG_ERR_NO_ERROR;
}

gcry_err_code_t
grub_crypto_ecb_encrypt (grub_crypto_cipher_handle_t cipher,
			 void *out, const void *in, grub_size_t size)
{
  const grub_uint8_t *inptr;
  grub_uint8_t *outptr, *end;
  if (!cipher->cipher->encrypt)
    return GPG_ERR_NOT_SUPPORTED;
  if (size % cipher->cipher->blocksize != 0)
    return GPG_ERR_INV_ARG;
  end = (grub_uint8_t *) in + size;
  for (inptr = in, outptr = out; inptr < end;
       inptr += cipher->cipher->blocksize, outptr += cipher->cipher->blocksize)
    cipher->cipher->encrypt (cipher->ctx, outptr, inptr);
  return GPG_ERR_NO_ERROR;
}

gcry_err_code_t
grub_crypto_cbc_encrypt (grub_crypto_cipher_handle_t cipher,
			 void *out, void *in, grub_size_t size,
			 void *iv_in)
{
  grub_uint8_t *inptr, *outptr, *end;
  void *iv;
  if (!cipher->cipher->decrypt)
    return GPG_ERR_NOT_SUPPORTED;
  if (size % cipher->cipher->blocksize != 0)
    return GPG_ERR_INV_ARG;
  end = (grub_uint8_t *) in + size;
  iv = iv_in;
  for (inptr = in, outptr = out; inptr < end;
       inptr += cipher->cipher->blocksize, outptr += cipher->cipher->blocksize)
    {
      grub_crypto_xor (outptr, inptr, iv, cipher->cipher->blocksize);
      cipher->cipher->encrypt (cipher->ctx, outptr, outptr);
      iv = outptr;
    }
  grub_memcpy (iv_in, iv, cipher->cipher->blocksize);
  return GPG_ERR_NO_ERROR;
}

gcry_err_code_t
grub_crypto_cbc_decrypt (grub_crypto_cipher_handle_t cipher,
			 void *out, const void *in, grub_size_t size,
			 void *iv)
{
  const grub_uint8_t *inptr;
  grub_uint8_t *outptr, *end;
  grub_uint8_t ivt[cipher->cipher->blocksize];
  if (!cipher->cipher->decrypt)
    return GPG_ERR_NOT_SUPPORTED;
  if (size % cipher->cipher->blocksize != 0)
    return GPG_ERR_INV_ARG;
  end = (grub_uint8_t *) in + size;
  for (inptr = in, outptr = out; inptr < end;
       inptr += cipher->cipher->blocksize, outptr += cipher->cipher->blocksize)
    {
      grub_memcpy (ivt, inptr, cipher->cipher->blocksize);
      cipher->cipher->decrypt (cipher->ctx, outptr, inptr);
      grub_crypto_xor (outptr, outptr, iv, cipher->cipher->blocksize);
      grub_memcpy (iv, ivt, cipher->cipher->blocksize);
    }
  return GPG_ERR_NO_ERROR;
}

/* Based on gcry/cipher/md.c.  */
struct grub_crypto_hmac_handle *
grub_crypto_hmac_init (const struct gcry_md_spec *md,
		       const void *key, grub_size_t keylen)
{
  grub_uint8_t *helpkey = NULL;
  grub_uint8_t *ipad = NULL, *opad = NULL;
  void *ctx = NULL;
  struct grub_crypto_hmac_handle *ret = NULL;
  unsigned i;

  if (md->mdlen > md->blocksize)
    return NULL;

  ctx = grub_malloc (md->contextsize);
  if (!ctx)
    goto err;

  if ( keylen > md->blocksize ) 
    {
      helpkey = grub_malloc (md->mdlen);
      if (!helpkey)
	goto err;
      grub_crypto_hash (md, helpkey, key, keylen);

      key = helpkey;
      keylen = md->mdlen;
    }

  ipad = grub_zalloc (md->blocksize);
  if (!ipad)
    goto err;

  opad = grub_zalloc (md->blocksize);
  if (!opad)
    goto err;

  grub_memcpy ( ipad, key, keylen );
  grub_memcpy ( opad, key, keylen );
  for (i=0; i < md->blocksize; i++ ) 
    {
      ipad[i] ^= 0x36;
      opad[i] ^= 0x5c;
    }
  grub_free (helpkey);
  helpkey = NULL;

  md->init (ctx);

  md->write (ctx, ipad, md->blocksize); /* inner pad */
  grub_memset (ipad, 0, md->blocksize);
  grub_free (ipad);
  ipad = NULL;

  ret = grub_malloc (sizeof (*ret));
  if (!ret)
    goto err;

  ret->md = md;
  ret->ctx = ctx;
  ret->opad = opad;

  return ret;

 err:
  grub_free (helpkey);
  grub_free (ctx);
  grub_free (ipad);
  grub_free (opad);
  return NULL;
}

void
grub_crypto_hmac_write (struct grub_crypto_hmac_handle *hnd,
			const void *data,
			grub_size_t datalen)
{
  hnd->md->write (hnd->ctx, data, datalen);
}

gcry_err_code_t
grub_crypto_hmac_fini (struct grub_crypto_hmac_handle *hnd, void *out)
{
  grub_uint8_t *p;
  grub_uint8_t *ctx2;

  ctx2 = grub_malloc (hnd->md->contextsize);
  if (!ctx2)
    return GPG_ERR_OUT_OF_MEMORY;

  hnd->md->final (hnd->ctx);
  hnd->md->read (hnd->ctx);
  p = hnd->md->read (hnd->ctx);

  hnd->md->init (ctx2);
  hnd->md->write (ctx2, hnd->opad, hnd->md->blocksize);
  hnd->md->write (ctx2, p, hnd->md->mdlen);
  hnd->md->final (ctx2);
  grub_memset (hnd->opad, 0, hnd->md->blocksize);
  grub_free (hnd->opad);
  grub_memset (hnd->ctx, 0, hnd->md->contextsize);
  grub_free (hnd->ctx);

  grub_memcpy (out, hnd->md->read (ctx2), hnd->md->mdlen);
  grub_memset (ctx2, 0, hnd->md->contextsize);
  grub_free (ctx2);

  grub_memset (hnd, 0, sizeof (*hnd));
  grub_free (hnd);

  return GPG_ERR_NO_ERROR;
}

gcry_err_code_t
grub_crypto_hmac_buffer (const struct gcry_md_spec *md,
			 const void *key, grub_size_t keylen,
			 const void *data, grub_size_t datalen, void *out)
{
  struct grub_crypto_hmac_handle *hnd;

  hnd = grub_crypto_hmac_init (md, key, keylen);
  if (!hnd)
    return GPG_ERR_OUT_OF_MEMORY;

  grub_crypto_hmac_write (hnd, data, datalen);
  return grub_crypto_hmac_fini (hnd, out);
}


grub_err_t
grub_crypto_gcry_error (gcry_err_code_t in)
{
  if (in == GPG_ERR_NO_ERROR)
    return GRUB_ERR_NONE;
  return GRUB_ACCESS_DENIED;
}

int
grub_crypto_memcmp (const void *a, const void *b, grub_size_t n)
{
  register grub_size_t counter = 0;
  const grub_uint8_t *pa, *pb;

  for (pa = a, pb = b; n; pa++, pb++, n--)
    {
      if (*pa != *pb)
	counter++;
    }

  return !!counter;
}

int
grub_password_get (char buf[], unsigned buf_size)
{
#ifdef GRUB_UTIL
  FILE *in;
  struct termios s, t;
  int tty_changed = 0;
  char *ptr;

  /* Disable echoing. Based on glibc.  */
  in = fopen ("/dev/tty", "w+c");
  if (in == NULL)
    in = stdin;

  if (tcgetattr (fileno (in), &t) == 0)
    {
      /* Save the old one. */
      s = t;
      /* Tricky, tricky. */
      t.c_lflag &= ~(ECHO|ISIG);
      tty_changed = (tcsetattr (fileno (in), TCSAFLUSH, &t) == 0);
    }
  else
    tty_changed = 0;
  fgets (buf, buf_size, stdin);
  ptr = buf + strlen (buf) - 1;
  while (buf <= ptr && (*ptr == '\n' || *ptr == '\r'))
    *ptr-- = 0;
  /* Restore the original setting.  */
  if (tty_changed)
    (void) tcsetattr (fileno (in), TCSAFLUSH, &s);

  grub_xputs ("\n");
  grub_refresh ();

  return 1;
#else
  unsigned cur_len = 0;
  int key;

  while (1)
    {
      key = grub_getkey (); 
      if (key == '\n' || key == '\r')
	break;

      if (key == '\e')
	{
	  cur_len = 0;
	  break;
	}

      if (key == '\b')
	{
	  cur_len--;
	  continue;
	}

      if (!grub_isprint (key))
	continue;

      if (cur_len + 2 < buf_size)
	buf[cur_len++] = key;
    }

  grub_memset (buf + cur_len, 0, buf_size - cur_len);

  grub_xputs ("\n");
  grub_refresh ();

  return (key != '\e');
#endif
}
