/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unix filedescriptor/socket IO include file
*/

#ifndef UXIO_H
#define UXIO_H

#define timeval sys_timeval

#include <sys/types.h>
#include <poll.h>

#undef timeval

#include <exec/tasks.h>
#include <exec/ports.h>
#include <dos/bptr.h>
#include <hidd/unixio.h>
#include <oop/oop.h>
#include <proto/exec.h>

/* Android is not a real Linux :-) */
#ifdef HOST_OS_android
#undef HOST_OS_linux
#endif

#ifdef HOST_OS_linux
#define LIBC_NAME "libc.so.6"
#endif

#ifdef HOST_OS_darwin
#define LIBC_NAME "libSystem.dylib"
#endif

#ifndef LIBC_NAME
#define LIBC_NAME "libc.so"
#endif

struct UnixIO_Waiter
{
    struct Task *task;
    BYTE	 signal;
};

struct LibCInterface
{
    int	    (*open)(const char *path, int oflag, ...);
    int	    (*close)(int filedes);
    int	    (*ioctl)(int d, int request, ...);
    int	    (*fcntl)(int fd, int cmd, ...);
    int     (*poll)(struct pollfd *fds, nfds_t nfds, int timeout);
    ssize_t (*read)(int fd, void *buf, size_t count);
    ssize_t (*write)(int fildes, const void *buf, size_t nbyte);
    pid_t   (*getpid)(void);
    int	   *(*__error)(void);
    void    *(*mmap)(void *addr, size_t len, int prot, int flags, int fd, __off_t offset);
    int     (*munmap)(void *addr, size_t len);
};

/* For simplicity, our library base is our static data */
struct unixio_base
{
    struct UnixIOBase	   uio_Public;		/* Public portion				*/
    OOP_AttrBase	   UnixIOAB;		/* Our attribute base	    			*/
    OOP_Class		  *uio_unixioclass;	/* Our class		    			*/
    OOP_Object		  *obj;			/* Our singletone	    			*/
    APTR		   irqHandle;		/* SIGIO IRQ handle	    			*/
    APTR		   KernelBase;		/* Resource bases	    			*/
    APTR		   HostLibBase;
    STRPTR		   SystemArch;		/* System architecture string (cached)		*/
    struct LibCInterface  *SysIFace;		/* Our libc interface				*/
    pid_t		   aros_PID;		/* PID of AROS process (for F_SETOWN fcntl)	*/
    struct MinList	   intList;		/* User's interrupts list			*/
    struct SignalSemaphore lock;		/* Singletone creation lock			*/
};

#define UD(cl) ((struct unixio_base *)cl->UserData)

#endif /* UXIO */
