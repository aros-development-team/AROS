/* This file was automatically imported with 
   import_gcry.py. Please don't modify it */
#include <grub/dl.h>
GRUB_MOD_LICENSE ("GPLv3+");
/* dsa.c - DSA signature algorithm
 * Copyright (C) 1998, 2000, 2001, 2002, 2003,
 *               2006, 2008  Free Software Foundation, Inc.
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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */


#include "g10lib.h"
#include "mpi.h"
#include "cipher.h"

typedef struct
{
  gcry_mpi_t p;	    /* prime */
  gcry_mpi_t q;	    /* group order */
  gcry_mpi_t g;	    /* group generator */
  gcry_mpi_t y;	    /* g^x mod p */
} DSA_public_key;


typedef struct
{
  gcry_mpi_t p;	    /* prime */
  gcry_mpi_t q;	    /* group order */
  gcry_mpi_t g;	    /* group generator */
  gcry_mpi_t y;	    /* g^x mod p */
  gcry_mpi_t x;	    /* secret exponent */
} DSA_secret_key;


/* A structure used to hold domain parameters.  */
typedef struct
{
  gcry_mpi_t p;	    /* prime */
  gcry_mpi_t q;	    /* group order */
  gcry_mpi_t g;	    /* group generator */
} dsa_domain_t;


/* A sample 1024 bit DSA key used for the selftests.  */
/* A sample 1024 bit DSA key used for the selftests (public only).  */




static int check_secret_key (DSA_secret_key *sk);
static int verify (gcry_mpi_t r, gcry_mpi_t s, gcry_mpi_t input,
                   DSA_public_key *pkey);







/*
 * Generate a random secret exponent k less than q.
 */


/* Check that a freshly generated key actually works.  Returns 0 on success. */



/*
   Generate a DSA key pair with a key of size NBITS.  If transient_key
   is true the key is generated using the standard RNG and not the
   very secure one.

   Returns: 2 structures filled with all needed values
 	    and an array with the n-1 factors of (p-1)
 */


/* Generate a DSA key pair with a key of size NBITS using the
   algorithm given in FIPS-186-3.  If USE_FIPS186_2 is true,
   FIPS-186-2 is used and thus the length is restricted to 1024/160.
   If DERIVEPARMS is not NULL it may contain a seed value.  If domain
   parameters are specified in DOMAIN, DERIVEPARMS may not be given
   and NBITS and QBITS must match the specified domain parameters.  */



/*
   Test whether the secret key is valid.
   Returns: if this is a valid key.
 */
static int
check_secret_key( DSA_secret_key *sk )
{
  int rc;
  gcry_mpi_t y = mpi_alloc( mpi_get_nlimbs(sk->y) );

  gcry_mpi_powm( y, sk->g, sk->x, sk->p );
  rc = !mpi_cmp( y, sk->y );
  mpi_free( y );
  return rc;
}



/*
   Make a DSA signature from HASH and put it into r and s.
 */


/*
   Returns true if the signature composed from R and S is valid.
 */
static int
verify (gcry_mpi_t r, gcry_mpi_t s, gcry_mpi_t hash, DSA_public_key *pkey )
{
  int rc;
  gcry_mpi_t w, u1, u2, v;
  gcry_mpi_t base[3];
  gcry_mpi_t ex[3];

  if( !(mpi_cmp_ui( r, 0 ) > 0 && mpi_cmp( r, pkey->q ) < 0) )
    return 0; /* assertion	0 < r < q  failed */
  if( !(mpi_cmp_ui( s, 0 ) > 0 && mpi_cmp( s, pkey->q ) < 0) )
    return 0; /* assertion	0 < s < q  failed */

  w  = mpi_alloc( mpi_get_nlimbs(pkey->q) );
  u1 = mpi_alloc( mpi_get_nlimbs(pkey->q) );
  u2 = mpi_alloc( mpi_get_nlimbs(pkey->q) );
  v  = mpi_alloc( mpi_get_nlimbs(pkey->p) );

  /* w = s^(-1) mod q */
  mpi_invm( w, s, pkey->q );

  /* u1 = (hash * w) mod q */
  mpi_mulm( u1, hash, w, pkey->q );

  /* u2 = r * w mod q  */
  mpi_mulm( u2, r, w, pkey->q );

  /* v =  g^u1 * y^u2 mod p mod q */
  base[0] = pkey->g; ex[0] = u1;
  base[1] = pkey->y; ex[1] = u2;
  base[2] = NULL;    ex[2] = NULL;
  mpi_mulpowm( v, base, ex, pkey->p );
  mpi_fdiv_r( v, v, pkey->q );

  rc = !mpi_cmp( v, r );

  mpi_free(w);
  mpi_free(u1);
  mpi_free(u2);
  mpi_free(v);

  return rc;
}


/*********************************************
 **************  interface  ******************
 *********************************************/

#define dsa_generate 0

#define dsa_generate 0


static gcry_err_code_t
dsa_check_secret_key (int algo, gcry_mpi_t *skey)
{
  gcry_err_code_t err = GPG_ERR_NO_ERROR;
  DSA_secret_key sk;

  (void)algo;

  if ((! skey[0]) || (! skey[1]) || (! skey[2]) || (! skey[3]) || (! skey[4]))
    err = GPG_ERR_BAD_MPI;
  else
    {
      sk.p = skey[0];
      sk.q = skey[1];
      sk.g = skey[2];
      sk.y = skey[3];
      sk.x = skey[4];
      if (! check_secret_key (&sk))
	err = GPG_ERR_BAD_SECKEY;
    }

  return err;
}


#define dsa_sign 0
static gcry_err_code_t
dsa_verify (int algo, gcry_mpi_t hash, gcry_mpi_t *data, gcry_mpi_t *pkey,
            int (*cmp) (void *, gcry_mpi_t), void *opaquev)
{
  gcry_err_code_t err = GPG_ERR_NO_ERROR;
  DSA_public_key pk;

  (void)algo;
  (void)cmp;
  (void)opaquev;

  if ((! data[0]) || (! data[1]) || (! hash)
      || (! pkey[0]) || (! pkey[1]) || (! pkey[2]) || (! pkey[3]))
    err = GPG_ERR_BAD_MPI;
  else
    {
      pk.p = pkey[0];
      pk.q = pkey[1];
      pk.g = pkey[2];
      pk.y = pkey[3];
      if (! verify (data[0], data[1], hash, &pk))
	err = GPG_ERR_BAD_SIGNATURE;
    }
  return err;
}


static unsigned int
dsa_get_nbits (int algo, gcry_mpi_t *pkey)
{
  (void)algo;

  return mpi_get_nbits (pkey[0]);
}



/*
     Self-test section.
 */





/* Run a full self-test for ALGO and return 0 on success.  */




static const char *dsa_names[] =
  {
    "dsa",
    "openpgp-dsa",
    NULL,
  };

gcry_pk_spec_t _gcry_pubkey_spec_dsa =
  {
    "DSA", dsa_names,
    "pqgy", "pqgyx", "", "rs", "pqgy",
    GCRY_PK_USAGE_SIGN,
    dsa_generate,
    dsa_check_secret_key,
    NULL,
    NULL,
    dsa_sign,
    dsa_verify,
    dsa_get_nbits
    ,
#ifdef GRUB_UTIL
    .modname = "gcry_dsa",
#endif
  };


GRUB_MOD_INIT(gcry_dsa)
{
  grub_crypto_pk_dsa = &_gcry_pubkey_spec_dsa;
}

GRUB_MOD_FINI(gcry_dsa)
{
  grub_crypto_pk_dsa = 0;
}
