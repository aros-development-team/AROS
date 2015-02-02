/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2007,2010,2011  Free Software Foundation, Inc.
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

#include <grub/cryptodisk.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/dl.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>
#include <grub/fs.h>
#include <grub/file.h>
#include <grub/procfs.h>

#ifdef GRUB_UTIL
#include <grub/emu/hostdisk.h>
#endif

GRUB_MOD_LICENSE ("GPLv3+");

grub_cryptodisk_dev_t grub_cryptodisk_list;

static const struct grub_arg_option options[] =
  {
    {"uuid", 'u', 0, N_("Mount by UUID."), 0, 0},
    /* TRANSLATORS: It's still restricted to cryptodisks only.  */
    {"all", 'a', 0, N_("Mount all."), 0, 0},
    {"boot", 'b', 0, N_("Mount all volumes with `boot' flag set."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

/* Our irreducible polynom is x^128+x^7+x^2+x+1. Lowest byte of it is:  */
#define GF_POLYNOM 0x87
static inline int GF_PER_SECTOR (const struct grub_cryptodisk *dev)
{
  return 1U << (dev->log_sector_size - GRUB_CRYPTODISK_GF_LOG_BYTES);
}

static grub_cryptodisk_t cryptodisk_list = NULL;
static grub_uint8_t last_cryptodisk_id = 0;

static void
gf_mul_x (grub_uint8_t *g)
{
  int over = 0, over2 = 0;
  unsigned j;

  for (j = 0; j < GRUB_CRYPTODISK_GF_BYTES; j++)
    {
      over2 = !!(g[j] & 0x80);
      g[j] <<= 1;
      g[j] |= over;
      over = over2;
    }
  if (over)
    g[0] ^= GF_POLYNOM;
}


static void
gf_mul_x_be (grub_uint8_t *g)
{
  int over = 0, over2 = 0;
  int j;

  for (j = (int) GRUB_CRYPTODISK_GF_BYTES - 1; j >= 0; j--)
    {
      over2 = !!(g[j] & 0x80);
      g[j] <<= 1;
      g[j] |= over;
      over = over2;
    }
  if (over)
    g[GRUB_CRYPTODISK_GF_BYTES - 1] ^= GF_POLYNOM;
}

static void
gf_mul_be (grub_uint8_t *o, const grub_uint8_t *a, const grub_uint8_t *b)
{
  unsigned i;
  grub_uint8_t t[GRUB_CRYPTODISK_GF_BYTES];
  grub_memset (o, 0, GRUB_CRYPTODISK_GF_BYTES);
  grub_memcpy (t, b, GRUB_CRYPTODISK_GF_BYTES);
  for (i = 0; i < GRUB_CRYPTODISK_GF_SIZE; i++)
    {
      if (((a[GRUB_CRYPTODISK_GF_BYTES - i / GRUB_CHAR_BIT - 1] >> (i % GRUB_CHAR_BIT))) & 1)
	grub_crypto_xor (o, o, t, GRUB_CRYPTODISK_GF_BYTES);
      gf_mul_x_be (t);
    }
}

static gcry_err_code_t
grub_crypto_pcbc_decrypt (grub_crypto_cipher_handle_t cipher,
			 void *out, void *in, grub_size_t size,
			 void *iv)
{
  grub_uint8_t *inptr, *outptr, *end;
  grub_uint8_t ivt[GRUB_CRYPTO_MAX_CIPHER_BLOCKSIZE];
  if (cipher->cipher->blocksize > GRUB_CRYPTO_MAX_CIPHER_BLOCKSIZE)
    return GPG_ERR_INV_ARG;
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
      grub_crypto_xor (iv, ivt, outptr, cipher->cipher->blocksize);
    }
  return GPG_ERR_NO_ERROR;
}

static gcry_err_code_t
grub_crypto_pcbc_encrypt (grub_crypto_cipher_handle_t cipher,
			  void *out, void *in, grub_size_t size,
			  void *iv)
{
  grub_uint8_t *inptr, *outptr, *end;
  grub_uint8_t ivt[GRUB_CRYPTO_MAX_CIPHER_BLOCKSIZE];
  if (cipher->cipher->blocksize > GRUB_CRYPTO_MAX_CIPHER_BLOCKSIZE)
    return GPG_ERR_INV_ARG;
  if (!cipher->cipher->decrypt)
    return GPG_ERR_NOT_SUPPORTED;
  if (size % cipher->cipher->blocksize != 0)
    return GPG_ERR_INV_ARG;
  end = (grub_uint8_t *) in + size;
  for (inptr = in, outptr = out; inptr < end;
       inptr += cipher->cipher->blocksize, outptr += cipher->cipher->blocksize)
    {
      grub_memcpy (ivt, inptr, cipher->cipher->blocksize);
      grub_crypto_xor (outptr, outptr, iv, cipher->cipher->blocksize);
      cipher->cipher->encrypt (cipher->ctx, outptr, inptr);
      grub_crypto_xor (iv, ivt, outptr, cipher->cipher->blocksize);
    }
  return GPG_ERR_NO_ERROR;
}

struct lrw_sector
{
  grub_uint8_t low[GRUB_CRYPTODISK_GF_BYTES];
  grub_uint8_t high[GRUB_CRYPTODISK_GF_BYTES];
  grub_uint8_t low_byte, low_byte_c;
};

static void
generate_lrw_sector (struct lrw_sector *sec,
		     const struct grub_cryptodisk *dev,
		     const grub_uint8_t *iv)
{
  grub_uint8_t idx[GRUB_CRYPTODISK_GF_BYTES];
  grub_uint16_t c;
  int j;
  grub_memcpy (idx, iv, GRUB_CRYPTODISK_GF_BYTES);
  sec->low_byte = (idx[GRUB_CRYPTODISK_GF_BYTES - 1]
		   & (GF_PER_SECTOR (dev) - 1));
  sec->low_byte_c = (((GF_PER_SECTOR (dev) - 1) & ~sec->low_byte) + 1);
  idx[GRUB_CRYPTODISK_GF_BYTES - 1] &= ~(GF_PER_SECTOR (dev) - 1);
  gf_mul_be (sec->low, dev->lrw_key, idx);
  if (!sec->low_byte)
    return;

  c = idx[GRUB_CRYPTODISK_GF_BYTES - 1] + GF_PER_SECTOR (dev);
  if (c & 0x100)
    {
      for (j = GRUB_CRYPTODISK_GF_BYTES - 2; j >= 0; j--)
	{
	  idx[j]++;
	  if (idx[j] != 0)
	    break;
	}
    }
  idx[GRUB_CRYPTODISK_GF_BYTES - 1] = c;
  gf_mul_be (sec->high, dev->lrw_key, idx);
}

static void __attribute__ ((unused))
lrw_xor (const struct lrw_sector *sec,
	 const struct grub_cryptodisk *dev,
	 grub_uint8_t *b)
{
  unsigned i;

  for (i = 0; i < sec->low_byte_c * GRUB_CRYPTODISK_GF_BYTES;
       i += GRUB_CRYPTODISK_GF_BYTES)
    grub_crypto_xor (b + i, b + i, sec->low, GRUB_CRYPTODISK_GF_BYTES);
  grub_crypto_xor (b, b, dev->lrw_precalc + GRUB_CRYPTODISK_GF_BYTES * sec->low_byte,
		   sec->low_byte_c * GRUB_CRYPTODISK_GF_BYTES);
  if (!sec->low_byte)
    return;

  for (i = sec->low_byte_c * GRUB_CRYPTODISK_GF_BYTES;
       i < (1U << dev->log_sector_size); i += GRUB_CRYPTODISK_GF_BYTES)
    grub_crypto_xor (b + i, b + i, sec->high, GRUB_CRYPTODISK_GF_BYTES);
  grub_crypto_xor (b + sec->low_byte_c * GRUB_CRYPTODISK_GF_BYTES,
		   b + sec->low_byte_c * GRUB_CRYPTODISK_GF_BYTES,
		   dev->lrw_precalc, sec->low_byte * GRUB_CRYPTODISK_GF_BYTES);
}

static gcry_err_code_t
grub_cryptodisk_endecrypt (struct grub_cryptodisk *dev,
			   grub_uint8_t * data, grub_size_t len,
			   grub_disk_addr_t sector, int do_encrypt)
{
  grub_size_t i;
  gcry_err_code_t err;

  if (dev->cipher->cipher->blocksize > GRUB_CRYPTO_MAX_CIPHER_BLOCKSIZE)
    return GPG_ERR_INV_ARG;

  /* The only mode without IV.  */
  if (dev->mode == GRUB_CRYPTODISK_MODE_ECB && !dev->rekey)
    return (do_encrypt ? grub_crypto_ecb_encrypt (dev->cipher, data, data, len)
	    : grub_crypto_ecb_decrypt (dev->cipher, data, data, len));

  for (i = 0; i < len; i += (1U << dev->log_sector_size))
    {
      grub_size_t sz = ((dev->cipher->cipher->blocksize
			 + sizeof (grub_uint32_t) - 1)
			/ sizeof (grub_uint32_t));
      grub_uint32_t iv[(GRUB_CRYPTO_MAX_CIPHER_BLOCKSIZE + 3) / 4];

      if (dev->rekey)
	{
	  grub_uint64_t zone = sector >> dev->rekey_shift;
	  if (zone != dev->last_rekey)
	    {
	      err = dev->rekey (dev, zone);
	      if (err)
		return err;
	      dev->last_rekey = zone;
	    }
	}

      grub_memset (iv, 0, sizeof (iv));
      switch (dev->mode_iv)
	{
	case GRUB_CRYPTODISK_MODE_IV_NULL:
	  break;
	case GRUB_CRYPTODISK_MODE_IV_BYTECOUNT64_HASH:
	  {
	    grub_uint64_t tmp;
	    void *ctx;

	    ctx = grub_zalloc (dev->iv_hash->contextsize);
	    if (!ctx)
	      return GPG_ERR_OUT_OF_MEMORY;

	    tmp = grub_cpu_to_le64 (sector << dev->log_sector_size);
	    dev->iv_hash->init (ctx);
	    dev->iv_hash->write (ctx, dev->iv_prefix, dev->iv_prefix_len);
	    dev->iv_hash->write (ctx, &tmp, sizeof (tmp));
	    dev->iv_hash->final (ctx);

	    grub_memcpy (iv, dev->iv_hash->read (ctx), sizeof (iv));
	    grub_free (ctx);
	  }
	  break;
	case GRUB_CRYPTODISK_MODE_IV_PLAIN64:
	  iv[1] = grub_cpu_to_le32 (sector >> 32);
	case GRUB_CRYPTODISK_MODE_IV_PLAIN:
	  iv[0] = grub_cpu_to_le32 (sector & 0xFFFFFFFF);
	  break;
	case GRUB_CRYPTODISK_MODE_IV_BYTECOUNT64:
	  iv[1] = grub_cpu_to_le32 (sector >> (32 - dev->log_sector_size));
	  iv[0] = grub_cpu_to_le32 ((sector << dev->log_sector_size)
				    & 0xFFFFFFFF);
	  break;
	case GRUB_CRYPTODISK_MODE_IV_BENBI:
	  {
	    grub_uint64_t num = (sector << dev->benbi_log) + 1;
	    iv[sz - 2] = grub_cpu_to_be32 (num >> 32);
	    iv[sz - 1] = grub_cpu_to_be32 (num & 0xFFFFFFFF);
	  }
	  break;
	case GRUB_CRYPTODISK_MODE_IV_ESSIV:
	  iv[0] = grub_cpu_to_le32 (sector & 0xFFFFFFFF);
	  err = grub_crypto_ecb_encrypt (dev->essiv_cipher, iv, iv,
					 dev->cipher->cipher->blocksize);
	  if (err)
	    return err;
	}

      switch (dev->mode)
	{
	case GRUB_CRYPTODISK_MODE_CBC:
	  if (do_encrypt)
	    err = grub_crypto_cbc_encrypt (dev->cipher, data + i, data + i,
					   (1U << dev->log_sector_size), iv);
	  else
	    err = grub_crypto_cbc_decrypt (dev->cipher, data + i, data + i,
					   (1U << dev->log_sector_size), iv);
	  if (err)
	    return err;
	  break;

	case GRUB_CRYPTODISK_MODE_PCBC:
	  if (do_encrypt)
	    err = grub_crypto_pcbc_encrypt (dev->cipher, data + i, data + i,
					    (1U << dev->log_sector_size), iv);
	  else
	    err = grub_crypto_pcbc_decrypt (dev->cipher, data + i, data + i,
					    (1U << dev->log_sector_size), iv);
	  if (err)
	    return err;
	  break;
	case GRUB_CRYPTODISK_MODE_XTS:
	  {
	    unsigned j;
	    err = grub_crypto_ecb_encrypt (dev->secondary_cipher, iv, iv,
					   dev->cipher->cipher->blocksize);
	    if (err)
	      return err;
	    
	    for (j = 0; j < (1U << dev->log_sector_size);
		 j += dev->cipher->cipher->blocksize)
	      {
		grub_crypto_xor (data + i + j, data + i + j, iv,
				 dev->cipher->cipher->blocksize);
		if (do_encrypt)
		  err = grub_crypto_ecb_encrypt (dev->cipher, data + i + j, 
						 data + i + j,
						 dev->cipher->cipher->blocksize);
		else
		  err = grub_crypto_ecb_decrypt (dev->cipher, data + i + j, 
						 data + i + j,
						 dev->cipher->cipher->blocksize);
		if (err)
		  return err;
		grub_crypto_xor (data + i + j, data + i + j, iv,
				 dev->cipher->cipher->blocksize);
		gf_mul_x ((grub_uint8_t *) iv);
	      }
	  }
	  break;
	case GRUB_CRYPTODISK_MODE_LRW:
	  {
	    struct lrw_sector sec;

	    generate_lrw_sector (&sec, dev, (grub_uint8_t *) iv);
	    lrw_xor (&sec, dev, data + i);

	    if (do_encrypt)
	      err = grub_crypto_ecb_encrypt (dev->cipher, data + i, 
					     data + i,
					     (1U << dev->log_sector_size));
	    else
	      err = grub_crypto_ecb_decrypt (dev->cipher, data + i, 
					     data + i,
					     (1U << dev->log_sector_size));
	    if (err)
	      return err;
	    lrw_xor (&sec, dev, data + i);
	  }
	  break;
	case GRUB_CRYPTODISK_MODE_ECB:
	  if (do_encrypt)
	    err = grub_crypto_ecb_encrypt (dev->cipher, data + i, data + i,
					   (1U << dev->log_sector_size));
	  else
	    err = grub_crypto_ecb_decrypt (dev->cipher, data + i, data + i,
					   (1U << dev->log_sector_size));
	  if (err)
	    return err;
	  break;
	default:
	  return GPG_ERR_NOT_IMPLEMENTED;
	}
      sector++;
    }
  return GPG_ERR_NO_ERROR;
}

gcry_err_code_t
grub_cryptodisk_decrypt (struct grub_cryptodisk *dev,
			 grub_uint8_t * data, grub_size_t len,
			 grub_disk_addr_t sector)
{
  return grub_cryptodisk_endecrypt (dev, data, len, sector, 0);
}

gcry_err_code_t
grub_cryptodisk_setkey (grub_cryptodisk_t dev, grub_uint8_t *key, grub_size_t keysize)
{
  gcry_err_code_t err;
  int real_keysize;

  real_keysize = keysize;
  if (dev->mode == GRUB_CRYPTODISK_MODE_XTS)
    real_keysize /= 2;
  if (dev->mode == GRUB_CRYPTODISK_MODE_LRW)
    real_keysize -= dev->cipher->cipher->blocksize;
	
  /* Set the PBKDF2 output as the cipher key.  */
  err = grub_crypto_cipher_set_key (dev->cipher, key, real_keysize);
  if (err)
    return err;
  grub_memcpy (dev->key, key, keysize);
  dev->keysize = keysize;

  /* Configure ESSIV if necessary.  */
  if (dev->mode_iv == GRUB_CRYPTODISK_MODE_IV_ESSIV)
    {
      grub_size_t essiv_keysize = dev->essiv_hash->mdlen;
      grub_uint8_t hashed_key[GRUB_CRYPTO_MAX_MDLEN];
      if (essiv_keysize > GRUB_CRYPTO_MAX_MDLEN)
	return GPG_ERR_INV_ARG;

      grub_crypto_hash (dev->essiv_hash, hashed_key, key, keysize);
      err = grub_crypto_cipher_set_key (dev->essiv_cipher,
					hashed_key, essiv_keysize);
      if (err)
	return err;
    }
  if (dev->mode == GRUB_CRYPTODISK_MODE_XTS)
    {
      err = grub_crypto_cipher_set_key (dev->secondary_cipher,
					key + real_keysize,
					keysize / 2);
      if (err)
	return err;
    }

  if (dev->mode == GRUB_CRYPTODISK_MODE_LRW)
    {
      unsigned i;
      grub_uint8_t idx[GRUB_CRYPTODISK_GF_BYTES];

      grub_free (dev->lrw_precalc);
      grub_memcpy (dev->lrw_key, key + real_keysize,
		   dev->cipher->cipher->blocksize);
      dev->lrw_precalc = grub_malloc ((1U << dev->log_sector_size));
      if (!dev->lrw_precalc)
	return GPG_ERR_OUT_OF_MEMORY;
      grub_memset (idx, 0, GRUB_CRYPTODISK_GF_BYTES);
      for (i = 0; i < (1U << dev->log_sector_size);
	   i += GRUB_CRYPTODISK_GF_BYTES)
	{
	  idx[GRUB_CRYPTODISK_GF_BYTES - 1] = i / GRUB_CRYPTODISK_GF_BYTES;
	  gf_mul_be (dev->lrw_precalc + i, idx, dev->lrw_key);
	}
    }
  return GPG_ERR_NO_ERROR;
}

static int
grub_cryptodisk_iterate (grub_disk_dev_iterate_hook_t hook, void *hook_data,
			 grub_disk_pull_t pull)
{
  grub_cryptodisk_t i;

  if (pull != GRUB_DISK_PULL_NONE)
    return 0;

  for (i = cryptodisk_list; i != NULL; i = i->next)
    {
      char buf[30];
      grub_snprintf (buf, sizeof (buf), "crypto%lu", i->id);
      if (hook (buf, hook_data))
	return 1;
    }

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cryptodisk_open (const char *name, grub_disk_t disk)
{
  grub_cryptodisk_t dev;

  if (grub_memcmp (name, "crypto", sizeof ("crypto") - 1) != 0)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "No such device");

  if (grub_memcmp (name, "cryptouuid/", sizeof ("cryptouuid/") - 1) == 0)
    {
      for (dev = cryptodisk_list; dev != NULL; dev = dev->next)
	if (grub_strcasecmp (name + sizeof ("cryptouuid/") - 1, dev->uuid) == 0)
	  break;
    }
  else
    {
      unsigned long id = grub_strtoul (name + sizeof ("crypto") - 1, 0, 0);
      if (grub_errno)
	return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "No such device");
      /* Search for requested device in the list of CRYPTODISK devices.  */
      for (dev = cryptodisk_list; dev != NULL; dev = dev->next)
	if (dev->id == id)
	  break;
    }
  if (!dev)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "No such device");

  disk->log_sector_size = dev->log_sector_size;

#ifdef GRUB_UTIL
  if (dev->cheat)
    {
      if (!GRUB_UTIL_FD_IS_VALID (dev->cheat_fd))
	dev->cheat_fd = grub_util_fd_open (dev->cheat, GRUB_UTIL_FD_O_RDONLY);
      if (!GRUB_UTIL_FD_IS_VALID (dev->cheat_fd))
	return grub_error (GRUB_ERR_IO, N_("cannot open `%s': %s"),
			   dev->cheat, grub_util_fd_strerror ());
    }
#endif

  if (!dev->source_disk)
    {
      grub_dprintf ("cryptodisk", "Opening device %s\n", name);
      /* Try to open the source disk and populate the requested disk.  */
      dev->source_disk = grub_disk_open (dev->source);
      if (!dev->source_disk)
	return grub_errno;
    }

  disk->data = dev;
  disk->total_sectors = dev->total_length;
  disk->max_agglomerate = GRUB_DISK_MAX_MAX_AGGLOMERATE;
  disk->id = dev->id;
  dev->ref++;
  return GRUB_ERR_NONE;
}

static void
grub_cryptodisk_close (grub_disk_t disk)
{
  grub_cryptodisk_t dev = (grub_cryptodisk_t) disk->data;
  grub_dprintf ("cryptodisk", "Closing disk\n");

  dev->ref--;

  if (dev->ref != 0)
    return;
#ifdef GRUB_UTIL
  if (dev->cheat)
    {
      grub_util_fd_close (dev->cheat_fd);
      dev->cheat_fd = GRUB_UTIL_FD_INVALID;
    }
#endif
  grub_disk_close (dev->source_disk);
  dev->source_disk = NULL;
}

static grub_err_t
grub_cryptodisk_read (grub_disk_t disk, grub_disk_addr_t sector,
		      grub_size_t size, char *buf)
{
  grub_cryptodisk_t dev = (grub_cryptodisk_t) disk->data;
  grub_err_t err;
  gcry_err_code_t gcry_err;

#ifdef GRUB_UTIL
  if (dev->cheat)
    {
      int r;
      r = grub_util_fd_seek (dev->cheat_fd, sector << disk->log_sector_size);
      if (r)
	return grub_error (GRUB_ERR_BAD_DEVICE, N_("cannot seek `%s': %s"),
			   dev->cheat, grub_util_fd_strerror ());
      if (grub_util_fd_read (dev->cheat_fd, buf, size << disk->log_sector_size)
	  != (ssize_t) (size << disk->log_sector_size))
	return grub_error (GRUB_ERR_READ_ERROR, N_("cannot read `%s': %s"),
			   dev->cheat, grub_util_fd_strerror ());
      return GRUB_ERR_NONE;
    }
#endif

  grub_dprintf ("cryptodisk",
		"Reading %" PRIuGRUB_SIZE " sectors from sector 0x%"
		PRIxGRUB_UINT64_T " with offset of %" PRIuGRUB_UINT64_T "\n",
		size, sector, dev->offset);

  err = grub_disk_read (dev->source_disk,
			(sector << (disk->log_sector_size
				   - GRUB_DISK_SECTOR_BITS)) + dev->offset, 0,
			size << disk->log_sector_size, buf);
  if (err)
    {
      grub_dprintf ("cryptodisk", "grub_disk_read failed with error %d\n", err);
      return err;
    }
  gcry_err = grub_cryptodisk_endecrypt (dev, (grub_uint8_t *) buf,
					size << disk->log_sector_size,
					sector, 0);
  return grub_crypto_gcry_error (gcry_err);
}

static grub_err_t
grub_cryptodisk_write (grub_disk_t disk, grub_disk_addr_t sector,
		       grub_size_t size, const char *buf)
{
  grub_cryptodisk_t dev = (grub_cryptodisk_t) disk->data;
  gcry_err_code_t gcry_err;
  char *tmp;
  grub_err_t err;

#ifdef GRUB_UTIL
  if (dev->cheat)
    {
      int r;
      r = grub_util_fd_seek (dev->cheat_fd, sector << disk->log_sector_size);
      if (r)
	return grub_error (GRUB_ERR_BAD_DEVICE, N_("cannot seek `%s': %s"),
			   dev->cheat, grub_util_fd_strerror ());
      if (grub_util_fd_write (dev->cheat_fd, buf, size << disk->log_sector_size)
	  != (ssize_t) (size << disk->log_sector_size))
	return grub_error (GRUB_ERR_READ_ERROR, N_("cannot read `%s': %s"),
			   dev->cheat, grub_util_fd_strerror ());
      return GRUB_ERR_NONE;
    }
#endif

  tmp = grub_malloc (size << disk->log_sector_size);
  if (!tmp)
    return grub_errno;
  grub_memcpy (tmp, buf, size << disk->log_sector_size);

  grub_dprintf ("cryptodisk",
		"Writing %" PRIuGRUB_SIZE " sectors to sector 0x%"
		PRIxGRUB_UINT64_T " with offset of %" PRIuGRUB_UINT64_T "\n",
		size, sector, dev->offset);

  gcry_err = grub_cryptodisk_endecrypt (dev, (grub_uint8_t *) tmp,
					size << disk->log_sector_size,
					sector, 1);
  if (gcry_err)
    {
      grub_free (tmp);
      return grub_crypto_gcry_error (gcry_err);
    }

  /* Since ->write was called so disk.mod is loaded but be paranoid  */
  
  if (grub_disk_write_weak)
    err = grub_disk_write_weak (dev->source_disk,
				(sector << (disk->log_sector_size
					    - GRUB_DISK_SECTOR_BITS))
				+ dev->offset,
				0, size << disk->log_sector_size, tmp);
  else
    err = grub_error (GRUB_ERR_BUG, "disk.mod not loaded");
  grub_free (tmp);
  return err;
}

#ifdef GRUB_UTIL
static grub_disk_memberlist_t
grub_cryptodisk_memberlist (grub_disk_t disk)
{
  grub_cryptodisk_t dev = (grub_cryptodisk_t) disk->data;
  grub_disk_memberlist_t list = NULL;

  list = grub_malloc (sizeof (*list));
  if (list)
    {
      list->disk = dev->source_disk;
      list->next = NULL;
    }

  return list;
}
#endif

static void
cryptodisk_cleanup (void)
{
#if 0
  grub_cryptodisk_t dev = cryptodisk_list;
  grub_cryptodisk_t tmp;

  while (dev != NULL)
    {
      grub_free (dev->source);
      grub_free (dev->cipher);
      grub_free (dev->secondary_cipher);
      grub_free (dev->essiv_cipher);
      tmp = dev->next;
      grub_free (dev);
      dev = tmp;
    }
#endif
}

grub_err_t
grub_cryptodisk_insert (grub_cryptodisk_t newdev, const char *name,
			grub_disk_t source)
{
  newdev->source = grub_strdup (name);
  if (!newdev->source)
    {
      grub_free (newdev);
      return grub_errno;
    }

  newdev->id = last_cryptodisk_id++;
  newdev->source_id = source->id;
  newdev->source_dev_id = source->dev->id;
  newdev->next = cryptodisk_list;
  cryptodisk_list = newdev;

  return GRUB_ERR_NONE;
}

grub_cryptodisk_t
grub_cryptodisk_get_by_uuid (const char *uuid)
{
  grub_cryptodisk_t dev;
  for (dev = cryptodisk_list; dev != NULL; dev = dev->next)
    if (grub_strcasecmp (dev->uuid, uuid) == 0)
      return dev;
  return NULL;
}

grub_cryptodisk_t
grub_cryptodisk_get_by_source_disk (grub_disk_t disk)
{
  grub_cryptodisk_t dev;
  for (dev = cryptodisk_list; dev != NULL; dev = dev->next)
    if (dev->source_id == disk->id && dev->source_dev_id == disk->dev->id)
      return dev;
  return NULL;
}

#ifdef GRUB_UTIL
grub_err_t
grub_cryptodisk_cheat_insert (grub_cryptodisk_t newdev, const char *name,
			      grub_disk_t source, const char *cheat)
{
  newdev->cheat = grub_strdup (cheat);
  newdev->source = grub_strdup (name);
  if (!newdev->source || !newdev->cheat)
    {
      grub_free (newdev->source);
      grub_free (newdev->cheat);
      return grub_errno;
    }

  newdev->cheat_fd = GRUB_UTIL_FD_INVALID;
  newdev->source_id = source->id;
  newdev->source_dev_id = source->dev->id;
  newdev->id = last_cryptodisk_id++;
  newdev->next = cryptodisk_list;
  cryptodisk_list = newdev;

  return GRUB_ERR_NONE;
}

void
grub_util_cryptodisk_get_abstraction (grub_disk_t disk,
				      void (*cb) (const char *val, void *data),
				      void *data)
{
  grub_cryptodisk_t dev = (grub_cryptodisk_t) disk->data;

  cb ("cryptodisk", data);
  cb (dev->modname, data);

  if (dev->cipher)
    cb (dev->cipher->cipher->modname, data);
  if (dev->secondary_cipher)
    cb (dev->secondary_cipher->cipher->modname, data);
  if (dev->essiv_cipher)
    cb (dev->essiv_cipher->cipher->modname, data);
  if (dev->hash)
    cb (dev->hash->modname, data);
  if (dev->essiv_hash)
    cb (dev->essiv_hash->modname, data);
  if (dev->iv_hash)
    cb (dev->iv_hash->modname, data);
}

const char *
grub_util_cryptodisk_get_uuid (grub_disk_t disk)
{
  grub_cryptodisk_t dev = (grub_cryptodisk_t) disk->data;
  return dev->uuid;
}

#endif

static int check_boot, have_it;
static char *search_uuid;

static void
cryptodisk_close (grub_cryptodisk_t dev)
{
  grub_crypto_cipher_close (dev->cipher);
  grub_crypto_cipher_close (dev->secondary_cipher);
  grub_crypto_cipher_close (dev->essiv_cipher);
  grub_free (dev);
}

static grub_err_t
grub_cryptodisk_scan_device_real (const char *name, grub_disk_t source)
{
  grub_err_t err;
  grub_cryptodisk_t dev;
  grub_cryptodisk_dev_t cr;

  dev = grub_cryptodisk_get_by_source_disk (source);

  if (dev)
    return GRUB_ERR_NONE;

  FOR_CRYPTODISK_DEVS (cr)
  {
    dev = cr->scan (source, search_uuid, check_boot);
    if (grub_errno)
      return grub_errno;
    if (!dev)
      continue;
    
    err = cr->recover_key (source, dev);
    if (err)
    {
      cryptodisk_close (dev);
      return err;
    }

    grub_cryptodisk_insert (dev, name, source);

    have_it = 1;

    return GRUB_ERR_NONE;
  }
  return GRUB_ERR_NONE;
}

#ifdef GRUB_UTIL
#include <grub/util/misc.h>
grub_err_t
grub_cryptodisk_cheat_mount (const char *sourcedev, const char *cheat)
{
  grub_err_t err;
  grub_cryptodisk_t dev;
  grub_cryptodisk_dev_t cr;
  grub_disk_t source;

  /* Try to open disk.  */
  source = grub_disk_open (sourcedev);
  if (!source)
    return grub_errno;

  dev = grub_cryptodisk_get_by_source_disk (source);

  if (dev)
    {
      grub_disk_close (source);	
      return GRUB_ERR_NONE;
    }

  FOR_CRYPTODISK_DEVS (cr)
  {
    dev = cr->scan (source, search_uuid, check_boot);
    if (grub_errno)
      return grub_errno;
    if (!dev)
      continue;

    grub_util_info ("cheatmounted %s (%s) at %s", sourcedev, dev->modname,
		    cheat);
    err = grub_cryptodisk_cheat_insert (dev, sourcedev, source, cheat);
    grub_disk_close (source);
    if (err)
      grub_free (dev);

    return GRUB_ERR_NONE;
  }

  grub_disk_close (source);

  return GRUB_ERR_NONE;
}
#endif

static int
grub_cryptodisk_scan_device (const char *name,
			     void *data __attribute__ ((unused)))
{
  grub_err_t err;
  grub_disk_t source;

  /* Try to open disk.  */
  source = grub_disk_open (name);
  if (!source)
    {
      grub_print_error ();
      return 0;
    }

  err = grub_cryptodisk_scan_device_real (name, source);

  grub_disk_close (source);
  
  if (err)
    grub_print_error ();
  return have_it && search_uuid ? 1 : 0;
}

static grub_err_t
grub_cmd_cryptomount (grub_extcmd_context_t ctxt, int argc, char **args)
{
  struct grub_arg_list *state = ctxt->state;

  if (argc < 1 && !state[1].set && !state[2].set)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "device name required");

  have_it = 0;
  if (state[0].set)
    {
      grub_cryptodisk_t dev;

      dev = grub_cryptodisk_get_by_uuid (args[0]);
      if (dev)
	{
	  grub_dprintf ("cryptodisk",
			"already mounted as crypto%lu\n", dev->id);
	  return GRUB_ERR_NONE;
	}

      check_boot = state[2].set;
      search_uuid = args[0];
      grub_device_iterate (&grub_cryptodisk_scan_device, NULL);
      search_uuid = NULL;

      if (!have_it)
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "no such cryptodisk found");
      return GRUB_ERR_NONE;
    }
  else if (state[1].set || (argc == 0 && state[2].set))
    {
      search_uuid = NULL;
      check_boot = state[2].set;
      grub_device_iterate (&grub_cryptodisk_scan_device, NULL);
      search_uuid = NULL;
      return GRUB_ERR_NONE;
    }
  else
    {
      grub_err_t err;
      grub_disk_t disk;
      grub_cryptodisk_t dev;
      char *diskname;
      char *disklast;

      search_uuid = NULL;
      check_boot = state[2].set;
      diskname = args[0];
      if (diskname[0] == '(' && *(disklast = &diskname[grub_strlen (diskname) - 1]) == ')')
	{
	  *disklast = '\0';
	  disk = grub_disk_open (diskname + 1);
	  *disklast = ')';
	}
      else
	disk = grub_disk_open (diskname);
      if (!disk)
	return grub_errno;

      dev = grub_cryptodisk_get_by_source_disk (disk);
      if (dev)
	{
	  grub_dprintf ("cryptodisk", "already mounted as crypto%lu\n", dev->id);
	  grub_disk_close (disk);
	  return GRUB_ERR_NONE;
	}

      err = grub_cryptodisk_scan_device_real (args[0], disk);

      grub_disk_close (disk);

      return err;
    }
}

static struct grub_disk_dev grub_cryptodisk_dev = {
  .name = "cryptodisk",
  .id = GRUB_DISK_DEVICE_CRYPTODISK_ID,
  .iterate = grub_cryptodisk_iterate,
  .open = grub_cryptodisk_open,
  .close = grub_cryptodisk_close,
  .read = grub_cryptodisk_read,
  .write = grub_cryptodisk_write,
#ifdef GRUB_UTIL
  .memberlist = grub_cryptodisk_memberlist,
#endif
  .next = 0
};

static char
hex (grub_uint8_t val)
{
  if (val < 10)
    return '0' + val;
  return 'a' + val - 10;
}

/* Open a file named NAME and initialize FILE.  */
static char *
luks_script_get (grub_size_t *sz)
{
  grub_cryptodisk_t i;
  grub_size_t size = 0;
  char *ptr, *ret;

  *sz = 0;

  for (i = cryptodisk_list; i != NULL; i = i->next)
    if (grub_strcmp (i->modname, "luks") == 0)
      {
	size += sizeof ("luks_mount ");
	size += grub_strlen (i->uuid);
	size += grub_strlen (i->cipher->cipher->name);
	size += 54;
	if (i->essiv_hash)
	  size += grub_strlen (i->essiv_hash->name);
	size += i->keysize * 2;
      }

  ret = grub_malloc (size + 1);
  if (!ret)
    return 0;

  ptr = ret;

  for (i = cryptodisk_list; i != NULL; i = i->next)
    if (grub_strcmp (i->modname, "luks") == 0)
      {
	unsigned j;
	const char *iptr;
	ptr = grub_stpcpy (ptr, "luks_mount ");
	ptr = grub_stpcpy (ptr, i->uuid);
	*ptr++ = ' ';
	grub_snprintf (ptr, 21, "%" PRIuGRUB_UINT64_T " ", i->offset);
	while (*ptr)
	  ptr++;
	for (iptr = i->cipher->cipher->name; *iptr; iptr++)
	  *ptr++ = grub_tolower (*iptr);
	switch (i->mode)
	  {
	  case GRUB_CRYPTODISK_MODE_ECB:
	    ptr = grub_stpcpy (ptr, "-ecb"); 
	    break;
	  case GRUB_CRYPTODISK_MODE_CBC:
	    ptr = grub_stpcpy (ptr, "-cbc");
	    break;
	  case GRUB_CRYPTODISK_MODE_PCBC:
	    ptr = grub_stpcpy (ptr, "-pcbc");
	    break;
	  case GRUB_CRYPTODISK_MODE_XTS:
	    ptr = grub_stpcpy (ptr, "-xts");
	    break;
	  case GRUB_CRYPTODISK_MODE_LRW:
	    ptr = grub_stpcpy (ptr, "-lrw");
	    break;
	  }

	switch (i->mode_iv)
	  {
	  case GRUB_CRYPTODISK_MODE_IV_NULL:
	    ptr = grub_stpcpy (ptr, "-null"); 
	    break;
	  case GRUB_CRYPTODISK_MODE_IV_PLAIN:
	    ptr = grub_stpcpy (ptr, "-plain"); 
	    break;
	  case GRUB_CRYPTODISK_MODE_IV_PLAIN64:
	    ptr = grub_stpcpy (ptr, "-plain64"); 
	    break;
	  case GRUB_CRYPTODISK_MODE_IV_BENBI:
	    ptr = grub_stpcpy (ptr, "-benbi"); 
	    break;
	  case GRUB_CRYPTODISK_MODE_IV_ESSIV:
	    ptr = grub_stpcpy (ptr, "-essiv:"); 
	    ptr = grub_stpcpy (ptr, i->essiv_hash->name);
	    break;
	  case GRUB_CRYPTODISK_MODE_IV_BYTECOUNT64:
	  case GRUB_CRYPTODISK_MODE_IV_BYTECOUNT64_HASH:
	    break;
	  }
	*ptr++ = ' ';
	for (j = 0; j < i->keysize; j++)
	  {
	    *ptr++ = hex (i->key[j] >> 4);
	    *ptr++ = hex (i->key[j] & 0xf);
	  }
	*ptr++ = '\n';
      }
  *ptr = '\0';
  *sz = ptr - ret;
  return ret;
}

struct grub_procfs_entry luks_script =
{
  .name = "luks_script",
  .get_contents = luks_script_get
};

static grub_extcmd_t cmd;

GRUB_MOD_INIT (cryptodisk)
{
  grub_disk_dev_register (&grub_cryptodisk_dev);
  cmd = grub_register_extcmd ("cryptomount", grub_cmd_cryptomount, 0,
			      N_("SOURCE|-u UUID|-a|-b"),
			      N_("Mount a crypto device."), options);
  grub_procfs_register ("luks_script", &luks_script);
}

GRUB_MOD_FINI (cryptodisk)
{
  grub_disk_dev_unregister (&grub_cryptodisk_dev);
  cryptodisk_cleanup ();
  grub_procfs_unregister (&luks_script);
}
