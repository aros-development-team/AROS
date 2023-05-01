#ifndef _AROS_TYPES_FS_T_H
#define _AROS_TYPES_FS_T_H

/*
    Copyright © 2010-2020, The AROS Development Team. All rights reserved.
    $Id$


    provides the definitions for block device types, namely -:
        fsblkcnt_t - the type used to count file system blocks
        fsfilcnt_t - the type used to count file system inodes
    
    on 32bit systems, if __USE_LARGEFILE64 is defined the following are
    also provided -:
        fsblkcnt64_t - the type used to count file system blocks on large filesystems
        fsfilcnt64_t - the type used to count file system inodes on large filesystems
*/

#include <aros/cpu.h>

#if (__WORDSIZE==64)
typedef AROS_64BIT_TYPE __fsblkcnt_t;
typedef AROS_64BIT_TYPE __fsfilcnt_t;
#else
typedef AROS_32BIT_TYPE __fsblkcnt_t;
typedef AROS_32BIT_TYPE __fsfilcnt_t;
#endif
typedef AROS_64BIT_TYPE __fsblkcnt64_t;
typedef AROS_64BIT_TYPE __fsfilcnt64_t;

#ifndef __USE_FILE_OFFSET64
# ifndef __fsblkcnt_t_defined
typedef __fsblkcnt_t fsblkcnt_t; /* Type to count file system blocks.  */
#  define __fsblkcnt_t_defined
# endif
# ifndef __fsfilcnt_t_defined
typedef __fsfilcnt_t fsfilcnt_t; /* Type to count file system inodes.  */
#  define __fsfilcnt_t_defined
# endif
#else
# ifndef __fsblkcnt_t_defined
typedef __fsblkcnt64_t fsblkcnt_t;
#  define __fsblkcnt_t_defined
# endif
# ifndef __fsfilcnt_t_defined
typedef __fsfilcnt64_t fsfilcnt_t;
#  define __fsfilcnt_t_defined
# endif
#endif

#ifdef __USE_LARGEFILE64
typedef __fsblkcnt64_t fsblkcnt64_t;
typedef __fsfilcnt64_t fsfilcnt64_t;
#endif

#endif /* _AROS_TYPES_FS_T_H */
