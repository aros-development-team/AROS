/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#ifndef GRUB_GCRY_WRAP_HEADER
#define GRUB_GCRY_WRAP_HEADER 1

#include <grub/types.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/dl.h>
#include <grub/crypto.h>

#undef WORDS_BIGENDIAN

#ifdef GRUB_CPU_WORDS_BIGENDIAN
#define WORDS_BIGENDIAN 1
#endif

#undef __GNU_LIBRARY__
#define __GNU_LIBRARY__ 1

#define DIM ARRAY_SIZE

typedef grub_uint64_t u64;
typedef grub_uint32_t u32;
typedef grub_uint16_t u16;
typedef grub_uint8_t byte;
typedef grub_size_t size_t;

#define U64_C(c) (c ## ULL)

#define _gcry_burn_stack grub_burn_stack
#define log_error(fmt, args...) grub_dprintf ("crypto", fmt, ## args)


#define PUBKEY_FLAG_NO_BLINDING    (1 << 0)

#define CIPHER_INFO_NO_WEAK_KEY    1

#define HAVE_U64_TYPEDEF 1

typedef union {
    int a;
    short b;
    char c[1];
    long d;
#ifdef HAVE_U64_TYPEDEF
    u64 e;
#endif
    float f;
    double g;
} PROPERLY_ALIGNED_TYPE;

#define gcry_assert(x) grub_assert_real(GRUB_FILE, __LINE__, x)

static inline void
grub_assert_real (const char *file, int line, int cond)
{
  if (!cond)
    grub_fatal ("Assertion failed at %s:%d\n", file, line);
}

/* Selftests are in separate modules.  */
static inline char *
selftest (void)
{
  return NULL;
}

static inline int
fips_mode (void)
{
  return 0;
}

#ifdef GRUB_UTIL
#pragma GCC diagnostic ignored "-Wshadow"
static inline void *
memcpy (void *dest, const void *src, grub_size_t n)
{
  return grub_memcpy (dest, src, n);
}

static inline void *
memset (void *s, int c, grub_size_t n)
{
  return grub_memset (s, c, n);
}

static inline int
memcmp (const void *s1, const void *s2, grub_size_t n)
{
  return grub_memcmp (s1, s2, n);
}
#pragma GCC diagnostic error "-Wshadow"
#endif


#endif
