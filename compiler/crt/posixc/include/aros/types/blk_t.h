#ifndef _AROS_TYPES_BLK_T_H
#define _AROS_TYPES_BLK_T_H

/*
    Copyright © 2010-2020, The AROS Development Team. All rights reserved.
    $Id$

    provides the definitions for block device types, namely -:
        blksize_t - the type used to store the size of disk blocks
        blkcnt_t - the type used to count # of disk blocks
    
    on 32bit systems, if __USE_LARGEFILE64 is defined the following are
    also provided -:
        blkcnt64_t - the type used to count # of disk blocks on large devices
*/

#include <aros/cpu.h>

#if (__WORDSIZE==64)
typedef signed AROS_64BIT_TYPE __blksize_t;
typedef signed AROS_64BIT_TYPE __blkcnt_t;
#else
typedef signed AROS_32BIT_TYPE __blksize_t;
typedef signed AROS_32BIT_TYPE __blkcnt_t;
#endif
typedef signed AROS_64BIT_TYPE __blkcnt64_t;

#if defined __USE_UNIX98 && !defined __blksize_t_defined
typedef __blksize_t blksize_t;
# define __blksize_t_defined
#endif

#ifndef __USE_FILE_OFFSET64
# ifndef __blkcnt_t_defined
typedef __blkcnt_t blkcnt_t;         /* Type to count number of disk blocks.  */
#  define __blkcnt_t_defined
# endif
#else
# ifndef __blkcnt_t_defined
typedef __blkcnt64_t blkcnt_t;
#  define __blkcnt_t_defined
# endif
#endif

#ifdef __USE_LARGEFILE64
typedef __blkcnt64_t blkcnt64_t;
#endif

#endif /* _AROS_TYPES_BLK_T_H */
