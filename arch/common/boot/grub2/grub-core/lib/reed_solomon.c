/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#ifdef TEST
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define xmalloc malloc
#define grub_memset memset
#define grub_memcpy memcpy
#endif

#ifndef STANDALONE
#ifdef TEST
typedef unsigned int grub_size_t;
typedef unsigned char grub_uint8_t;
typedef unsigned short grub_uint16_t;
#else
#include <grub/types.h>
#include <grub/reed_solomon.h>
#include <grub/util/misc.h>
#include <grub/misc.h>
#endif
#endif

#ifdef STANDALONE
#ifdef TEST
typedef unsigned int grub_size_t;
typedef unsigned char grub_uint8_t;
typedef unsigned short grub_uint16_t;
#else
#include <grub/types.h>
#include <grub/misc.h>
#endif
void
grub_reed_solomon_recover (void *ptr_, grub_size_t s, grub_size_t rs);
#endif

#define GF_SIZE 8
typedef grub_uint8_t gf_single_t;
typedef grub_uint16_t gf_double_t;
#define GF_POLYNOMIAL 0x1d
#define GF_INVERT2 0x8e
#if defined (STANDALONE) && !defined (TEST)
static char *gf_invert __attribute__ ((section(".text"))) = (void *) 0x100000;
static char *scratch __attribute__ ((section(".text"))) = (void *) 0x100100;
#else
#if defined (STANDALONE)
static char *scratch;
#endif
static grub_uint8_t gf_invert[256];
#endif

#define SECTOR_SIZE 512
#define MAX_BLOCK_SIZE (200 * SECTOR_SIZE)

static gf_single_t
gf_reduce (gf_double_t a)
{
  int i;
  for (i = GF_SIZE - 1; i >= 0; i--)
    if (a & (1ULL << (i + GF_SIZE)))
      a ^= (((gf_double_t) GF_POLYNOMIAL) << i);
  return a & ((1ULL << GF_SIZE) - 1);
}

static gf_single_t
gf_mul (gf_single_t a, gf_single_t b)
{
  gf_double_t res = 0;
  int i;
  for (i = 0; i < GF_SIZE; i++)
    if (b & (1 << i))
      res ^= ((gf_double_t) a) << i;
  return gf_reduce (res);
}

static void
init_inverts (void)
{
  gf_single_t a = 1, ai = 1;
  do
    {
      a = gf_mul (a, 2);
      ai = gf_mul (ai, GF_INVERT2);
      gf_invert[a] = ai;
    }
  while (a != 1);
}

static gf_single_t
pol_evaluate (gf_single_t *pol, grub_size_t degree, gf_single_t x)
{
  int i;
  gf_single_t xn = 1, s = 0;
  for (i = degree; i >= 0; i--)
    {
      s ^= gf_mul (pol[i], xn);
      xn = gf_mul (x, xn);
    }
  return s;
}

#if !defined (STANDALONE)
static void
rs_encode (gf_single_t *data, grub_size_t s, grub_size_t rs)
{
  gf_single_t *rs_polynomial, a = 1;
  int i, j;
  gf_single_t *m;
  m = xmalloc ((s + rs) * sizeof (gf_single_t));
  grub_memcpy (m, data, s * sizeof (gf_single_t));
  grub_memset (m + s, 0, rs * sizeof (gf_single_t));
  rs_polynomial = xmalloc ((rs + 1) * sizeof (gf_single_t));
  grub_memset (rs_polynomial, 0, (rs + 1) * sizeof (gf_single_t));
  rs_polynomial[rs] = 1;
  /* Multiply with X - a^r */
  for (j = 0; j < rs; j++)
    {
      if (a & (1 << (GF_SIZE - 1)))
	{
	  a <<= 1;
	  a ^= GF_POLYNOMIAL;
	}
      else
	a <<= 1;
      for (i = 0; i < rs; i++)
	rs_polynomial[i] = rs_polynomial[i + 1] ^ gf_mul (a, rs_polynomial[i]);
      rs_polynomial[rs] = gf_mul (a, rs_polynomial[rs]);
    }
  for (j = 0; j < s; j++)
    if (m[j])
      {
	gf_single_t f = m[j];
	for (i = 0; i <= rs; i++)
	  m[i+j] ^= gf_mul (rs_polynomial[i], f);
      }
  free (rs_polynomial);
  grub_memcpy (data + s, m + s, rs * sizeof (gf_single_t));
  free (m);
}
#endif

static void
syndroms (gf_single_t *m, grub_size_t s, grub_size_t rs,
	  gf_single_t *sy)
{
  gf_single_t xn = 1;
  unsigned i;
  for (i = 0; i < rs; i++)
    {
      if (xn & (1 << (GF_SIZE - 1)))
	{
	  xn <<= 1;
	  xn ^= GF_POLYNOMIAL;
	}
      else
	xn <<= 1;
      sy[i] = pol_evaluate (m, s + rs - 1, xn);
    }
}

static void
gauss_eliminate (gf_single_t *eq, int n, int m, int *chosen)
{
  int i, j;

  for (i = 0 ; i < n; i++)
    {
      int nzidx;
      int k;
      gf_single_t r;
      for (nzidx = 0; nzidx < m && (eq[i * (m + 1) + nzidx] == 0);
	   nzidx++);
      if (nzidx == m)
	continue;
      chosen[i] = nzidx;
      r = gf_invert [eq[i * (m + 1) + nzidx]];
      for (j = 0; j < m + 1; j++)
	eq[i * (m + 1) + j] = gf_mul (eq[i * (m + 1) + j], r);
      for (j = i + 1; j < n; j++)
	{
	  gf_single_t rr = eq[j * (m + 1) + nzidx];
	  for (k = 0; k < m + 1; k++)
	    eq[j * (m + 1) + k] ^= gf_mul (eq[i * (m + 1) + k], rr);
	}
    }
}

static void
gauss_solve (gf_single_t *eq, int n, int m, gf_single_t *sol)
{
  int *chosen;
  int i, j;

#ifndef STANDALONE
  chosen = xmalloc (n * sizeof (int));
#else
  chosen = (void *) scratch;
  scratch += n * sizeof (int);
#endif
  for (i = 0; i < n; i++)
    chosen[i] = -1;
  for (i = 0; i < m; i++)
    sol[i] = 0;
  gauss_eliminate (eq, n, m, chosen);
  for (i = n - 1; i >= 0; i--)
    {
      gf_single_t s = 0;
      if (chosen[i] == -1)
	continue;
      for (j = 0; j < m; j++)
	s ^= gf_mul (eq[i * (m + 1) + j], sol[j]);
      s ^= eq[i * (m + 1) + m];
      sol[chosen[i]] = s;
    }
#ifndef STANDALONE
  free (chosen);
#else
  scratch -= n * sizeof (int);
#endif
}

static void
rs_recover (gf_single_t *m, grub_size_t s, grub_size_t rs)
{
  grub_size_t rs2 = rs / 2;
  gf_single_t *sigma;
  gf_single_t *errpot;
  int *errpos;
  gf_single_t *sy;
  int errnum = 0;
  int i, j;

#ifndef STANDALONE
  sigma = xmalloc (rs2 * sizeof (gf_single_t));
  errpot = xmalloc (rs2 * sizeof (gf_single_t));
  errpos = xmalloc (rs2 * sizeof (int));
  sy = xmalloc (rs * sizeof (gf_single_t));
#else
  sigma = (void *) scratch;
  scratch += rs2 * sizeof (gf_single_t);
  errpot = (void *) scratch;
  scratch += rs2 * sizeof (gf_single_t);
  errpos = (void *) scratch;
  scratch += rs2 * sizeof (int);
  sy = (void *) scratch;
  scratch += rs * sizeof (gf_single_t);
#endif

  syndroms (m, s, rs, sy);

  {
    gf_single_t *eq;

#ifndef STANDALONE
    eq = xmalloc (rs2 * (rs2 + 1) * sizeof (gf_single_t));
#else
    eq = (void *) scratch;
    scratch += rs2 * (rs2 + 1) * sizeof (gf_single_t);
#endif

    for (i = 0; i < (int) rs; i++)
      if (sy[i] != 0)
	break;

    /* No error detected.  */
    if (i == (int) rs)
      return;

    for (i = 0; i < (int) rs2; i++)
      for (j = 0; j < (int) rs2 + 1; j++)
	eq[i * (rs2 + 1) + j] = sy[i+j];

    for (i = 0; i < (int) rs2; i++)
      sigma[i] = 0;

    gauss_solve (eq, rs2, rs2, sigma);

#ifndef STANDALONE
    free (eq);
#else
    scratch -= rs2 * (rs2 + 1) * sizeof (gf_single_t);
#endif
  } 

  {
    gf_single_t xn = 1, yn = 1;
    for (i = 0; i < (int) (rs + s); i++)
      {
	gf_single_t ev = (gf_mul (pol_evaluate (sigma, rs2 - 1, xn), xn) ^ 1);
	if (ev == 0)
	  {
	    errpot[errnum] = yn;
	    errpos[errnum++] = s + rs - i - 1;
	  }
	yn = gf_mul (yn, 2);
	xn = gf_mul (xn, GF_INVERT2);
      }
  }
  {
    gf_single_t *errvals;
    gf_single_t *eq;

#ifndef STANDALONE
    eq = xmalloc (rs * (errnum + 1) * sizeof (gf_single_t));
    errvals = xmalloc (errnum * sizeof (int));
#else
    eq = (void *) scratch;
    scratch += rs * (errnum + 1) * sizeof (gf_single_t);
    errvals = (void *) scratch;
    scratch += errnum * sizeof (int);
#endif

    for (j = 0; j < errnum; j++)
      eq[j] = errpot[j];
    eq[errnum] = sy[0];
    for (i = 1; i < (int) rs; i++)
      {
	for (j = 0; j < (int) errnum; j++)
	  eq[(errnum + 1) * i + j] = gf_mul (errpot[j],
					     eq[(errnum + 1) * (i - 1) + j]);
	eq[(errnum + 1) * i + errnum] = sy[i];
      }

    gauss_solve (eq, rs, errnum, errvals);

    for (i = 0; i < (int) errnum; i++)
      m[errpos[i]] ^= errvals[i];
#ifndef STANDALONE
    free (eq);
    free (errvals);
#else
    scratch -= rs * (errnum + 1) * sizeof (gf_single_t);
    scratch -= errnum * sizeof (int);
#endif
  }
#ifndef STANDALONE
  free (sigma);
  free (errpot);
  free (errpos);
  free (sy);
#else
  scratch -= rs2 * sizeof (gf_single_t);
  scratch -= rs2 * sizeof (gf_single_t);
  scratch -= rs2 * sizeof (int);
  scratch -= rs * sizeof (gf_single_t);
#endif
}

static void
decode_block (gf_single_t *ptr, grub_size_t s,
	      gf_single_t *rptr, grub_size_t rs)
{
  int i, j;
  for (i = 0; i < SECTOR_SIZE; i++)
    {
      grub_size_t ds = (s + SECTOR_SIZE - 1 - i) / SECTOR_SIZE;
      grub_size_t rr = (rs + SECTOR_SIZE - 1 - i) / SECTOR_SIZE;
      gf_single_t m[ds + rr];

      /* Nothing to do.  */
      if (!ds || !rr)
	continue;

      for (j = 0; j < (int) ds; j++)
	m[j] = ptr[SECTOR_SIZE * j + i];
      for (j = 0; j < (int) rr; j++)
	m[j + ds] = rptr[SECTOR_SIZE * j + i];

      rs_recover (m, ds, rr);

      for (j = 0; j < (int) ds; j++)
	ptr[SECTOR_SIZE * j + i] = m[j];
    }
}

#if !defined (STANDALONE)
static void
encode_block (gf_single_t *ptr, grub_size_t s,
	      gf_single_t *rptr, grub_size_t rs)
{
  int i, j;
  for (i = 0; i < SECTOR_SIZE; i++)
    {
      grub_size_t ds = (s + SECTOR_SIZE - 1 - i) / SECTOR_SIZE;
      grub_size_t rr = (rs + SECTOR_SIZE - 1 - i) / SECTOR_SIZE;
      gf_single_t m[ds + rr];
      for (j = 0; j < ds; j++)
	m[j] = ptr[SECTOR_SIZE * j + i];
      rs_encode (m, ds, rr);
      for (j = 0; j < rr; j++)      
	rptr[SECTOR_SIZE * j + i] = m[j + ds];
    }
}
#endif

#if !defined (STANDALONE)
void
grub_reed_solomon_add_redundancy (void *buffer, grub_size_t data_size,
				  grub_size_t redundancy)
{
  grub_size_t s = data_size;
  grub_size_t rs = redundancy;
  gf_single_t *ptr = buffer;
  gf_single_t *rptr = ptr + s;

  /* Nothing to do.  */
  if (!rs)
    return;

  while (s > 0)
    {
      grub_size_t tt;
      grub_size_t cs, crs;
      cs = s;
      crs = rs;
      tt = cs + crs;
      if (tt > MAX_BLOCK_SIZE)
	{
	  cs = ((cs * (MAX_BLOCK_SIZE / 512)) / tt) * 512;
	  crs = ((crs * (MAX_BLOCK_SIZE / 512)) / tt) * 512;
	}
      encode_block (ptr, cs, rptr, crs);
      ptr += cs;
      rptr += crs;
      s -= cs;
      rs -= crs;
    }
}
#endif

void
grub_reed_solomon_recover (void *ptr_, grub_size_t s, grub_size_t rs)
{
  gf_single_t *ptr = ptr_;
  gf_single_t *rptr = ptr + s;

  /* Nothing to do.  */
  if (!rs)
    return;

#if defined (STANDALONE)
  init_inverts ();
#endif

  while (s > 0)
    {
      grub_size_t tt;
      grub_size_t cs, crs;
      cs = s;
      crs = rs;
      tt = cs + crs;
      if (tt > MAX_BLOCK_SIZE)
	{
	  cs = ((cs * (MAX_BLOCK_SIZE / 512)) / tt) * 512;
	  crs = ((crs * (MAX_BLOCK_SIZE / 512)) / tt) * 512;
	}
      decode_block (ptr, cs, rptr, crs);
      ptr += cs;
      rptr += crs;
      s -= cs;
      rs -= crs;
    }
}

#ifdef TEST
int
main (int argc, char **argv)
{
  FILE *in, *out;
  grub_size_t s, rs;
  char *buf;

#ifdef STANDALONE
  scratch = xmalloc (1048576);
#endif

#ifndef STANDALONE
  init_inverts ();
#endif

  in = fopen ("tst.bin", "rb");
  if (!in)
    return 1;
  fseek (in, 0, SEEK_END);
  s = ftell (in);
  fseek (in, 0, SEEK_SET);
  rs = s / 3;
  buf = xmalloc (s + rs + SECTOR_SIZE);
  fread (buf, 1, s, in);

  grub_reed_solomon_add_redundancy (buf, s, rs);

  out = fopen ("tst_rs.bin", "wb");
  fwrite (buf, 1, s + rs, out);
  fclose (out);

  grub_memset (buf + 512 * 15, 0, 512);

  out = fopen ("tst_dam.bin", "wb");
  fwrite (buf, 1, s + rs, out);
  fclose (out);
  grub_reed_solomon_recover (buf, s, rs);

  out = fopen ("tst_rec.bin", "wb");
  fwrite (buf, 1, s, out);
  fclose (out);

  return 0;
}
#endif
