#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file sys/types.h
    Lang: English
*/

#include <sys/_types.h>
#include <sys/cdefs.h>
#include <aros/macros.h>

/* Technically namespace pollution, but what can you do... */
#include <stdint.h>

/*** For compatibility with POSIX source *************************************/

#if __BSD_VISIBLE
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;
#endif

#ifdef __SYSV_VISIBLE
typedef unsigned short  ushort;         /* Sys V compatibility */
typedef unsigned int    uint;           /* Sys V compatibility */
#endif

/* These are additions to the types in <stdint.h> */
typedef __uint64_t      u_int64_t;      /* 64-bit unsigned integer */
typedef __uint32_t      u_int32_t;      /* 32-bit unsigned integer */
typedef __uint16_t      u_int16_t;      /* 16-bit unsigned integer */
typedef __uint8_t       u_int8_t;       /* 8-bit unsigned integer  */

typedef __uint64_t      u_quad_t;
typedef __int64_t       quad_t;
typedef quad_t *        qaddr_t;


/*
    Standard POSIX/SUS/ISO C types

    Note that some of these are capable of being defined in multiple header
    files, and need protection from this.
*/

/*
   stddef.h is part of the freestanding implementation of the C language,
   GCC provides one such header already, by including it we include the 
   definitions of wchar_t, wint_t, size_t, ptrdiff_t and NULL.
   
   POSIX, however, needs us to define some other types, which we do later.
*/

#define __need_size_t
#include <stddef.h>

/* Define the rest of the POSIX types */

#include <sys/types/ssize_t.h>
#include <sys/types/clockid_t.h>
#include <sys/types/clock_t.h>
#include <sys/types/dev_t.h>
#include <sys/types/fs_t.h>
#include <sys/types/gid_t.h>
#include <sys/types/id_t.h>
#include <sys/types/ino_t.h>
#include <sys/types/key_t.h>
#include <sys/types/mode_t.h>
#include <sys/types/nlink_t.h>
#include <sys/types/off_t.h>
#include <sys/types/pid_t.h>
#include <sys/types/socklen_t.h>
#include <sys/types/suseconds_t.h>
#include <sys/types/time_t.h>
#include <sys/types/timer_t.h>
#include <sys/types/uid_t.h>
#include <sys/types/useconds_t.h>

/* These require this header to be included first */
typedef	__blkcnt_t                  blkcnt_t;	/* File block count         */
typedef __blksize_t                 blksize_t;	/* File block size          */

typedef __caddr_t                   caddr_t;    /* Core address             */
typedef __int32_t                   daddr_t;    /* Disk address             */
typedef __uint32_t                  fixpt_t;    /* Fixed point number       */
typedef __int64_t                   rlim_t;     /* Resource limit           */
typedef __int64_t                   segsz_t;    /* Segment size             */
typedef __int32_t                   swblk_t;    /* Swap offset              */

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

typedef __int32_t       fd_mask;
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
