#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI-C header file sys/types.h
    Lang: English
*/

#include <exec/types.h>
#include <aros/macros.h>

#if !defined(_SIZE_T) && !defined(__typedef_size_t)
#   define __typedef_size_t
#   define _SIZE_T
#   ifdef AMIGA
        typedef unsigned long size_t;
#   else
        /* Must be int and not long. Otherwise gcc will complain */
        typedef unsigned int size_t;
#   endif
#endif

#if !defined(_SSIZE_T) && !defined(__typedef_ssize_t)
#define __typedef_ssize_t
#define _SSIZE_T
typedef int ssize_t;
#endif

#if defined(__FreeBSD__) || defined(_AMIGA)
#if !defined(_TIME_T) && !defined(__typedef_time_t)
#define _TIME_T
typedef long time_t;
#endif
#endif

#if !defined(__typedef_pid_t)
#define __typedef_pid_t
typedef int pid_t;
#endif

#if !defined(__typedef_off_t)
#define __typedef_off_t
typedef LONG off_t;             /* Is this correct? */
#endif

/*** For compatability with POSIX source *************************************/

typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

typedef QUAD            int64_t;        /* 64-bit signed integer   */
typedef UQUAD           u_int64_t;      /* 64-bit unsigned integer */
typedef LONG            int32_t;        /* 32-bit signed integer   */
typedef ULONG           u_int32_t;      /* 32-bit unsigned integer */
typedef WORD            int16_t;        /* 16-bit signed integer   */
typedef UWORD           u_int16_t;      /* 16-bit unsigned integer */
typedef BYTE            int8_t;         /* 8-bit signed integer    */
typedef UBYTE           u_int8_t;       /* 8-bit unsigned integer  */

typedef UQUAD           u_quad_t;
typedef QUAD            quad_t;
typedef quad_t *        qaddr_t;

/* Network related */
typedef ULONG           in_addr_t;
typedef UWORD           in_port_t;
typedef UBYTE           sa_family_t;
typedef UWORD           socklen_t;

/* Most of these will probably never be used under AROS,
   since they seem to be very *nix-centric. */
typedef char *          caddr_t;        /* Core address            */
typedef LONG            daddr_t;        /* Disk address            */
typedef LONG            dev_t;          /* Device number           */
typedef ULONG           fixpt_t;        /* Fixed point number      */
typedef ULONG           gid_t;          /* Group id                */
typedef ULONG           ino_t;          /* Inode number            */
typedef LONG            key_t;          /* IPC key (for Sys V IPC) */
typedef UWORD           mode_t;         /* Permissions             */
typedef UWORD           nlink_t;        /* Link count              */
typedef QUAD            rlim_t;         /* Resource limit          */
typedef LONG            segsz_t;        /* Segment size            */
typedef LONG            swblk_t;        /* Swap offset             */
typedef ULONG           uid_t;          /* User ID                 */
typedef ULONG           useconds_t;     /* Microseconds            */
typedef LONG            suseconds_t;    /* Microseconds (signed)   */

/* this is actually cpu-dependant */
#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef LONG            ptrdiff_t;
#endif

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

