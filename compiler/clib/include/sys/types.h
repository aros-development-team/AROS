#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file sys/types.h
    Lang: English
*/

#include <aros/system.h>
#include <aros/macros.h>

/* Some standard types */
#include <aros/types/size_t.h>
#include <aros/types/int_t.h>

/* These are additions to the types in <stdint.h> */
typedef uint64_t      u_int64_t;      /* 64-bit unsigned integer */
typedef uint32_t      u_int32_t;      /* 32-bit unsigned integer */
typedef uint16_t      u_int16_t;      /* 16-bit unsigned integer */
typedef uint8_t       u_int8_t;       /* 8-bit unsigned integer  */

typedef uint64_t      u_quad_t;
typedef int64_t       quad_t;
typedef quad_t *      qaddr_t;

/*** For compatibility with POSIX source *************************************/

#ifdef __SYSV_VISIBLE
typedef unsigned short  ushort;         /* Sys V compatibility */
typedef unsigned int    uint;           /* Sys V compatibility */
#endif

/*
    Standard POSIX/SUS/ISO C types

    Note that some of these are capable of being defined in multiple header
    files, and need protection from this.
*/

#include <aros/types/blk_t.h> /* blkcnt_t and blksize_t */
#include <aros/types/clockid_t.h>
#include <aros/types/clock_t.h>
#include <aros/types/dev_t.h>
#include <aros/types/fs_t.h>
#include <aros/types/gid_t.h>
#include <aros/types/id_t.h>
#include <aros/types/ino_t.h>
#include <aros/types/key_t.h>
#include <aros/types/mode_t.h>
#include <aros/types/nlink_t.h>
#include <aros/types/off_t.h>
#include <aros/types/pid_t.h>
#include <aros/types/ssize_t.h>
#include <aros/types/socklen_t.h>
#include <aros/types/suseconds_t.h>
#include <aros/types/time_t.h>
#include <aros/types/timer_t.h>
#include <aros/types/uid_t.h>
#include <aros/types/useconds_t.h>

typedef char *                    caddr_t;    /* Core address             */
typedef int32_t                   daddr_t;    /* Disk address             */
typedef uint32_t                  fixpt_t;    /* Fixed point number       */
typedef int64_t                   rlim_t;     /* Resource limit           */
typedef int64_t                   segsz_t;    /* Segment size             */
typedef int32_t                   swblk_t;    /* Swap offset              */

/* These are not supported */
/*
    pthread_attr_t
    pthread_cond_t
    pthread_condattr_t
    pthread_key_t
    pthread_mutex_t
    pthread_mutexattr_t
    pthread_once_t
    pthread_rwlock_t
    pthread_rwlockattr_t
    pthread_t
*/

/*** Macros for endianness conversion ****************************************/

#define __htons(w) AROS_WORD2BE(w)
#define __htonl(l) AROS_LONG2BE(l)
#define __ntohs(w) AROS_BE2WORD(w)
#define __ntohl(l) AROS_BE2LONG(l)

/*** Defines and macros for select() *****************************************/

#define NBBY            8

#ifndef FD_SETSIZE
#define FD_SETSIZE      64
#endif

typedef int32_t       fd_mask;
#define NFDBITS         (sizeof(fd_mask) * NBBY)

#ifndef howmany
#define howmany(x, y)   (((x) + ((y) - 1)) / (y))
#endif

typedef struct fd_set {
        fd_mask fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

#define FD_SET(n, p)    ((p)->fds_bits[(n)/NFDBITS] |=  (1 << ((n) % NFDBITS)))
#define FD_CLR(n, p)    ((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define FD_ISSET(n, p)  ((p)->fds_bits[(n)/NFDBITS] &   (1 << ((n) % NFDBITS)))
#define	FD_COPY(f, t)   memcpy(t, f, sizeof(*(f)))
#define	FD_ZERO(p)      memset(p, 0, sizeof(*(p)))

#endif /* _SYS_TYPES_H_ */

#if __BSD_VISIBLE && !defined(__BSD_SYS_TYPES_H)
#define __BSD_SYS_TYPES_H

/* FIXME: remove temporary hack */
#include <sys/cdefs.h>

typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

#endif /* __BSD_VISIBLE && !__BSD_SYS_TYPES_H */
