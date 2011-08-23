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

/* static data for the unixioclass */
struct uio_data
{
    APTR		    KernelBase;
    APTR		    HostLibBase;
    STRPTR		    SystemArch;
    OOP_AttrBase	    UnixIOAB;
    struct LibCInterface   *SysIFace;
    int		   	   *errnoPtr;
    pid_t		    aros_PID;
    OOP_Object		   *obj;
    struct MinList	    intList;
    struct MsgPort	   *ud_Port;
    struct SignalSemaphore  lock;
};

struct LibCInterface
{
    int	    (*open)(char *path, int oflag, ...);
    int	    (*close)(int filedes);
    int	    (*ioctl)(int d, int request, ...);
    int	    (*fcntl)(int fd, int cmd, ...);
    int     (*poll)(struct pollfd *fds, nfds_t nfds, int timeout);
    ssize_t (*read)(int fd, void *buf, size_t count);
    ssize_t (*write)(int fildes, const void *buf, size_t nbyte);
    pid_t   (*getpid)(void);
    int	   *(*__error)(void);
};

struct unixio_base
{
    struct UnixIOBase	  uio_Public;
    BPTR		  uio_SegList;
    OOP_Class		 *uio_unixioclass;
    APTR		  irqHandle;
    struct uio_data	  uio_csd;
};

#define UD(cl) (&((struct unixio_base *)cl->UserData)->uio_csd)

#endif /* UXIO */
