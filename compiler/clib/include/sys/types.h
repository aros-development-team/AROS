#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file sys/types.h
    Lang: English
*/

#define __AROS_CLIB_TYPES_ONLY

#include <aros/systypes.h>
#include <aros/macros.h>

/* Technically namespace pollution, but what can you do... */
#include <stdint.h>

/*** For compatability with POSIX source *************************************/

#ifndef _POSIX_SOURCE
/* These aren't actually POSIX */
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;
#endif

/* These are additions to the types in <stdint.h> */
typedef UQUAD           u_int64_t;      /* 64-bit unsigned integer */
typedef ULONG           u_int32_t;      /* 32-bit unsigned integer */
typedef UWORD           u_int16_t;      /* 16-bit unsigned integer */
typedef UBYTE           u_int8_t;       /* 8-bit unsigned integer  */

typedef UQUAD           u_quad_t;
typedef QUAD            quad_t;
typedef quad_t *        qaddr_t;


/*
    Standard POSIX/SUS/ISO C types

    Note that some of these are capable of being defined in multiple header
    files, and need protection from this. The header <aros/systypes.h>
    provides the basic definitions.
*/
#ifdef	_AROS_CLOCK_T_
typedef _AROS_CLOCK_T_	    clock_t;
#undef	_AROS_CLOCK_T_
#endif

#ifdef	_AROS_CLOCKID_T_
typedef _AROS_CLOCKID_T_    clockid_t;
#undef	_AROS_CLOCKID_T_
#endif

#ifdef	_AROS_PID_T_
typedef	_AROS_PID_T_	    pid_t;
#undef	_AROS_PID_T_
#endif

/* This is CPU-dependant */
#ifdef	_AROS_PTRDIFF_T_
typedef _AROS_PTRDIFF_T_    ptrdiff_t;
#undef	_AROS_PTRDIFF_T_
#endif

#ifdef	_AROS_SIZE_T_
typedef	_AROS_SIZE_T_	    size_t;
#undef	_AROS_SIZE_T_
#endif

#ifdef	_AROS_SSIZE_T_
typedef	_AROS_SSIZE_T_	    ssize_t;
#undef	_AROS_SSIZE_T_
#endif

#ifdef	_AROS_TIME_T_
typedef	_AROS_TIME_T_	    time_t;
#undef	_AROS_TIME_T_
#endif

#ifdef	_AROS_TIMER_T_
typedef	_AROS_TIMER_T_	    timer_t;
#undef	_AROS_TIMER_T_
#endif

#ifdef	_AROS_OFF_T_
typedef _AROS_OFF_T_	off_t;
#undef	_AROS_OFF_T_
#endif

/* These should only be defined in this header */
typedef _AROS_UID_T_	uid_t;

/* These require this header to be included first */
typedef	LONG		blkcnt_t;	/* File block count        */
typedef LONG		blksize_t;	/* File block size         */
typedef char *          caddr_t;        /* Core address            */
typedef LONG            daddr_t;        /* Disk address            */
typedef LONG            dev_t;          /* Device number           */
typedef LONG		fsblkcnt_t;	/* Filesystem block count  */
typedef LONG		fsfilcnt_t;	/* Filesystem file count   */
typedef ULONG           fixpt_t;        /* Fixed point number      */
typedef _AROS_UID_T_	gid_t;		/* Group id                */
typedef _AROS_UID_T_	id_t;		/* User/Group/Proc id      */
typedef ULONG           ino_t;          /* Inode number            */
typedef LONG            key_t;          /* IPC key (for Sys V IPC) */
typedef UWORD           mode_t;         /* Permissions             */
typedef UWORD           nlink_t;        /* Link count              */
typedef QUAD            rlim_t;         /* Resource limit          */
typedef QUAD            segsz_t;        /* Segment size            */
typedef LONG            swblk_t;        /* Swap offset             */
typedef ULONG           useconds_t;     /* Microseconds            */
typedef LONG            suseconds_t;    /* Microseconds (signed)   */

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

/* XXX */

/* Network related */
typedef ULONG           in_addr_t;
typedef UWORD           in_port_t;
typedef UBYTE           sa_family_t;
typedef UWORD           socklen_t;


/*** Macros for endianness conversion ****************************************/

#define htons(w) AROS_WORD2BE(w)
#define htonl(l) AROS_LONG2BE(l)
#define ntohs(w) AROS_BE2WORD(w)
#define ntohl(l) AROS_BE2LONG(l)

/*** Defines and macros for select() *****************************************/

#define NBBY            8

#ifndef FD_SETSIZE
#define FD_SETSIZE      64
#endif

typedef LONG            fd_mask;
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

#endif /* _SYS_TYPES_H */
