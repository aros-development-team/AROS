/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013  Free Software Foundation, Inc.
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
#include <grub/file.h>
#include <grub/command.h>
#include <grub/crypto.h>
#include <grub/i18n.h>
#include <grub/gcrypt/gcrypt.h>
#include <grub/pubkey.h>
#include <grub/env.h>
#include <grub/kernel.h>
#include <grub/extcmd.h>

GRUB_MOD_LICENSE ("GPLv3+");

struct grub_verified
{
  grub_file_t file;
  void *buf;
};
typedef struct grub_verified *grub_verified_t;

enum
  {
    OPTION_SKIP_SIG = 0
  };

static const struct grub_arg_option options[] =
  {
    {"skip-sig", 's', 0,
     N_("Skip signature-checking of the public key file."), 0, ARG_TYPE_NONE},
    {0, 0, 0, 0, 0, 0}
  };

static grub_err_t
read_packet_header (grub_file_t sig, grub_uint8_t *out_type, grub_size_t *len)
{
  grub_uint8_t type;
  grub_uint8_t l;
  grub_uint16_t l16;
  grub_uint32_t l32;

  /* New format.  */
  switch (grub_file_read (sig, &type, sizeof (type)))
    {
    case 1:
      break;
    case 0:
      {
	*out_type = 0xff;
	return 0;
      }
    default:
      if (grub_errno)
	return grub_errno;
      /* TRANSLATORS: it's about GNUPG signatures.  */
      return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
    }

  if (type == 0)
    {
      *out_type = 0xfe;
      return 0;      
    }

  if (!(type & 0x80))
    return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
  if (type & 0x40)
    {
      *out_type = (type & 0x3f);
      if (grub_file_read (sig, &l, sizeof (l)) != 1)
	return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
      if (l < 192)
	{
	  *len = l;
	  return 0;
	}
      if (l < 224)
	{
	  *len = (l - 192) << GRUB_CHAR_BIT;
	  if (grub_file_read (sig, &l, sizeof (l)) != 1)
	    return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
	  *len |= l;
	  return 0;
	}
      if (l == 255)
	{
	  if (grub_file_read (sig, &l32, sizeof (l32)) != sizeof (l32))
	    return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
	  *len = grub_be_to_cpu32 (l32);
	  return 0;
	}
      return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
    }
  *out_type = ((type >> 2) & 0xf);
  switch (type & 0x3)
    {
    case 0:
      if (grub_file_read (sig, &l, sizeof (l)) != sizeof (l))
	return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
      *len = l;
      return 0;
    case 1:
      if (grub_file_read (sig, &l16, sizeof (l16)) != sizeof (l16))
	return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
      *len = grub_be_to_cpu16 (l16);
      return 0;
    case 2:
      if (grub_file_read (sig, &l32, sizeof (l32)) != sizeof (l32))
	return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
      *len = grub_be_to_cpu32 (l32);
      return 0;
    }
  return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
}

struct signature_v4_header
{
  grub_uint8_t type;
  grub_uint8_t pkeyalgo;
  grub_uint8_t hash;
  grub_uint16_t hashed_sub;
} GRUB_PACKED;

const char *hashes[] = {
  [0x01] = "md5",
  [0x02] = "sha1",
  [0x03] = "ripemd160",
  [0x08] = "sha256",
  [0x09] = "sha384",
  [0x0a] = "sha512",
  [0x0b] = "sha224"
};

struct gcry_pk_spec *grub_crypto_pk_dsa;
struct gcry_pk_spec *grub_crypto_pk_ecdsa;
struct gcry_pk_spec *grub_crypto_pk_rsa;

static int
dsa_pad (gcry_mpi_t *hmpi, grub_uint8_t *hval,
	 const gcry_md_spec_t *hash, struct grub_public_subkey *sk);
static int
rsa_pad (gcry_mpi_t *hmpi, grub_uint8_t *hval,
	 const gcry_md_spec_t *hash, struct grub_public_subkey *sk);

struct
{
  const char *name;
  grub_size_t nmpisig;
  grub_size_t nmpipub;
  struct gcry_pk_spec **algo;
  int (*pad) (gcry_mpi_t *hmpi, grub_uint8_t *hval,
	      const gcry_md_spec_t *hash, struct grub_public_subkey *sk);
  const char *module;
} pkalgos[] = 
  {
    [1] = { "rsa", 1, 2, &grub_crypto_pk_rsa, rsa_pad, "gcry_rsa" },
    [3] = { "rsa", 1, 2, &grub_crypto_pk_rsa, rsa_pad, "gcry_rsa" },
    [17] = { "dsa", 2, 4, &grub_crypto_pk_dsa, dsa_pad, "gcry_dsa" },
  };

struct grub_public_key
{
  struct grub_public_key *next;
  struct grub_public_subkey *subkeys;
};

struct grub_public_subkey
{
  struct grub_public_subkey *next;
  grub_uint8_t type;
  grub_uint32_t fingerprint[5];
  gcry_mpi_t mpis[10];
};

static void
free_pk (struct grub_public_key *pk)
{
  struct grub_public_subkey *nsk, *sk;
  for (sk = pk->subkeys; sk; sk = nsk)
    {
      grub_size_t i;
      for (i = 0; i < ARRAY_SIZE (sk->mpis); i++)
	if (sk->mpis[i])
	  gcry_mpi_release (sk->mpis[i]);
      nsk = sk->next;
      grub_free (sk);
    }
  grub_free (pk);
}

#define READBUF_SIZE 4096

struct grub_public_key *
grub_load_public_key (grub_file_t f)
{
  grub_err_t err;
  struct grub_public_key *ret;
  struct grub_public_subkey **last = 0;
  void *fingerprint_context = NULL;
  grub_uint8_t *buffer = NULL;

  ret = grub_zalloc (sizeof (*ret));
  if (!ret)
    {
      grub_free (fingerprint_context);
      return NULL;
    }

  buffer = grub_zalloc (READBUF_SIZE);
  fingerprint_context = grub_zalloc (GRUB_MD_SHA1->contextsize);

  if (!buffer || !fingerprint_context)
    goto fail;

  last = &ret->subkeys;

  while (1)
    {
      grub_uint8_t type;
      grub_size_t len;
      grub_uint8_t v, pk;
      grub_uint32_t creation_time;
      grub_off_t pend;
      struct grub_public_subkey *sk;
      grub_size_t i;
      grub_uint16_t len_be;

      err = read_packet_header (f, &type, &len);

      if (err)
	goto fail;
      if (type == 0xfe)
	continue;
      if (type == 0xff)
	{
	  grub_free (fingerprint_context);
	  grub_free (buffer);
	  return ret;
	}

      grub_dprintf ("crypt", "len = %x\n", (int) len);

      pend = grub_file_tell (f) + len;
      if (type != 6 && type != 14
	  && type != 5 && type != 7)
	{
	  grub_file_seek (f, pend);
	  continue;
	}

      if (grub_file_read (f, &v, sizeof (v)) != sizeof (v))
	{
	  grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
	  goto fail;
	}

      grub_dprintf ("crypt", "v = %x\n", v);

      if (v != 4)
	{
	  grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
	  goto fail;
	}
      if (grub_file_read (f, &creation_time, sizeof (creation_time)) != sizeof (creation_time))
	{
	  grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
	  goto fail;
	}

      grub_dprintf ("crypt", "time = %x\n", creation_time);

      if (grub_file_read (f, &pk, sizeof (pk)) != sizeof (pk))
	{
	  grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
	  goto fail;
	}

      grub_dprintf ("crypt", "pk = %x\n", pk);

      if (pk >= ARRAY_SIZE (pkalgos) || pkalgos[pk].name == NULL)
	{
	  grub_file_seek (f, pend);
	  continue;
	}

      sk = grub_zalloc (sizeof (struct grub_public_subkey));
      if (!sk)
	goto fail;

      grub_memset (fingerprint_context, 0, GRUB_MD_SHA1->contextsize);
      GRUB_MD_SHA1->init (fingerprint_context);
      GRUB_MD_SHA1->write (fingerprint_context, "\x99", 1);
      len_be = grub_cpu_to_be16 (len);
      GRUB_MD_SHA1->write (fingerprint_context, &len_be, sizeof (len_be));
      GRUB_MD_SHA1->write (fingerprint_context, &v, sizeof (v));
      GRUB_MD_SHA1->write (fingerprint_context, &creation_time, sizeof (creation_time));
      GRUB_MD_SHA1->write (fingerprint_context, &pk, sizeof (pk));

      for (i = 0; i < pkalgos[pk].nmpipub; i++)
	{
	  grub_uint16_t l;
	  grub_size_t lb;
	  if (grub_file_read (f, &l, sizeof (l)) != sizeof (l))
	    {
	      grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
	      goto fail;
	    }
	  
	  lb = (grub_be_to_cpu16 (l) + GRUB_CHAR_BIT - 1) / GRUB_CHAR_BIT;
	  if (lb > READBUF_SIZE - sizeof (grub_uint16_t))
	    {
	      grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
	      goto fail;
	    }
	  if (grub_file_read (f, buffer + sizeof (grub_uint16_t), lb) != (grub_ssize_t) lb)
	    {
	      grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
	      goto fail;
	    }
	  grub_memcpy (buffer, &l, sizeof (l));

	  GRUB_MD_SHA1->write (fingerprint_context, buffer, lb + sizeof (grub_uint16_t));
 
	  if (gcry_mpi_scan (&sk->mpis[i], GCRYMPI_FMT_PGP,
			     buffer, lb + sizeof (grub_uint16_t), 0))
	    {
	      grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
	      goto fail;
	    }
	}

      GRUB_MD_SHA1->final (fingerprint_context);

      grub_memcpy (sk->fingerprint, GRUB_MD_SHA1->read (fingerprint_context), 20);

      *last = sk;
      last = &sk->next;

      grub_dprintf ("crypt", "actual pos: %x, expected: %x\n", (int)grub_file_tell (f), (int)pend);

      grub_file_seek (f, pend);
    }
 fail:
  free_pk (ret);
  grub_free (fingerprint_context);
  grub_free (buffer);
  return NULL;
}

struct grub_public_key *grub_pk_trusted;

struct grub_public_subkey *
grub_crypto_pk_locate_subkey (grub_uint64_t keyid, struct grub_public_key *pkey)
{
  struct grub_public_subkey *sk;
  for (sk = pkey->subkeys; sk; sk = sk->next)
    if (grub_memcmp (sk->fingerprint + 3, &keyid, 8) == 0)
      return sk;
  return 0;
}

struct grub_public_subkey *
grub_crypto_pk_locate_subkey_in_trustdb (grub_uint64_t keyid)
{
  struct grub_public_key *pkey;
  struct grub_public_subkey *sk;
  for (pkey = grub_pk_trusted; pkey; pkey = pkey->next)
    {
      sk = grub_crypto_pk_locate_subkey (keyid, pkey);
      if (sk)
	return sk;
    }
  return 0;
}


static int
dsa_pad (gcry_mpi_t *hmpi, grub_uint8_t *hval,
	 const gcry_md_spec_t *hash, struct grub_public_subkey *sk)
{
  unsigned nbits = gcry_mpi_get_nbits (sk->mpis[1]);
  grub_dprintf ("crypt", "must be %u bits got %d bits\n", nbits,
		(int)(8 * hash->mdlen));
  return gcry_mpi_scan (hmpi, GCRYMPI_FMT_USG, hval,
			nbits / 8 < (unsigned) hash->mdlen ? nbits / 8
			: (unsigned) hash->mdlen, 0);
}

static int
rsa_pad (gcry_mpi_t *hmpi, grub_uint8_t *hval,
	 const gcry_md_spec_t *hash, struct grub_public_subkey *sk)
{
  grub_size_t tlen, emlen, fflen;
  grub_uint8_t *em, *emptr;
  unsigned nbits = gcry_mpi_get_nbits (sk->mpis[0]);
  int ret;
  tlen = hash->mdlen + hash->asnlen;
  emlen = (nbits + 7) / 8;
  if (emlen < tlen + 11)
    return 1;

  em = grub_malloc (emlen);
  if (!em)
    return 1;

  em[0] = 0x00;
  em[1] = 0x01;
  fflen = emlen - tlen - 3;
  for (emptr = em + 2; emptr < em + 2 + fflen; emptr++)
    *emptr = 0xff;
  *emptr++ = 0x00;
  grub_memcpy (emptr, hash->asnoid, hash->asnlen);
  emptr += hash->asnlen;
  grub_memcpy (emptr, hval, hash->mdlen);

  ret = gcry_mpi_scan (hmpi, GCRYMPI_FMT_USG, em, emlen, 0);
  grub_free (em);
  return ret;
}

static grub_err_t
grub_verify_signature_real (char *buf, grub_size_t size,
			    grub_file_t f, grub_file_t sig,
			    struct grub_public_key *pkey)
{
  grub_size_t len;
  grub_uint8_t v;
  grub_uint8_t h;
  grub_uint8_t t;
  grub_uint8_t pk;
  const gcry_md_spec_t *hash;
  struct signature_v4_header v4;
  grub_err_t err;
  grub_size_t i;
  gcry_mpi_t mpis[10];
  grub_uint8_t type;

  err = read_packet_header (sig, &type, &len);
  if (err)
    return err;

  if (type != 0x2)
    return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));

  if (grub_file_read (sig, &v, sizeof (v)) != sizeof (v))
    return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));

  if (v != 4)
    return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));

  if (grub_file_read (sig, &v4, sizeof (v4)) != sizeof (v4))
    return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));

  h = v4.hash;
  t = v4.type;
  pk = v4.pkeyalgo;
  
  if (t != 0)
    return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));

  if (h >= ARRAY_SIZE (hashes) || hashes[h] == NULL)
    return grub_error (GRUB_ERR_BAD_SIGNATURE, "unknown hash");

  if (pk >= ARRAY_SIZE (pkalgos) || pkalgos[pk].name == NULL)
    return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));

  hash = grub_crypto_lookup_md_by_name (hashes[h]);
  if (!hash)
    return grub_error (GRUB_ERR_BAD_SIGNATURE, "hash `%s' not loaded", hashes[h]);

  grub_dprintf ("crypt", "alive\n");

  {
    void *context = NULL;
    unsigned char *hval;
    grub_ssize_t rem = grub_be_to_cpu16 (v4.hashed_sub);
    grub_uint32_t headlen = grub_cpu_to_be32 (rem + 6);
    grub_uint8_t s;
    grub_uint16_t unhashed_sub;
    grub_ssize_t r;
    grub_uint8_t hash_start[2];
    gcry_mpi_t hmpi;
    grub_uint64_t keyid = 0;
    struct grub_public_subkey *sk;
    grub_uint8_t *readbuf = NULL;

    context = grub_zalloc (hash->contextsize);
    readbuf = grub_zalloc (READBUF_SIZE);
    if (!context || !readbuf)
      goto fail;

    hash->init (context);
    if (buf)
      hash->write (context, buf, size);
    else 
      while (1)
	{
	  r = grub_file_read (f, readbuf, READBUF_SIZE);
	  if (r < 0)
	    goto fail;
	  if (r == 0)
	    break;
	  hash->write (context, readbuf, r);
	}

    hash->write (context, &v, sizeof (v));
    hash->write (context, &v4, sizeof (v4));
    while (rem)
      {
	r = grub_file_read (sig, readbuf,
			    rem < READBUF_SIZE ? rem : READBUF_SIZE);
	if (r < 0)
	  goto fail;
	if (r == 0)
	  break;
	hash->write (context, readbuf, r);
	rem -= r;
      }
    hash->write (context, &v, sizeof (v));
    s = 0xff;
    hash->write (context, &s, sizeof (s));
    hash->write (context, &headlen, sizeof (headlen));
    r = grub_file_read (sig, &unhashed_sub, sizeof (unhashed_sub));
    if (r != sizeof (unhashed_sub))
      goto fail;
    {
      grub_uint8_t *ptr;
      grub_uint32_t l;
      rem = grub_be_to_cpu16 (unhashed_sub);
      if (rem > READBUF_SIZE)
	goto fail;
      r = grub_file_read (sig, readbuf, rem);
      if (r != rem)
	goto fail;
      for (ptr = readbuf; ptr < readbuf + rem; ptr += l)
	{
	  if (*ptr < 192)
	    l = *ptr++;
	  else if (*ptr < 255)
	    {
	      if (ptr + 1 >= readbuf + rem)
		break;
	      l = (((ptr[0] & ~192) << GRUB_CHAR_BIT) | ptr[1]) + 192;
	      ptr += 2;
	    }
	  else
	    {
	      if (ptr + 5 >= readbuf + rem)
		break;
	      l = grub_be_to_cpu32 (grub_get_unaligned32 (ptr + 1));
	      ptr += 5;
	    }
	  if (*ptr == 0x10 && l >= 8)
	    keyid = grub_get_unaligned64 (ptr + 1);
	}
    }

    hash->final (context);

    grub_dprintf ("crypt", "alive\n");

    hval = hash->read (context);

    if (grub_file_read (sig, hash_start, sizeof (hash_start)) != sizeof (hash_start))
      goto fail;
    if (grub_memcmp (hval, hash_start, sizeof (hash_start)) != 0)
      goto fail;

    grub_dprintf ("crypt", "@ %x\n", (int)grub_file_tell (sig));

    for (i = 0; i < pkalgos[pk].nmpisig; i++)
      {
	grub_uint16_t l;
	grub_size_t lb;
	grub_dprintf ("crypt", "alive\n");
	if (grub_file_read (sig, &l, sizeof (l)) != sizeof (l))
	  goto fail;
	grub_dprintf ("crypt", "alive\n");
	lb = (grub_be_to_cpu16 (l) + 7) / 8;
	grub_dprintf ("crypt", "l = 0x%04x\n", grub_be_to_cpu16 (l));
	if (lb > READBUF_SIZE - sizeof (grub_uint16_t))
	  goto fail;
	grub_dprintf ("crypt", "alive\n");
	if (grub_file_read (sig, readbuf + sizeof (grub_uint16_t), lb) != (grub_ssize_t) lb)
	  goto fail;
	grub_dprintf ("crypt", "alive\n");
	grub_memcpy (readbuf, &l, sizeof (l));
	grub_dprintf ("crypt", "alive\n");

	if (gcry_mpi_scan (&mpis[i], GCRYMPI_FMT_PGP,
			   readbuf, lb + sizeof (grub_uint16_t), 0))
	  goto fail;
	grub_dprintf ("crypt", "alive\n");
      }

    if (pkey)
      sk = grub_crypto_pk_locate_subkey (keyid, pkey);
    else
      sk = grub_crypto_pk_locate_subkey_in_trustdb (keyid);
    if (!sk)
      {
	/* TRANSLATORS: %08x is 32-bit key id.  */
	grub_error (GRUB_ERR_BAD_SIGNATURE, N_("public key %08x not found"),
		    keyid);
	goto fail;
      }

    if (pkalgos[pk].pad (&hmpi, hval, hash, sk))
      goto fail;
    if (!*pkalgos[pk].algo)
      {
	grub_dl_load (pkalgos[pk].module);
	grub_errno = GRUB_ERR_NONE;
      }

    if (!*pkalgos[pk].algo)
      {
	grub_error (GRUB_ERR_BAD_SIGNATURE, N_("module `%s' isn't loaded"),
		    pkalgos[pk].module);
	goto fail;
      }
    if ((*pkalgos[pk].algo)->verify (0, hmpi, mpis, sk->mpis, 0, 0))
      goto fail;

    grub_free (context);
    grub_free (readbuf);

    return GRUB_ERR_NONE;

  fail:
    grub_free (context);
    grub_free (readbuf);
    if (!grub_errno)
      return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("bad signature"));
    return grub_errno;
  }
}

grub_err_t
grub_verify_signature (grub_file_t f, grub_file_t sig,
		       struct grub_public_key *pkey)
{
  return grub_verify_signature_real (0, 0, f, sig, pkey);
}

static grub_err_t
grub_cmd_trust (grub_extcmd_context_t ctxt,
		int argc, char **args)
{
  grub_file_t pkf;
  struct grub_public_key *pk = NULL;

  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("one argument expected"));

  grub_file_filter_disable_compression ();
  if (ctxt->state[OPTION_SKIP_SIG].set)
    grub_file_filter_disable_pubkey ();
  pkf = grub_file_open (args[0]);
  if (!pkf)
    return grub_errno;
  pk = grub_load_public_key (pkf);
  if (!pk)
    {
      grub_file_close (pkf);
      return grub_errno;
    }
  grub_file_close (pkf);

  pk->next = grub_pk_trusted;
  grub_pk_trusted = pk;

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_list (grub_command_t cmd  __attribute__ ((unused)),
	       int argc __attribute__ ((unused)),
	       char **args __attribute__ ((unused)))
{
  struct grub_public_key *pk = NULL;
  struct grub_public_subkey *sk = NULL;

  for (pk = grub_pk_trusted; pk; pk = pk->next)
    for (sk = pk->subkeys; sk; sk = sk->next)
      {
	unsigned i;
	for (i = 0; i < 20; i += 2)
	  grub_printf ("%02x%02x ", ((grub_uint8_t *) sk->fingerprint)[i],
		       ((grub_uint8_t *) sk->fingerprint)[i + 1]);
	grub_printf ("\n");
      }

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_distrust (grub_command_t cmd  __attribute__ ((unused)),
		   int argc, char **args)
{
  grub_uint32_t keyid, keyid_be;
  struct grub_public_key **pkey;
  struct grub_public_subkey *sk;

  if (argc < 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("one argument expected"));
  keyid = grub_strtoull (args[0], 0, 16);
  if (grub_errno)
    return grub_errno;
  keyid_be = grub_cpu_to_be32 (keyid);

  for (pkey = &grub_pk_trusted; *pkey; pkey = &((*pkey)->next))
    {
      struct grub_public_key *next;
      for (sk = (*pkey)->subkeys; sk; sk = sk->next)
	if (grub_memcmp (sk->fingerprint + 4, &keyid_be, 4) == 0)
	  break;
      if (!sk)
	continue;
      next = (*pkey)->next;
      free_pk (*pkey);
      *pkey = next;
      return GRUB_ERR_NONE;
    }
  /* TRANSLATORS: %08x is 32-bit key id.  */
  return grub_error (GRUB_ERR_BAD_SIGNATURE, N_("public key %08x not found"), keyid);
}

static grub_err_t
grub_cmd_verify_signature (grub_extcmd_context_t ctxt,
			   int argc, char **args)
{
  grub_file_t f = NULL, sig = NULL;
  grub_err_t err = GRUB_ERR_NONE;
  struct grub_public_key *pk = NULL;

  grub_dprintf ("crypt", "alive\n");

  if (argc < 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("two arguments expected"));

  grub_dprintf ("crypt", "alive\n");

  if (argc > 2)
    {
      grub_file_t pkf;
      grub_file_filter_disable_compression ();
      if (ctxt->state[OPTION_SKIP_SIG].set)
	grub_file_filter_disable_pubkey ();
      pkf = grub_file_open (args[2]);
      if (!pkf)
	return grub_errno;
      pk = grub_load_public_key (pkf);
      if (!pk)
	{
	  grub_file_close (pkf);
	  return grub_errno;
	}
      grub_file_close (pkf);
    }

  grub_file_filter_disable_all ();
  f = grub_file_open (args[0]);
  if (!f)
    {
      err = grub_errno;
      goto fail;
    }

  grub_file_filter_disable_all ();
  sig = grub_file_open (args[1]);
  if (!sig)
    {
      err = grub_errno;
      goto fail;
    }

  err = grub_verify_signature (f, sig, pk);
 fail:
  if (sig)
    grub_file_close (sig);
  if (f)
    grub_file_close (f);
  if (pk)
    free_pk (pk);
  return err;
}

static int sec = 0;

static void
verified_free (grub_verified_t verified)
{
  if (verified)
    {
      grub_free (verified->buf);
      grub_free (verified);
    }
}

static grub_ssize_t
verified_read (struct grub_file *file, char *buf, grub_size_t len)
{
  grub_verified_t verified = file->data;

  grub_memcpy (buf, (char *) verified->buf + file->offset, len);
  return len;
}

static grub_err_t
verified_close (struct grub_file *file)
{
  grub_verified_t verified = file->data;

  grub_file_close (verified->file);
  verified_free (verified);
  file->data = 0;

  /* device and name are freed by parent */
  file->device = 0;
  file->name = 0;

  return grub_errno;
}

struct grub_fs verified_fs =
{
  .name = "verified_read",
  .read = verified_read,
  .close = verified_close
};

static grub_file_t
grub_pubkey_open (grub_file_t io, const char *filename)
{
  grub_file_t sig;
  char *fsuf, *ptr;
  grub_err_t err;
  grub_file_filter_t curfilt[GRUB_FILE_FILTER_MAX];
  grub_file_t ret;
  grub_verified_t verified;

  if (!sec)
    return io;
  if (io->device->disk && 
      (io->device->disk->dev->id == GRUB_DISK_DEVICE_MEMDISK_ID
       || io->device->disk->dev->id == GRUB_DISK_DEVICE_PROCFS_ID))
    return io;
  fsuf = grub_malloc (grub_strlen (filename) + sizeof (".sig"));
  if (!fsuf)
    return NULL;
  ptr = grub_stpcpy (fsuf, filename);
  grub_memcpy (ptr, ".sig", sizeof (".sig"));

  grub_memcpy (curfilt, grub_file_filters_enabled,
	       sizeof (curfilt));
  grub_file_filter_disable_all ();
  sig = grub_file_open (fsuf);
  grub_memcpy (grub_file_filters_enabled, curfilt,
	       sizeof (curfilt));
  grub_free (fsuf);
  if (!sig)
    return NULL;

  ret = grub_malloc (sizeof (*ret));
  if (!ret)
    {
      grub_file_close (sig);
      return NULL;
    }
  *ret = *io;

  ret->fs = &verified_fs;
  ret->not_easily_seekable = 0;
  if (ret->size >> (sizeof (grub_size_t) * GRUB_CHAR_BIT - 1))
    {
      grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		  "big file signature isn't implemented yet");
      grub_file_close (sig);
      grub_free (ret);
      return NULL;
    }
  verified = grub_malloc (sizeof (*verified));
  if (!verified)
    {
      grub_file_close (sig);
      grub_free (ret);
      return NULL;
    }
  verified->buf = grub_malloc (ret->size);
  if (!verified->buf)
    {
      grub_file_close (sig);
      grub_free (verified);
      grub_free (ret);
      return NULL;
    }
  if (grub_file_read (io, verified->buf, ret->size) != (grub_ssize_t) ret->size)
    {
      if (!grub_errno)
	grub_error (GRUB_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
		    filename);
      grub_file_close (sig);
      verified_free (verified);
      grub_free (ret);
      return NULL;
    }

  err = grub_verify_signature_real (verified->buf, ret->size, 0, sig, NULL);
  grub_file_close (sig);
  if (err)
    {
      verified_free (verified);
      grub_free (ret);
      return NULL;
    }
  verified->file = io;
  ret->data = verified;
  return ret;
}

static char *
grub_env_write_sec (struct grub_env_var *var __attribute__ ((unused)),
		    const char *val)
{
  sec = (*val == '1') || (*val == 'e');
  return grub_strdup (sec ? "enforce" : "no");
}

static grub_ssize_t 
pseudo_read (struct grub_file *file, char *buf, grub_size_t len)
{
  grub_memcpy (buf, (grub_uint8_t *) file->data + file->offset, len);
  return len;
}


/* Filesystem descriptor.  */
struct grub_fs pseudo_fs = 
  {
    .name = "pseudo",
    .read = pseudo_read
};


static grub_extcmd_t cmd, cmd_trust;
static grub_command_t cmd_distrust, cmd_list;

GRUB_MOD_INIT(verify)
{
  const char *val;
  struct grub_module_header *header;

  val = grub_env_get ("check_signatures");
  if (val && (val[0] == '1' || val[0] == 'e'))
    sec = 1;
  else
    sec = 0;
    
  grub_file_filter_register (GRUB_FILE_FILTER_PUBKEY, grub_pubkey_open);

  grub_register_variable_hook ("check_signatures", 0, grub_env_write_sec);
  grub_env_export ("check_signatures");

  grub_pk_trusted = 0;
  FOR_MODULES (header)
  {
    struct grub_file pseudo_file;
    struct grub_public_key *pk = NULL;

    grub_memset (&pseudo_file, 0, sizeof (pseudo_file));

    /* Not an ELF module, skip.  */
    if (header->type != OBJ_TYPE_PUBKEY)
      continue;

    pseudo_file.fs = &pseudo_fs;
    pseudo_file.size = (header->size - sizeof (struct grub_module_header));
    pseudo_file.data = (char *) header + sizeof (struct grub_module_header);

    pk = grub_load_public_key (&pseudo_file);
    if (!pk)
      grub_fatal ("error loading initial key: %s\n", grub_errmsg);

    pk->next = grub_pk_trusted;
    grub_pk_trusted = pk;
  }

  if (!val)
    grub_env_set ("check_signatures", grub_pk_trusted ? "enforce" : "no");

  cmd = grub_register_extcmd ("verify_detached", grub_cmd_verify_signature, 0,
			      N_("[-s|--skip-sig] FILE SIGNATURE_FILE [PUBKEY_FILE]"),
			      N_("Verify detached signature."),
			      options);
  cmd_trust = grub_register_extcmd ("trust", grub_cmd_trust, 0,
				     N_("[-s|--skip-sig] PUBKEY_FILE"),
				     N_("Add PUBKEY_FILE to trusted keys."),
				     options);
  cmd_list = grub_register_command ("list_trusted", grub_cmd_list,
				    0,
				    N_("Show the list of trusted keys."));
  cmd_distrust = grub_register_command ("distrust", grub_cmd_distrust,
					N_("PUBKEY_ID"),
					N_("Remove PUBKEY_ID from trusted keys."));
}

GRUB_MOD_FINI(verify)
{
  grub_file_filter_unregister (GRUB_FILE_FILTER_PUBKEY);
  grub_unregister_extcmd (cmd);
  grub_unregister_extcmd (cmd_trust);
  grub_unregister_command (cmd_list);
  grub_unregister_command (cmd_distrust);
}
