/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    fd.library integration for bsdsocket.library.

    When built with -DENABLE_FDLIBRARY the socket API allocates its
    descriptor numbers from fd.library (the system-wide descriptor-number
    authority, shared with posixc.library) and registers a table of network
    operation hooks for FD_OWNER_BSDSOCKET.  A consumer such as posixc's
    read()/write()/close() can then act on a socket descriptor uniformly,
    without knowing it is a socket, by calling through these hooks.

    Without the switch none of this is compiled and bsdsocket keeps its
    pre-existing private descriptor allocation.
*/

#include <conf.h>

#if defined(ENABLE_FDLIBRARY)

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/synch.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/ioctl.h>

#include <aros/libcall.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>

#include <libraries/fd.h>
#include <proto/fd.h>
#include <aros/debug.h>

#include <api/amiga_api.h>
#include <api/amiga_libcallentry.h>
#include <kern/uipc_socket_protos.h>

/* fd.library base, opened once at api_init().  Shared (single instance). */
struct Library *FDBase = NULL;

LONG __CloseSocket(LONG fd, struct SocketBase *libPtr);

/*
 * The fd.library data for a socket descriptor is the struct socket * itself.
 * so->so_pgid holds the owning (per-task) SocketBase, so a hook invoked from
 * another library's context can recover everything it needs from the socket.
 */

/*
 * Return the SocketBase to use as the sleep/lock context for a blocking
 * operation on behalf of the calling task.
 *
 * A blocking socket operation sleeps (tsleep) on the base passed as its
 * process context, and the matching wakeup() signals that base's owner task.
 * Each base carries a single sleeper's worth of state (p_wchan, p_sleep_link,
 * timer), so two tasks must never sleep on one base at the same time.
 *
 * The task issuing the operation may have no base of its own - a posixc
 * worker thread reading an inherited socket descriptor reaches us through
 * fd.library without ever opening bsdsocket.library.  Sleeping such a task on
 * the socket owner's base (so->so_pgid) would let two tasks share one base's
 * sleep state and corrupt the sleep queue.  Give the task its own base
 * instead; a plain OpenLibrary() creates and registers a per-task base, and
 * FindSocketBase() returns it on every later call.
 */
static struct SocketBase *fdh_task_base(struct socket *so)
{
    struct SocketBase *p = FindSocketBase(FindTask(NULL));

    if (p == NULL)
        p = (struct SocketBase *)OpenLibrary("bsdsocket.library", 0);
    if (p == NULL)
        p = (struct SocketBase *)so->so_pgid; /* last resort */

    return p;
}

static SIPTR fdh_sock_read(APTR data, APTR buf, IPTR nbytes, LONG *perror)
{
    struct socket *so = (struct socket *)data;
    struct SocketBase *p = fdh_task_base(so);
    struct uio auio;
    struct iovec aiov;
    struct mbuf *from = NULL, *control = NULL;
    LONG error, flags = 0, len;

    if (nbytes < 0) { *perror = EINVAL; return -1; }

    aiov.iov_base = buf;
    aiov.iov_len = nbytes;
    auio.uio_iov = &aiov;
    auio.uio_iovcnt = 1;
    auio.uio_procp = p;
    auio.uio_resid = nbytes;
    len = nbytes;

    ObtainSyscallSemaphore(p);
    error = soreceive(so, &from, &auio, (struct mbuf **)0, &control, (int *)&flags);
    if (error && auio.uio_resid != len &&
        (error == ERESTART || error == EINTR || error == EWOULDBLOCK))
        error = 0;
    ReleaseSyscallSemaphore(p);

    if (from)
        m_freem(from);
    if (control)
        m_freem(control);

    if (error) { *perror = error; return -1; }
    return (SIPTR)(len - auio.uio_resid);
}

static SIPTR fdh_sock_write(APTR data, CONST_APTR buf, IPTR nbytes, LONG *perror)
{
    struct socket *so = (struct socket *)data;
    struct SocketBase *p = fdh_task_base(so);
    struct uio auio;
    struct iovec aiov;
    LONG error, len;

    if (nbytes < 0) { *perror = EINVAL; return -1; }

    aiov.iov_base = (caddr_t)buf;
    aiov.iov_len = nbytes;
    auio.uio_iov = &aiov;
    auio.uio_iovcnt = 1;
    auio.uio_procp = p;
    auio.uio_resid = nbytes;
    len = nbytes;

    ObtainSyscallSemaphore(p);
    error = sosend(so, (struct mbuf *)0, &auio, (struct mbuf *)0, (struct mbuf *)0, 0);
    if (error && auio.uio_resid != len &&
        (error == ERESTART || error == EINTR || error == EWOULDBLOCK))
        error = 0;
    ReleaseSyscallSemaphore(p);

    if (error) { *perror = error; return -1; }
    return (SIPTR)(len - auio.uio_resid);
}

static LONG fdh_sock_close(APTR data, LONG fd, LONG *perror)
{
    struct socket *so = (struct socket *)data;
    struct SocketBase *p = (struct SocketBase *)so->so_pgid;
    LONG error;

    /* Close exactly this descriptor.  With dup()'d descriptors several fds
       share one socket, so closing must target the requested fd (which
       __CloseSocket() drops from the table, decrements the socket refcount
       and frees the fd.library reservation) - not just any fd for the
       socket. */
    error = __CloseSocket(fd, p);
    if (error) { *perror = error; return -1; }
    return 0;
}

static LONG fdh_sock_dup(APTR data, LONG newfd, LONG *perror)
{
    struct socket *so = (struct socket *)data;
    struct SocketBase *p = (struct SocketBase *)so->so_pgid;
    LONG error;

    if (FDBase == NULL) { *perror = EBADF; return -1; }

    /* Claim newfd for this socket in the system-wide table. */
    error = FD_Reserve(newfd, FD_OWNER_BSDSOCKET, so);
    if (error) { *perror = error; return -1; }

    ObtainSyscallSemaphore(p);
    if ((ULONG)newfd >= p->dTableSize) {
        ReleaseSyscallSemaphore(p);
        FD_Free(newfd, FD_OWNER_BSDSOCKET);
        *perror = EMFILE;
        return -1;
    }
    p->dTable[newfd] = so;
    FD_SET(newfd, (fd_set *)(p->dTable + p->dTableSize));
    so->so_refcnt++;
    ReleaseSyscallSemaphore(p);

    return 0;
}

/*
 * A subset of ioctl() that matters for sockets.  Acting directly on the
 * struct socket avoids the fd lookup __IoctlSocket() would do.  As a small
 * extension, FIONBIO with a NULL argument *queries* the non-blocking flag
 * (returned as the result) so posixc's fcntl(F_GETFL) can report O_NONBLOCK.
 */
static LONG fdh_sock_ioctl(APTR data, IPTR request, APTR arg, LONG *perror)
{
    struct socket *so = (struct socket *)data;

    switch (request)
    {
    case FIONBIO:
        if (arg == NULL)
            return (so->so_state & SS_NBIO) ? 1 : 0;
        if (*(int *)arg)
            so->so_state |= SS_NBIO;
        else
            so->so_state &= ~SS_NBIO;
        return 0;

    case FIONREAD:
        if (arg)
            *(int *)arg = (int)so->so_rcv.sb_cc;
        return 0;

    default:
        *perror = EINVAL;
        return -1;
    }
}

static const struct fd_hooks bsdsocket_fd_hooks =
{
    FD_HOOKS_VERSION,
    fdh_sock_read,
    fdh_sock_write,
    fdh_sock_close,
    NULL,                /* lseek  - sockets are not seekable (ESPIPE) */
    fdh_sock_ioctl,
    NULL,                /* fcntl  - handled posixc-side (O_NONBLOCK)   */
    NULL,                /* fstat  - TODO: S_IFSOCK                     */
    fdh_sock_dup,
};

/* Called from api_init(): open fd.library and publish the network hooks. */
BOOL fdhooks_setup(void)
{
    if (FDBase == NULL)
        FDBase = OpenLibrary("fd.library", 0);

    if (FDBase == NULL)
        return FALSE;

    FD_SetOwnerHooks(FD_OWNER_BSDSOCKET, &bsdsocket_fd_hooks);
    return TRUE;
}

void fdhooks_cleanup(void)
{
    if (FDBase != NULL)
    {
        FD_SetOwnerHooks(FD_OWNER_BSDSOCKET, NULL);
        CloseLibrary(FDBase);
        FDBase = NULL;
    }
}

#endif /* ENABLE_FDLIBRARY */
