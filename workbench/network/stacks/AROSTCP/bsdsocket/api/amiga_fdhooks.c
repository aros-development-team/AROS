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


/*
 * The fd.library data for a socket descriptor is the struct socket * itself.
 * The caller read it without a lock, so the socket may have been closed and
 * freed since: a hook first checks, under syscall_semaphore, that fd still
 * leads to it. A socket's descriptors are set and freed under that semaphore,
 * and each holds a reference, so a descriptor that still leads to the socket
 * keeps it alive.
 */

/*
 * Return the SocketBase to use as the sleep/lock context for a blocking
 * operation on behalf of the calling task. fdh_task_done() closes it again
 * when it was opened just for this call.
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
 * sleep state and corrupt the sleep queue.  Give the task a base of its own
 * for the call. It is not kept: nothing would close it when the thread ends,
 * and a later task at the same address would inherit it.
 */
static struct SocketBase *fdh_task_base(BOOL *temp)
{
    struct SocketBase *p = FindSocketBase(FindTask(NULL));

    *temp = FALSE;
    if (p == NULL && (p = api_hookbase_open()) != NULL)
        *temp = TRUE;

    return p;
}

static void fdh_task_done(struct SocketBase *p, BOOL temp)
{
    if (temp)
        api_hookbase_close(p);
}

/* fd still leads to so; the caller holds syscall_semaphore */
static BOOL fdh_valid(LONG fd, struct socket *so)
{
    return so != NULL && FD_GetOwner(fd) == FD_OWNER_BSDSOCKET &&
           FD_GetData(fd) == so;
}

/* The base whose table holds fd; so_pgid need not be it (FIOSETOWN) */
static struct SocketBase *fdh_fd_base(LONG fd, struct socket *so)
{
    extern struct List socketBaseList;
    struct Node *n;
    struct SocketBase *p = NULL;

    ObtainSemaphoreShared(&baselist_semaphore);
    for (n = socketBaseList.lh_Head; n->ln_Succ; n = n->ln_Succ)
    {
        struct SocketBase *b = (struct SocketBase *)n;

        if (b->dTable && (ULONG)fd < b->dTableSize && b->dTable[fd] == so)
        {
            p = b;
            break;
        }
    }
    ReleaseSemaphore(&baselist_semaphore);
    return p;
}

static SIPTR fdh_sock_read(APTR data, LONG fd, APTR buf, IPTR nbytes, LONG *perror)
{
    struct socket *so = (struct socket *)data;
    struct SocketBase *p;
    struct uio auio;
    struct iovec aiov;
    struct mbuf *from = NULL, *control = NULL;
    LONG error, flags = 0, len;
    BOOL temp;

    if (so == NULL) { *perror = EBADF; return -1; }  /* fd without a socket */
    if (nbytes < 0) { *perror = EINVAL; return -1; }
    if ((p = fdh_task_base(&temp)) == NULL) { *perror = ENOMEM; return -1; }

    aiov.iov_base = buf;
    aiov.iov_len = nbytes;
    auio.uio_iov = &aiov;
    auio.uio_iovcnt = 1;
    auio.uio_procp = p;
    auio.uio_resid = nbytes;
    len = nbytes;

    ObtainSyscallSemaphore(p);
    if (fdh_valid(fd, so))
    {
        so->so_refcnt++;    /* the owner may close it while we sleep */
        error = soreceive(so, &from, &auio, (struct mbuf **)0, &control, (int *)&flags);
        if (error && auio.uio_resid != len &&
            (error == ERESTART || error == EINTR || error == EWOULDBLOCK))
            error = 0;
        releaseSocketRef(so);
    }
    else
        error = EBADF;
    ReleaseSyscallSemaphore(p);
    fdh_task_done(p, temp);

    if (from)
        m_freem(from);
    if (control)
        m_freem(control);

    if (error) { *perror = error; return -1; }
    return (SIPTR)(len - auio.uio_resid);
}

static SIPTR fdh_sock_write(APTR data, LONG fd, CONST_APTR buf, IPTR nbytes, LONG *perror)
{
    struct socket *so = (struct socket *)data;
    struct SocketBase *p;
    struct uio auio;
    struct iovec aiov;
    LONG error, len;
    BOOL temp;

    if (so == NULL) { *perror = EBADF; return -1; }  /* fd without a socket */
    if (nbytes < 0) { *perror = EINVAL; return -1; }
    if ((p = fdh_task_base(&temp)) == NULL) { *perror = ENOMEM; return -1; }

    aiov.iov_base = (caddr_t)buf;
    aiov.iov_len = nbytes;
    auio.uio_iov = &aiov;
    auio.uio_iovcnt = 1;
    auio.uio_procp = p;
    auio.uio_resid = nbytes;
    len = nbytes;

    ObtainSyscallSemaphore(p);
    if (fdh_valid(fd, so))
    {
        so->so_refcnt++;    /* the owner may close it while we sleep */
        error = sosend(so, (struct mbuf *)0, &auio, (struct mbuf *)0, (struct mbuf *)0, 0);
        if (error && auio.uio_resid != len &&
            (error == ERESTART || error == EINTR || error == EWOULDBLOCK))
            error = 0;
        releaseSocketRef(so);
    }
    else
        error = EBADF;
    ReleaseSyscallSemaphore(p);
    fdh_task_done(p, temp);

    if (error) { *perror = error; return -1; }
    return (SIPTR)(len - auio.uio_resid);
}

static LONG fdh_sock_close(APTR data, LONG fd, LONG *perror)
{
    struct socket *so = (struct socket *)data;
    struct SocketBase *p;
    LONG error;

    if (so == NULL) { *perror = EBADF; return -1; }  /* fd without a socket */

    /* Close exactly this descriptor.  With dup()'d descriptors several fds
       share one socket, so closing must target the requested fd (which
       closeSocketLocked() drops from the table, decrements the socket
       refcount and frees the fd.library reservation) - not just any fd for
       the socket.  The caller may be another thread than the owner, so
       CloseSocket()'s own-task check and priority boost do not apply. */
    ObtainSemaphore(&syscall_semaphore);
    p = fdh_fd_base(fd, so);
    error = p ? closeSocketLocked(fd, p) : EBADF;
    ReleaseSemaphore(&syscall_semaphore);
    if (error) { *perror = error; return -1; }
    return 0;
}

static LONG fdh_sock_dup(APTR data, LONG fd, LONG newfd, LONG *perror)
{
    struct socket *so = (struct socket *)data;
    struct SocketBase *p;
    LONG error = 0;

    if (so == NULL) { *perror = EBADF; return -1; }  /* fd without a socket */
    if (FDBase == NULL) { *perror = EBADF; return -1; }

    /* Not ObtainSyscallSemaphore(p): p may be another task's base */
    ObtainSemaphore(&syscall_semaphore);
    /* newfd joins the table that holds fd, which also proves so alive */
    if ((p = fdh_fd_base(fd, so)) == NULL || p->closing)
        error = EBADF;
    else
    {
        /* Numbers are system-wide: grow the table as sdFind() does */
        if ((ULONG)newfd >= p->dTableSize && newfd < 0xFFC0)
            setdtablesize(p, (newfd / 64 + 1) * 64);
        if ((ULONG)newfd >= p->dTableSize)
            error = EMFILE;
        /* Claim newfd for this socket in the system-wide table. */
        else if ((error = FD_Reserve(newfd, FD_OWNER_BSDSOCKET, so)) == 0)
        {
            p->dTable[newfd] = so;
            FD_SET(newfd, (fd_set *)(p->dTable + p->dTableSize));
            so->so_refcnt++;
        }
    }
    ReleaseSemaphore(&syscall_semaphore);

    if (error) { *perror = error; return -1; }
    return 0;
}

/*
 * A subset of ioctl() that matters for sockets.  Acting directly on the
 * struct socket avoids the fd lookup __IoctlSocket() would do.  As a small
 * extension, FIONBIO with a NULL argument *queries* the non-blocking flag
 * (returned as the result) so posixc's fcntl(F_GETFL) can report O_NONBLOCK.
 */
static LONG fdh_sock_ioctl(APTR data, LONG fd, IPTR request, APTR arg, LONG *perror)
{
    struct socket *so = (struct socket *)data;
    LONG result = 0, error = 0;

    ObtainSemaphore(&syscall_semaphore);
    if (!fdh_valid(fd, so))
        error = EBADF;
    else switch (request)
    {
    case FIONBIO:
        if (arg == NULL)
            result = (so->so_state & SS_NBIO) ? 1 : 0;
        else
        {
            spl_t s = splnet();     /* the daemon writes so_state too */

            if (*(int *)arg)
                so->so_state |= SS_NBIO;
            else
                so->so_state &= ~SS_NBIO;
            splx(s);
        }
        break;

    case FIONREAD:
        if (arg)
            *(int *)arg = (int)so->so_rcv.sb_cc;
        break;

    default:
        error = EINVAL;
        break;
    }
    ReleaseSemaphore(&syscall_semaphore);

    if (error) { *perror = error; return -1; }
    return result;
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
