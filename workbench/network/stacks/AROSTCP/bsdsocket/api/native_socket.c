/* Copyright (C) 2026 John KNIPPER <code@jkn.me>.
 * Licensed under the AROS Public License (APL), Version 1.1.
 * Task-independent socket lifetime over native stack calls and fd leases.
 * Legacy SocketBase descriptor tables and their cleanup are untouched.
 */
#include <conf.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/synch.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <fcntl.h>
#include <aros/libcall.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <libraries/fd.h>
#include <proto/fd.h>
#include <api/amiga_api.h>
#include <api/amiga_libcallentry.h>
#include <api/apicalls.h>
#include <api/allocdatabuffer.h>
#include <api/native_socket.h>
#include <kern/uipc_socket_protos.h>
#include <kern/uipc_socket2_protos.h>

#if defined(ENABLE_FDLIBRARY)
extern struct Library *FDBase;
extern struct Library *MasterSocketBase;
/* One object per socket, one reference per reservation; operation leases delay
 * the retirement callback. No Task or opener base pointer is retained here. */
struct native_socket { struct socket *socket; ULONG descriptors; };
struct native_binding { struct native_socket *object; LONG flags; };
static ULONG native_objects;
static BOOL native_admission;
static LONG native_call(struct aros_socket_request *, APTR);

static LONG native_retired(APTR data, APTR cookie)
{
    struct native_binding *binding=data;
    struct native_socket *object = binding->object;
    LONG error = 0;
    (void)cookie;
    ObtainSemaphore(&syscall_semaphore);
    FreeVec(binding);
    if (--object->descriptors == 0) {
        /* Enabled SO_LINGER is refused by this API: finalization never needs
         * a task's timer/context after the last admitted operation returns. */
        error = releaseSocketRef(object->socket);
        native_objects--;
        FreeVec(object);
        ObtainSemaphore(&baselist_semaphore);
        MasterSocketBase->lib_OpenCnt--;
        ReleaseSemaphore(&baselist_semaphore);
    }
    ReleaseSemaphore(&syscall_semaphore);
    return error;
}

/* Consumes so even when registration fails. Called with syscall exclusion. */
static LONG native_publish(struct socket *so, LONG minimum, LONG *fd)
{
    struct native_socket *object;
    struct native_binding *binding;
    LONG error;
    if (native_objects==(ULONG)~0U) {releaseSocketRef(so);return EOVERFLOW;}
    ObtainSemaphore(&baselist_semaphore);
    if (MasterSocketBase->lib_OpenCnt==(UWORD)~0U) {
        ReleaseSemaphore(&baselist_semaphore);
        releaseSocketRef(so);return EMFILE;
    }
    MasterSocketBase->lib_OpenCnt++;
    ReleaseSemaphore(&baselist_semaphore);
    object = AllocVec(sizeof(*object), MEMF_PUBLIC | MEMF_CLEAR);
    if (!object) {error=ENOMEM;goto failed_hold;}
    binding=AllocVec(sizeof(*binding),MEMF_PUBLIC|MEMF_CLEAR);
    if (!binding) {FreeVec(object);error=ENOMEM;goto failed_hold;}
    binding->object=object;
    object->socket = so;
    object->descriptors = 1;
    error = FD_Alloc(minimum < 3 ? 3 : minimum, FD_OWNER_NATIVE_SOCKET, binding, fd);
    if (error) {FreeVec(binding);FreeVec(object);goto failed_hold;}
    native_objects++;
    return 0;
failed_hold:
    ObtainSemaphore(&baselist_semaphore);
    MasterSocketBase->lib_OpenCnt--;
    ReleaseSemaphore(&baselist_semaphore);
    releaseSocketRef(so);
    return error;
}

static SIPTR native_read(APTR data, LONG fd, APTR buf, IPTR count, LONG *error)
{
    struct aros_socket_request r = {0};
    r.version=AROS_SOCKET_ABI; r.operation=AROS_SOCKET_RECV;
    r.fd=fd; r.buffer=buf; r.length=count;
    *error=native_call(&r,data);
    return *error ? -1 : r.result;
}
static SIPTR native_write(APTR data,LONG fd,CONST_APTR buf,IPTR count,LONG *error)
{
    struct aros_socket_request r = {0};
    r.version=AROS_SOCKET_ABI; r.operation=AROS_SOCKET_SEND;
    r.fd=fd; r.buffer=(APTR)buf; r.length=count;
    *error=native_call(&r,data);
    return *error ? -1 : r.result;
}
static LONG native_close(APTR data,LONG fd,LONG *error)
{
    struct aros_socket_request r = {0};
    r.version=AROS_SOCKET_ABI; r.operation=AROS_SOCKET_CLOSE; r.fd=fd;
    *error=native_call(&r,data); return *error ? -1 : 0;
}
static LONG native_ioctl(APTR data,LONG fd,IPTR command,APTR arg,LONG *error)
{
    struct aros_socket_request r = {0};
    struct fd_lease lease = {0};
    struct native_socket *object;
    LONG result=0;
    if ((ULONG)command == (ULONG)FIONBIO && arg) {
        r.version=AROS_SOCKET_ABI; r.operation=AROS_SOCKET_NONBLOCK;
        r.fd=fd; r.arg0=*(int *)arg;
        *error=native_call(&r,data); return *error ? -1 : 0;
    }
    *error=FD_Acquire(fd,&lease);
    if (*error) return -1;
    if (lease.owner!=FD_OWNER_NATIVE_SOCKET || lease.data!=data) *error=EBADF;
    else {
        object=((struct native_binding *)lease.data)->object;
        ObtainSemaphore(&syscall_semaphore);
        if ((ULONG)command==(ULONG)FIONBIO && !arg)
            result=(object->socket->so_state & SS_NBIO)!=0;
        else if ((ULONG)command==(ULONG)FIONREAD && arg)
            *(int *)arg=object->socket->so_rcv.sb_cc;
        else *error=EINVAL;
        ReleaseSemaphore(&syscall_semaphore);
    }
    FD_Release(&lease);
    return *error ? -1 : result;
}
static LONG native_dup(APTR data,LONG fd,LONG newfd,LONG *error)
{
    struct fd_lease lease={0};
    struct native_socket *object;
    struct native_binding *binding=NULL;
    *error=FD_Acquire(fd,&lease);
    if (*error) return -1;
    if (lease.owner!=FD_OWNER_NATIVE_SOCKET || lease.data!=data) *error=EBADF;
    else {
        object=((struct native_binding *)lease.data)->object;
        ObtainSemaphore(&syscall_semaphore);
        binding=AllocVec(sizeof(*binding),MEMF_PUBLIC|MEMF_CLEAR);
        if (!binding) *error=ENOMEM;
        else if (object->descriptors==(ULONG)~0U) *error=EOVERFLOW;
        else {
            binding->object=object;
            if (!(*error=FD_Reserve(newfd,FD_OWNER_NATIVE_SOCKET,binding)))
                object->descriptors++;
        }
        if (*error && binding) FreeVec(binding);
        ReleaseSemaphore(&syscall_semaphore);
    }
    FD_Release(&lease); return *error ? -1 : 0;
}
static LONG native_fcntl(APTR data,LONG fd,LONG cmd,IPTR arg,LONG *error)
{
    struct aros_socket_request r={0};
    struct fd_lease lease={0};
    struct native_binding *binding;
    LONG result=-1;
    if (cmd==F_DUPFD) {
        r.version=AROS_SOCKET_ABI;r.operation=AROS_SOCKET_CLONE;r.fd=fd;r.arg0=arg;
        *error=native_call(&r,data);return *error?-1:r.result;
    }
    if (cmd==F_SETFL) {
        r.version=AROS_SOCKET_ABI;r.operation=AROS_SOCKET_NONBLOCK;
        r.fd=fd;r.arg0=(arg & O_NONBLOCK)!=0;
        *error=native_call(&r,data);return *error?-1:0;
    }
    *error=FD_Acquire(fd,&lease);
    if (*error) return -1;
    if (lease.owner!=FD_OWNER_NATIVE_SOCKET || lease.data!=data) *error=EBADF;
    else {
        binding=lease.data;
        ObtainSemaphore(&syscall_semaphore);
        switch (cmd) {
        case F_GETFD:result=binding->flags;break;
        case F_SETFD:
            if (arg & ~FD_CLOEXEC) *error=EINVAL;
            else {binding->flags=arg;result=0;}
            break;
        case F_GETFL:result=O_RDWR|((binding->object->socket->so_state & SS_NBIO)?O_NONBLOCK:0);break;
        default:*error=EINVAL;
        }
        ReleaseSemaphore(&syscall_semaphore);
    }
    FD_Release(&lease);return *error?-1:result;
}
static const struct fd_hooks native_hooks={FD_HOOKS_VERSION,
    native_read,native_write,native_close,NULL,native_ioctl,native_fcntl,NULL,native_dup};

BOOL native_socket_setup(void)
{
    BOOL ok=FALSE;
    if (!FDBase || FDBase->lib_Version<51) return FALSE;
    ObtainSemaphore(&syscall_semaphore);
    if (!FD_SetOwnerHooks(FD_OWNER_NATIVE_SOCKET,&native_hooks)) {
        native_admission=TRUE;ok=TRUE;
    }
    ReleaseSemaphore(&syscall_semaphore);
    return ok;
}
BOOL native_socket_withdraw(void)
{
    BOOL ok;
    ObtainSemaphore(&syscall_semaphore);
    ok = native_objects==0 &&
        (!native_admission || !FD_SetOwnerHooks(FD_OWNER_NATIVE_SOCKET,NULL));
    if (ok) native_admission=FALSE;
    ReleaseSemaphore(&syscall_semaphore);
    return ok;
}

/* A batch retains every reservation until the common native wait completes.
 * Scan callbacks run under the stack syscall exclusion. Queueing both socket
 * buffers observes errors/shutdown even when no ordinary events are requested;
 * unrequested data does not turn into a ready result or a retry loop. */
struct native_poll_state {
    struct aros_socket_pollfd *fds;
    struct socket **sockets;
    ULONG count;
};
static int native_poll_scan(struct SocketBase *context, APTR data,
                            int *ready, int *items)
{
    struct native_poll_state *state = data;
    ULONG i;
    spl_t level = splnet();
    *ready = 0;
    *items = 0;
    for (i = 0; i < state->count; i++) {
        struct aros_socket_pollfd *entry = &state->fds[i];
        struct socket *so = state->sockets[i];
        UWORD events = 0;
        if (!so) {
            if (entry->revents) (*ready)++;
            continue;
        }
        if (so->so_error) events |= AROS_POLL_ERR; /* Do not consume SO_ERROR. */
        if (so->so_type == SOCK_STREAM &&
            (so->so_state & (SS_CANTRCVMORE | SS_CANTSENDMORE)) ==
                           (SS_CANTRCVMORE | SS_CANTSENDMORE))
            events |= AROS_POLL_HUP;
        if ((entry->events & AROS_POLL_IN) &&
            (soreadable(so) || (so->so_type == SOCK_DGRAM && so->so_rcv.sb_mb)))
            events |= AROS_POLL_IN;
        if (!(events & AROS_POLL_HUP) && (entry->events & AROS_POLL_OUT) && sowriteable(so)) events |= AROS_POLL_OUT;
        if ((entry->events & AROS_POLL_PRI) &&
            (so->so_oobmark || (so->so_state & SS_RCVATMARK))) events |= AROS_POLL_PRI;
        entry->revents = events;
        if (events) (*ready)++;
        sbselqueue(&so->so_rcv, context);
        sbselqueue(&so->so_snd, context);
        *items += 2;
    }
    splx(level);
    return 0;
}
static LONG native_poll_batch(struct aros_socket_request *request)
{
    struct aros_socket_pollfd *fds = request->buffer;
    struct fd_lease *leases = NULL;
    struct socket **sockets = NULL;
    struct SocketBase *context = NULL;
    struct native_poll_state state;
    struct timeval timeout;
    ULONG i, count;
    LONG error = 0, result;
    BOOL invalid = FALSE;
    request->result = -1;
    if (request->arg1 < -1 || request->length > INT_MAX / 256 ||
        (request->length && !fds)) return EINVAL;
    count = request->length;
    for (i = 0; i < count; i++) fds[i].revents = 0;
    if (count) {
        leases = AllocVec(count * sizeof(*leases), MEMF_ANY | MEMF_CLEAR);
        sockets = AllocVec(count * sizeof(*sockets), MEMF_ANY | MEMF_CLEAR);
        if (!leases || !sockets) { error = ENOMEM; goto done; }
    }
    for (i = 0; i < count; i++) {
        fd_owner_t owner;
        if (fds[i].fd < 0) continue;
        owner = FD_GetOwner(fds[i].fd);
        if (owner != FD_OWNER_NONE && owner != FD_OWNER_NATIVE_SOCKET) {
            error = EOPNOTSUPP; goto done;
        }
        error = FD_Acquire(fds[i].fd, &leases[i]);
        if (error == EBADF) {
            fds[i].revents = AROS_POLL_NVAL;
            invalid = TRUE; error = 0; continue;
        }
        if (error) goto done;
        if (leases[i].owner != FD_OWNER_NATIVE_SOCKET) { error = EOPNOTSUPP; goto done; }
        sockets[i] = ((struct native_binding *)leases[i].data)->object->socket;
    }
    context = api_hookbase_open();
    if (!context) { error = ENOMEM; goto done; }
    context->dTable = sockets;
    context->dTableSize = count;
    state.fds = fds; state.sockets = sockets; state.count = count;
    timeout.tv_sec = request->arg1 < 0 ? 0 : request->arg1 / 1000;
    timeout.tv_usec = request->arg1 < 0 ? 0 : (request->arg1 % 1000) * 1000;
    if (invalid) timeout.tv_sec = timeout.tv_usec = 0;
    result = __WaitNativePoll(count, request->arg1 < 0 && !invalid ? NULL : &timeout,
                             context, native_poll_scan, &state);
    if (result < 0) error = context->defErrno;
    else request->result = result;
    freeDataBuffer(&context->selitems);
    context->dTable = NULL; context->dTableSize = 0;
    api_hookbase_close(context);
done:
    if (leases) for (i = 0; i < count; i++)
        if (leases[i].private_record) FD_Release(&leases[i]);
    FreeVec(sockets); FreeVec(leases);
    return error;
}

/* Every call gets its own sleep state and one private slot. Reusing the
 * stack's ordinary internal calls preserves blocking, UDP, option and
 * readiness behavior without adding its socket to a creator's dTable. */
static LONG native_call(struct aros_socket_request *r, APTR expected)
{
    struct fd_lease lease={0};
    struct native_socket *object;
    struct native_binding *binding;
    struct SocketBase *context;
    struct socket *table[1], *created=NULL;
    LONG error=0, ret=0, fd=-1;
    ULONG size;
    fd_set reads,writes,excepts;
    struct timeval timeout;
    if (!FDBase || FDBase->lib_Version<51) return ENOSYS;
    r->result=-1;
    if (r->operation==AROS_SOCKET_POLL_BATCH) return native_poll_batch(r);
    if (r->operation!=AROS_SOCKET_CREATE) {
        error=FD_Acquire(r->fd,&lease);
        if (error) return error;
        if (lease.owner!=FD_OWNER_NATIVE_SOCKET || (expected && expected!=lease.data)) {
            FD_Release(&lease); return EBADF;
        }
        object=((struct native_binding *)lease.data)->object;
        table[0]=object->socket;
    }
    if (r->operation==AROS_SOCKET_CLOSE) {
        error=FD_Retire(&lease,native_retired,NULL);
        if (!error) error=FD_Release(&lease);
        else FD_Release(&lease);
        if (!error) r->result=0;
        return error;
    }
    if (r->operation==AROS_SOCKET_CLONE) {
        if (r->arg0<0) error=EINVAL;
        else {
            ObtainSemaphore(&syscall_semaphore);
            binding=AllocVec(sizeof(*binding),MEMF_PUBLIC|MEMF_CLEAR);
            if (!binding) error=ENOMEM;
            else if (object->descriptors==(ULONG)~0U) error=EOVERFLOW;
            else {
                binding->object=object;
                error=FD_Alloc(r->arg0<3?3:r->arg0,FD_OWNER_NATIVE_SOCKET,binding,&fd);
                if (!error) {object->descriptors++;r->result=fd;}
            }
            if (error && binding) FreeVec(binding);
            ReleaseSemaphore(&syscall_semaphore);
        }
        FD_Release(&lease); return error;
    }
    context=api_hookbase_open();
    if (!context) { if (lease.private_record) FD_Release(&lease); return ENOMEM; }
    context->dTable=table; context->dTableSize=1;
    switch (r->operation) {
    case AROS_SOCKET_CREATE:
        ObtainSemaphore(&syscall_semaphore);
        if (!native_admission) error=ENOSYS;
        else if ((r->arg1 & ~(SOCK_NONBLOCK))!=SOCK_STREAM &&
                 (r->arg1 & ~(SOCK_NONBLOCK))!=SOCK_DGRAM) error=EPROTOTYPE;
        else {
            error=socreate(r->arg0,&created,r->arg1 & ~SOCK_NONBLOCK,r->arg2);
            if (!error) {
                created->so_refcnt=1;
                if (r->arg1 & SOCK_NONBLOCK) created->so_state|=SS_NBIO;
                error=native_publish(created,3,&fd); r->result=fd;
            }
        }
        ReleaseSemaphore(&syscall_semaphore);
        break;
    case AROS_SOCKET_BIND:
    case AROS_SOCKET_CONNECT:
        if (!r->address || !r->address_length || *r->address_length>INT_MAX) {error=EINVAL;break;}
        ret=r->operation==AROS_SOCKET_BIND ?
            native_bind(0,r->address,*r->address_length,context):
            __connect(0,r->address,*r->address_length,context);
        break;
    case AROS_SOCKET_LISTEN: ret=native_listen(0,r->arg0,context); break;
    case AROS_SOCKET_ACCEPT:
        if (r->address && !r->address_length) {error=EINVAL;break;}
        ret=native_accept(0,r->address,r->address_length,context,&created);
        if (ret>=0) {
            ObtainSemaphore(&syscall_semaphore);
            error=native_publish(created,3,&fd);
            ReleaseSemaphore(&syscall_semaphore);
            ret=fd;
        }
        break;
    case AROS_SOCKET_SEND:
    case AROS_SOCKET_RECV:
        if (r->length>INT_MAX || (!r->buffer && r->length) ||
            (r->address && (!r->address_length || *r->address_length>INT_MAX))) {error=EINVAL;break;}
        if (r->operation==AROS_SOCKET_SEND)
            ret=r->address ? __sendto(0,r->buffer,r->length,r->arg0,
                r->address,*r->address_length,context):__send(0,r->buffer,r->length,r->arg0,context);
        else ret=r->address ? native_recvfrom(0,r->buffer,r->length,r->arg0,
                r->address,(LONG *)r->address_length,context):__recv(0,r->buffer,r->length,r->arg0,context);
        break;
    case AROS_SOCKET_SHUTDOWN: ret=native_shutdown(0,r->arg0,context);break;
    case AROS_SOCKET_GETOPT:
    case AROS_SOCKET_SETOPT:
        if (r->length>INT_MAX || (!r->buffer && r->length)) {error=EINVAL;break;}
        if (r->operation==AROS_SOCKET_SETOPT && r->arg0==SOL_SOCKET && r->arg1==SO_LINGER) {
            if (r->length!=sizeof(struct linger)) {error=EINVAL;break;}
            if (((struct linger *)r->buffer)->l_onoff) {error=ENOPROTOOPT;break;}
        }
        size=r->length;
        ret=r->operation==AROS_SOCKET_GETOPT ?
            native_getsockopt(0,r->arg0,r->arg1,r->buffer,&size,context):
            native_setsockopt(0,r->arg0,r->arg1,r->buffer,size,context);
        if (ret>=0 && r->operation==AROS_SOCKET_GETOPT) r->length=size;
        break;
    case AROS_SOCKET_LOCALNAME:
    case AROS_SOCKET_PEERNAME:
        if (!r->address || !r->address_length) {error=EINVAL;break;}
        ret=r->operation==AROS_SOCKET_LOCALNAME ?
            native_getsockname(0,r->address,r->address_length,context):
            native_getpeername(0,r->address,r->address_length,context);
        break;
    case AROS_SOCKET_NONBLOCK:
        ret=__IoctlSocket(0,FIONBIO,(caddr_t)&r->arg0,context); break;
    case AROS_SOCKET_POLL:
        if (r->arg1 < -1 || r->arg0 & ~7) {error=EINVAL;break;}
        FD_ZERO(&reads);FD_ZERO(&writes);FD_ZERO(&excepts);
        if (r->arg0 & AROS_SOCKET_POLL_READ) FD_SET(0,&reads);
        if (r->arg0 & AROS_SOCKET_POLL_WRITE) FD_SET(0,&writes);
        if (r->arg0 & AROS_SOCKET_POLL_EXCEPTION) FD_SET(0,&excepts);
        timeout.tv_sec=r->arg1/1000;timeout.tv_usec=(r->arg1%1000)*1000;
        ret=__WaitSelect(1,&reads,&writes,&excepts,r->arg1<0?NULL:&timeout,NULL,context);
        if (ret>=0) ret=(FD_ISSET(0,&reads)?1:0)|(FD_ISSET(0,&writes)?2:0)|(FD_ISSET(0,&excepts)?4:0);
        break;
    default: error=ENOSYS;
    }
    if (!error && ret<0) error=context->defErrno;
    if (!error && r->operation!=AROS_SOCKET_CREATE) r->result=ret;
    /* WaitSelect allocates temporary selection storage in this context. */
    freeDataBuffer(&context->selitems);
    context->dTable=NULL; context->dTableSize=0;
    api_hookbase_close(context);
    if (lease.private_record) FD_Release(&lease);
    return error;
}
#endif

AROS_LH2(LONG, NativeSocket,
    AROS_LHA(struct aros_socket_request *, request, A0),
    AROS_LHA(ULONG, size, D0), struct SocketBase *, libPtr, 139, UL)
{
    AROS_LIBFUNC_INIT
    if (!request || size!=sizeof(*request) || request->version!=AROS_SOCKET_ABI)
        return EINVAL;
#if defined(ENABLE_FDLIBRARY)
    if (libPtr->thisTask!=FindTask(NULL)) return EINVAL;
    return native_call(request,NULL);
#else
    return ENOSYS;
#endif
    AROS_LIBFUNC_EXIT
}
