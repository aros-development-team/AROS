/* This file was automatically imported with 
   import_gcry.py. Please don't modify it */
#include <grub/dl.h>
GRUB_MOD_LICENSE ("GPLv3+");
/* rsa.c - RSA implementation
 * Copyright (C) 1997, 1998, 1999 by Werner Koch (dd9jn)
 * Copyright (C) 2000, 2001, 2002, 2003, 2008 Free Software Foundation, Inc.
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

/* This code uses an algorithm protected by U.S. Patent #4,405,829
   which expired on September 20, 2000.  The patent holder placed that
   patent into the public domain on Sep 6th, 2000.
*/


#include "g10lib.h"
#include "mpi.h"
#include "cipher.h"


typedef struct
{
  gcry_mpi_t n;	    /* modulus */
  gcry_mpi_t e;	    /* exponent */
} RSA_public_key;


typedef struct
{
  gcry_mpi_t n;	    /* public modulus */
  gcry_mpi_t e;	    /* public exponent */
  gcry_mpi_t d;	    /* exponent */
  gcry_mpi_t p;	    /* prime  p. */
  gcry_mpi_t q;	    /* prime  q. */
  gcry_mpi_t u;	    /* inverse of p mod q. */
} RSA_secret_key;


/* A sample 1024 bit RSA key used for the selftests.  */
/* A sample 1024 bit RSA key used for the selftests (public only).  */




static int  check_secret_key (RSA_secret_key *sk);
static void public (gcry_mpi_t output, gcry_mpi_t input, RSA_public_key *skey);


/* Check that a freshly generated key actually works.  Returns 0 on success. */


/* Callback used by the prime generation to test whether the exponent
   is suitable. Returns 0 if the test has been passed. */

/****************
 * Generate a key pair with a key of size NBITS.
 * USE_E = 0 let Libcgrypt decide what exponent to use.
 *       = 1 request the use of a "secure" exponent; this is required by some
 *           specification to be 65537.
 *       > 2 Use this public exponent.  If the given exponent
 *           is not odd one is internally added to it.
 * TRANSIENT_KEY:  If true, generate the primes using the standard RNG.
 * Returns: 2 structures filled with all needed values
 */


/* Helper for generate_x931.  */


/* Helper for generate_x931.  */



/* Variant of the standard key generation code using the algorithm
   from X9.31.  Using this algorithm has the advantage that the
   generation can be made deterministic which is required for CAVS
   testing.  */


/****************
 * Test whether the secret key is valid.
 * Returns: true if this is a valid key.
 */
static int
check_secret_key( RSA_secret_key *sk )
{
  int rc;
  gcry_mpi_t temp = mpi_alloc( mpi_get_nlimbs(sk->p)*2 );

  mpi_mul(temp, sk->p, sk->q );
  rc = mpi_cmp( temp, sk->n );
  mpi_free(temp);
  return !rc;
}



/****************
 * Public key operation. Encrypt INPUT with PKEY and put result into OUTPUT.
 *
 *	c = m^e mod n
 *
 * Where c is OUTPUT, m is INPUT and e,n are elements of PKEY.
 */
static void
public(gcry_mpi_t output, gcry_mpi_t input, RSA_public_key *pkey )
{
  if( output == input )  /* powm doesn't like output and input the same */
    {
      gcry_mpi_t x = mpi_alloc( mpi_get_nlimbs(input)*2 );
      mpi_powm( x, input, pkey->e, pkey->n );
      mpi_set(output, x);
      mpi_free(x);
    }
  else
    mpi_powm( output, input, pkey->e, pkey->n );
}

#if 0
static void
stronger_key_check ( RSA_secret_key *skey )
{
  gcry_mpi_t t = mpi_alloc_secure ( 0 );
  gcry_mpi_t t1 = mpi_alloc_secure ( 0 );
  gcry_mpi_t t2 = mpi_alloc_secure ( 0 );
  gcry_mpi_t phi = mpi_alloc_secure ( 0 );

  /* check that n == p * q */
  mpi_mul( t, skey->p, skey->q);
  if (mpi_cmp( t, skey->n) )
    log_info ( "RSA Oops: n != p * q\n" );

  /* check that p is less than q */
  if( mpi_cmp( skey->p, skey->q ) > 0 )
    {
      log_info ("RSA Oops: p >= q - fixed\n");
      _gcry_mpi_swap ( skey->p, skey->q);
    }

    /* check that e divides neither p-1 nor q-1 */
    mpi_sub_ui(t, skey->p, 1 );
    mpi_fdiv_r(t, t, skey->e );
    if ( !mpi_cmp_ui( t, 0) )
        log_info ( "RSA Oops: e divides p-1\n" );
    mpi_sub_ui(t, skey->q, 1 );
    mpi_fdiv_r(t, t, skey->e );
    if ( !mpi_cmp_ui( t, 0) )
        log_info ( "RSA Oops: e divides q-1\n" );

    /* check that d is correct */
    mpi_sub_ui( t1, skey->p, 1 );
    mpi_sub_ui( t2, skey->q, 1 );
    mpi_mul( phi, t1, t2 );
    gcry_mpi_gcd(t, t1, t2);
    mpi_fdiv_q(t, phi, t);
    mpi_invm(t, skey->e, t );
    if ( mpi_cmp(t, skey->d ) )
      {
        log_info ( "RSA Oops: d is wrong - fixed\n");
        mpi_set (skey->d, t);
        _gcry_log_mpidump ("  fixed d", skey->d);
      }

    /* check for correctness of u */
    mpi_invm(t, skey->p, skey->q );
    if ( mpi_cmp(t, skey->u ) )
      {
        log_info ( "RSA Oops: u is wrong - fixed\n");
        mpi_set (skey->u, t);
        _gcry_log_mpidump ("  fixed u", skey->u);
      }

    log_info ( "RSA secret key check finished\n");

    mpi_free (t);
    mpi_free (t1);
    mpi_free (t2);
    mpi_free (phi);
}
#endif



/****************
 * Secret key operation. Encrypt INPUT with SKEY and put result into OUTPUT.
 *
 *	m = c^d mod n
 *
 * Or faster:
 *
 *      m1 = c ^ (d mod (p-1)) mod p
 *      m2 = c ^ (d mod (q-1)) mod q
 *      h = u * (m2 - m1) mod q
 *      m = m1 + h * p
 *
 * Where m is OUTPUT, c is INPUT and d,n,p,q,u are elements of SKEY.
 */



/* Perform RSA blinding.  */

/* Undo RSA blinding.  */

/*********************************************
 **************  interface  ******************
 *********************************************/



#define rsa_generate 0

static gcry_err_code_t
rsa_check_secret_key (int algo, gcry_mpi_t *skey)
{
  gcry_err_code_t err = GPG_ERR_NO_ERROR;
  RSA_secret_key sk;

  (void)algo;

  sk.n = skey[0];
  sk.e = skey[1];
  sk.d = skey[2];
  sk.p = skey[3];
  sk.q = skey[4];
  sk.u = skey[5];

  if (!sk.p || !sk.q || !sk.u)
    err = GPG_ERR_NO_OBJ;  /* To check the key we need the optional
                              parameters. */
  else if (!check_secret_key (&sk))
    err = GPG_ERR_BAD_SECKEY;

  return err;
}


static gcry_err_code_t
rsa_encrypt (int algo, gcry_mpi_t *resarr, gcry_mpi_t data,
             gcry_mpi_t *pkey, int flags)
{
  RSA_public_key pk;

  (void)algo;
  (void)flags;

  pk.n = pkey[0];
  pk.e = pkey[1];
  resarr[0] = mpi_alloc (mpi_get_nlimbs (pk.n));
  public (resarr[0], data, &pk);

  return GPG_ERR_NO_ERROR;
}


#define rsa_decrypt 0

#define rsa_sign 0

static gcry_err_code_t
rsa_verify (int algo, gcry_mpi_t hash, gcry_mpi_t *data, gcry_mpi_t *pkey,
		  int (*cmp) (void *opaque, gcry_mpi_t tmp),
		  void *opaquev)
{
  RSA_public_key pk;
  gcry_mpi_t result;
  gcry_err_code_t rc;

  (void)algo;
  (void)cmp;
  (void)opaquev;

  pk.n = pkey[0];
  pk.e = pkey[1];
  result = gcry_mpi_new ( 160 );
  public( result, data[0], &pk );
#ifdef IS_DEVELOPMENT_VERSION
  if (DBG_CIPHER)
    {
      log_mpidump ("rsa verify result:", result );
      log_mpidump ("             hash:", hash );
    }
#endif /*IS_DEVELOPMENT_VERSION*/
  if (cmp)
    rc = (*cmp) (opaquev, result);
  else
    rc = mpi_cmp (result, hash) ? GPG_ERR_BAD_SIGNATURE : GPG_ERR_NO_ERROR;
  gcry_mpi_release (result);

  return rc;
}


static unsigned int
rsa_get_nbits (int algo, gcry_mpi_t *pkey)
{
  (void)algo;

  return mpi_get_nbits (pkey[0]);
}


/* Compute a keygrip.  MD is the hash context which we are going to
   update.  KEYPARAM is an S-expression with the key parameters, this
   is usually a public key but may also be a secret key.  An example
   of such an S-expression is:

      (rsa
        (n #00B...#)
        (e #010001#))

   PKCS-15 says that for RSA only the modulus should be hashed -
   however, it is not clear whether this is meant to use the raw bytes
   (assuming this is an unsigned integer) or whether the DER required
   0 should be prefixed.  We hash the raw bytes.  */




/*
     Self-test section.
 */




/* Given an S-expression ENCR_DATA of the form:

   (enc-val
    (rsa
     (a a-value)))

   as returned by gcry_pk_decrypt, return the the A-VALUE.  On error,
   return NULL.  */






/* Run a full self-test for ALGO and return 0 on success.  */




static const char *rsa_names[] =
  {
    "rsa",
    "openpgp-rsa",
    "oid.1.2.840.113549.1.1.1",
    NULL,
  };

gcry_pk_spec_t _gcry_pubkey_spec_rsa =
  {
    "RSA", rsa_names,
    "ne", "nedpqu", "a", "s", "n",
    GCRY_PK_USAGE_SIGN | GCRY_PK_USAGE_ENCR,
    rsa_generate,
    rsa_check_secret_key,
    rsa_encrypt,
    rsa_decrypt,
    rsa_sign,
    rsa_verify,
    rsa_get_nbits,
#ifdef GRUB_UTIL
    .modname = "gcry_rsa",
#endif
  };


GRUB_MOD_INIT(gcry_rsa)
{
  grub_crypto_pk_rsa = &_gcry_pubkey_spec_rsa;
}

GRUB_MOD_FINI(gcry_rsa)
{
  grub_crypto_pk_rsa = 0;
}
