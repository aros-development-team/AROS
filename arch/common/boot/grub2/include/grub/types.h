/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005,2006,2007  Free Software Foundation, Inc.
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

#ifndef GRUB_TYPES_HEADER
#define GRUB_TYPES_HEADER	1

#include <config.h>
#include <grub/cpu/types.h>

#define UNUSED __attribute__ ((unused))

#ifdef GRUB_UTIL
# define GRUB_CPU_SIZEOF_VOID_P	SIZEOF_VOID_P
# define GRUB_CPU_SIZEOF_LONG	SIZEOF_LONG
# ifdef WORDS_BIGENDIAN
#  define GRUB_CPU_WORDS_BIGENDIAN	1
# else
#  undef GRUB_CPU_WORDS_BIGENDIAN
# endif
#else /* ! GRUB_UTIL */
# define GRUB_CPU_SIZEOF_VOID_P	GRUB_TARGET_SIZEOF_VOID_P
# define GRUB_CPU_SIZEOF_LONG	GRUB_TARGET_SIZEOF_LONG
# ifdef GRUB_TARGET_WORDS_BIGENDIAN
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
#if GRUB_TARGET_SIZEOF_VOID_P == 8
typedef grub_uint64_t	grub_target_addr_t;
typedef grub_uint64_t	grub_target_off_t;
typedef grub_uint64_t	grub_target_size_t;
typedef grub_int64_t	grub_target_ssize_t;
#else
typedef grub_uint32_t	grub_target_addr_t;
typedef grub_uint32_t	grub_target_off_t;
typedef grub_uint32_t	grub_target_size_t;
typedef grub_int32_t	grub_target_ssize_t;
#endif

#if GRUB_CPU_SIZEOF_VOID_P == 8
typedef grub_uint64_t	grub_addr_t;
typedef grub_uint64_t	grub_size_t;
typedef grub_int64_t	grub_ssize_t;
#else
typedef grub_uint32_t	grub_addr_t;
typedef grub_uint32_t	grub_size_t;
typedef grub_int32_t	grub_ssize_t;
#endif

#if GRUB_CPU_SIZEOF_VOID_P == 8
# define ULONG_MAX 18446744073709551615UL
#else
# define ULONG_MAX 4294967295UL
#endif

/* The type for representing a file offset.  */
typedef grub_uint64_t	grub_off_t;

/* The type for representing a disk block address.  */
typedef grub_uint64_t	grub_disk_addr_t;

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
# ifdef GRUB_TARGET_WORDS_BIGENDIAN
#  define grub_target_to_host16(x)	((grub_uint16_t) (x))
#  define grub_target_to_host32(x)	((grub_uint32_t) (x))
#  define grub_target_to_host64(x)	((grub_uint64_t) (x))
#  define grub_host_to_target16(x)	((grub_uint16_t) (x))
#  define grub_host_to_target32(x)	((grub_uint32_t) (x))
#  define grub_host_to_target64(x)	((grub_uint64_t) (x))
# else /* ! GRUB_TARGET_WORDS_BIGENDIAN */
#  define grub_target_to_host16(x)	grub_swap_bytes16(x)
#  define grub_target_to_host32(x)	grub_swap_bytes32(x)
#  define grub_target_to_host64(x)	grub_swap_bytes64(x)
#  define grub_host_to_target16(x)	grub_swap_bytes16(x)
#  define grub_host_to_target32(x)	grub_swap_bytes32(x)
#  define grub_host_to_target64(x)	grub_swap_bytes64(x)
# endif
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
# ifdef GRUB_TARGET_WORDS_BIGENDIAN
#  define grub_target_to_host16(x)	grub_swap_bytes16(x)
#  define grub_target_to_host32(x)	grub_swap_bytes32(x)
#  define grub_target_to_host64(x)	grub_swap_bytes64(x)
#  define grub_host_to_target16(x)	grub_swap_bytes16(x)
#  define grub_host_to_target32(x)	grub_swap_bytes32(x)
#  define grub_host_to_target64(x)	grub_swap_bytes64(x)
# else /* ! GRUB_TARGET_WORDS_BIGENDIAN */
#  define grub_target_to_host16(x)	((grub_uint16_t) (x))
#  define grub_target_to_host32(x)	((grub_uint32_t) (x))
#  define grub_target_to_host64(x)	((grub_uint64_t) (x))
#  define grub_host_to_target16(x)	((grub_uint16_t) (x))
#  define grub_host_to_target32(x)	((grub_uint32_t) (x))
#  define grub_host_to_target64(x)	((grub_uint64_t) (x))
# endif
#endif /* ! WORDS_BIGENDIAN */

#endif /* ! GRUB_TYPES_HEADER */
