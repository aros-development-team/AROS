/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GRUB_TYPES_HEADER
#define GRUB_TYPES_HEADER	1

#include <config.h>
#include <grub/cpu/types.h>

#ifdef GRUB_UTIL
# define GRUB_CPU_SIZEOF_VOID_P	SIZEOF_VOID_P
# define GRUB_CPU_SIZEOF_LONG	SIZEOF_LONG
# ifdef WORDS_BIGENDIAN
#  define GRUB_CPU_WORDS_BIGENDIAN	1
# else
#  undef GRUB_CPU_WORDS_BIGENDIAN
# endif
#else /* ! GRUB_UTIL */
# define GRUB_CPU_SIZEOF_VOID_P	GRUB_HOST_SIZEOF_VOID_P
# define GRUB_CPU_SIZEOF_LONG	GRUB_HOST_SIZEOF_LONG
# ifdef GRUB_HOST_WORDS_BIGENDIAN
#  define GRUB_CPU_WORDS_BIGENDIAN	1
# else
#  undef GRUB_CPU_WORDS_BIGENDIAN
# endif
#endif /* ! GRUB_UTIL */

#if GRUB_CPU_SIZEOF_VOID_P != GRUB_CPU_SIZEOF_LONG
# error "This architecture is not supported because sizeof(void *) != sizeof(long)"
#endif

#if GRUB_CPU_SIZEOF_VOID_P != 4 && GRUB_CPU_SIZEOF_VOID_P != 8
# error "This architecture is not supported because sizeof(void *) != 4 and sizeof(void *) != 8"
#endif

/* Define various wide integers.  */
typedef signed char		grub_int8_t;
typedef short			grub_int16_t;
typedef int			grub_int32_t;
#if GRUB_CPU_SIZEOF_VOID_P == 8
typedef long			grub_int64_t;
#else
typedef long long		grub_int64_t;
#endif

typedef unsigned char		grub_uint8_t;
typedef unsigned short		grub_uint16_t;
typedef unsigned		grub_uint32_t;
#if GRUB_CPU_SIZEOF_VOID_P == 8
typedef unsigned long		grub_uint64_t;
#else
typedef unsigned long long	grub_uint64_t;
#endif

/* Misc types.  */
#if GRUB_HOST_SIZEOF_VOID_P == 8
typedef grub_uint64_t	grub_host_addr_t;
typedef grub_uint64_t	grub_host_off_t;
typedef grub_uint64_t	grub_host_size_t;
typedef grub_int64_t	grub_host_ssize_t;
#else
typedef grub_uint32_t	grub_host_addr_t;
typedef grub_uint32_t	grub_host_off_t;
typedef grub_uint32_t	grub_host_size_t;
typedef grub_int32_t	grub_host_ssize_t;
#endif

#if GRUB_CPU_SIZEOF_VOID_P == 8
typedef grub_uint64_t	grub_addr_t;
typedef grub_uint64_t	grub_off_t;
typedef grub_uint64_t	grub_size_t;
typedef grub_int64_t	grub_ssize_t;
#else
typedef grub_uint32_t	grub_addr_t;
typedef grub_uint32_t	grub_off_t;
typedef grub_uint32_t	grub_size_t;
typedef grub_int32_t	grub_ssize_t;
#endif

/* Byte-orders.  */
#define grub_swap_bytes16(x)	\
({ \
   grub_uint16_t _x = (x); \
   (grub_uint16_t) ((_x << 8) | (_x >> 8)); \
})

#define grub_swap_bytes32(x)	\
({ \
   grub_uint32_t _x = (x); \
   (grub_uint32_t) ((_x << 24) \
                    | ((_x & (grub_uint32_t) 0xFF00UL) << 8) \
                    | ((_x & (grub_uint32_t) 0xFF0000UL) >> 8) \
                    | (_x >> 24)); \
})

#define grub_swap_bytes64(x)	\
({ \
   grub_uint64_t _x = (x); \
   (grub_uint64_t) ((_x << 56) \
                    | ((_x & (grub_uint64_t) 0xFF00ULL) << 40) \
                    | ((_x & (grub_uint64_t) 0xFF0000ULL) << 24) \
                    | ((_x & (grub_uint64_t) 0xFF000000ULL) << 8) \
                    | ((_x & (grub_uint64_t) 0xFF00000000ULL) >> 8) \
                    | ((_x & (grub_uint64_t) 0xFF0000000000ULL) >> 24) \
                    | ((_x & (grub_uint64_t) 0xFF000000000000ULL) >> 40) \
                    | (_x >> 56)); \
})

#ifdef GRUB_CPU_WORDS_BIGENDIAN
# define grub_cpu_to_le16(x)	grub_swap_bytes16(x)
# define grub_cpu_to_le32(x)	grub_swap_bytes32(x)
# define grub_cpu_to_le64(x)	grub_swap_bytes64(x)
# define grub_le_to_cpu16(x)	grub_swap_bytes16(x)
# define grub_le_to_cpu32(x)	grub_swap_bytes32(x)
# define grub_le_to_cpu64(x)	grub_swap_bytes64(x)
# define grub_cpu_to_be16(x)	((grub_uint16_t) (x))
# define grub_cpu_to_be32(x)	((grub_uint32_t) (x))
# define grub_cpu_to_be64(x)	((grub_uint64_t) (x))
# define grub_be_to_cpu16(x)	((grub_uint16_t) (x))
# define grub_be_to_cpu32(x)	((grub_uint32_t) (x))
# define grub_be_to_cpu64(x)	((grub_uint64_t) (x))
#else /* ! WORDS_BIGENDIAN */
# define grub_cpu_to_le16(x)	((grub_uint16_t) (x))
# define grub_cpu_to_le32(x)	((grub_uint32_t) (x))
# define grub_cpu_to_le64(x)	((grub_uint64_t) (x))
# define grub_le_to_cpu16(x)	((grub_uint16_t) (x))
# define grub_le_to_cpu32(x)	((grub_uint32_t) (x))
# define grub_le_to_cpu64(x)	((grub_uint64_t) (x))
# define grub_cpu_to_be16(x)	grub_swap_bytes16(x)
# define grub_cpu_to_be32(x)	grub_swap_bytes32(x)
# define grub_cpu_to_be64(x)	grub_swap_bytes64(x)
# define grub_be_to_cpu16(x)	grub_swap_bytes16(x)
# define grub_be_to_cpu32(x)	grub_swap_bytes32(x)
# define grub_be_to_cpu64(x)	grub_swap_bytes64(x)
#endif /* ! WORDS_BIGENDIAN */

#endif /* ! GRUB_TYPES_HEADER */
