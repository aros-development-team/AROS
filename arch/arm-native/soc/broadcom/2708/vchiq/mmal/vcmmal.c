/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    vcmmal: an MMAL client over vchiq.library, after the protocol of
    raspberrypi/userland interface/mmal/vc/mmal_vc_client.c.

    Every pointer the protocol carries is a cookie into a per-client
    table. Replies to synchronous calls are handed over on the VCHIQ slot
    handler task; buffers and events go to a worker task of the client,
    which can block in the bulk receives they need.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <dos/dos.h>
#include <proto/exec.h>

/* Our own base, so a program's VCHIQBase does not clash */
#define VCHIQBase vcmmal_VCHIQBase
#include <proto/vchiq.h>

#include <string.h>

#include "vcmmal_msgs.h"

struct Library *VCHIQBase;

#define VCMMAL_COOKIES          512
#define VCMMAL_WORKER_STACK     (16 * 1024)
#define VCMMAL_WORKER_PRI       4

#define SIGF_WORKER             SIGBREAKF_CTRL_E
#define SIGF_QUIT               SIGBREAKF_CTRL_C

struct VCMMALQueued {
    struct MinNode node;
    VCHIQ_HEADER_T *header;
};

struct VCMMAL {
    struct Library *vchiqbase;
    VCHIQ_INSTANCE_T instance;
    VCHIQ_SERVICE_HANDLE_T service;

    /* A message and its bulk transfer must reach the queue together */
    struct SignalSemaphore send_lock;

    struct SignalSemaphore cookie_lock;
    APTR cookies[VCMMAL_COOKIES];
    ULONG cookie_next;

    struct Task *worker;
    struct Task *opener;
    struct SignalSemaphore queue_lock;
    struct MinList queue;
};

struct VCMMALWaiter {
    struct Task *task;
    ULONG sigmask;
    volatile BOOL done;
    void *dest;
    ULONG destlen;
};

/*
 * Cookies
 */
static ULONG cookie_add(struct VCMMAL *mmal, APTR p)
{
    ULONG i, cookie = 0;

    ObtainSemaphore(&mmal->cookie_lock);
    for (i = 0; i < VCMMAL_COOKIES; i++)
    {
        ULONG n = (mmal->cookie_next + i) % VCMMAL_COOKIES;

        if (!mmal->cookies[n])
        {
            mmal->cookies[n] = p;
            mmal->cookie_next = n + 1;
            cookie = n + 1;
            break;
        }
    }
    ReleaseSemaphore(&mmal->cookie_lock);

    return cookie;
}

static APTR cookie_get(struct VCMMAL *mmal, ULONG cookie)
{
    APTR p = NULL;

    if (cookie >= 1 && cookie <= VCMMAL_COOKIES)
    {
        ObtainSemaphore(&mmal->cookie_lock);
        p = mmal->cookies[cookie - 1];
        ReleaseSemaphore(&mmal->cookie_lock);
    }

    return p;
}

static void cookie_del(struct VCMMAL *mmal, ULONG cookie)
{
    if (cookie >= 1 && cookie <= VCMMAL_COOKIES)
    {
        ObtainSemaphore(&mmal->cookie_lock);
        mmal->cookies[cookie - 1] = NULL;
        ReleaseSemaphore(&mmal->cookie_lock);
    }
}

static struct VCMMALPort *port_by_number(struct VCMMALComponent *comp,
                                         ULONG type, ULONG num)
{
    switch (type)
    {
    case MMAL_PORT_TYPE_CONTROL:
        return &comp->control;
    case MMAL_PORT_TYPE_INPUT:
        return (num < comp->input_num) ? &comp->input[num] : NULL;
    case MMAL_PORT_TYPE_OUTPUT:
        return (num < comp->output_num) ? &comp->output[num] : NULL;
    case MMAL_PORT_TYPE_CLOCK:
        return (num < comp->clock_num) ? &comp->clock[num] : NULL;
    }
    return NULL;
}

/*
 * Worker task: buffers and events from the VideoCore
 */
static void worker_buffer(struct VCMMAL *mmal, VCHIQ_HEADER_T *header)
{
    mmal_worker_buffer_from_host *msg = (mmal_worker_buffer_from_host *)header->data;
    struct VCMMALBuffer *buf = cookie_get(mmal, msg->drvbuf.client_context);
    struct VCMMALPort *port;
    ULONG len;

    if (!buf)
    {
        bug("[VCMMAL] buffer for unknown cookie %u\n", msg->drvbuf.client_context);
        VCHIQReleaseMessage(mmal->service, header);
        return;
    }
    port = buf->port;

    buf->length = msg->buffer_header.length;
    buf->offset = msg->buffer_header.offset;
    buf->flags = msg->buffer_header.flags;
    buf->pts = msg->buffer_header.pts;
    buf->dts = msg->buffer_header.dts;
    buf->video = msg->buffer_header_type_specific;

    if (!msg->is_zero_copy &&
        (buf->length || (buf->flags & MMAL_BUFFER_HEADER_FLAG_EOS)))
    {
        if (msg->payload_in_message)
        {
            len = msg->payload_in_message;
            if (len > MMAL_VC_SHORT_DATA || len > buf->alloc_size)
                len = 0;
            memcpy(buf->data, msg->short_data, len);
            buf->offset = 0;
            buf->length = len;
        }
        else
        {
            /* The VideoCore sends whole words */
            len = (buf->length + 3) & ~3;
            if (!len)
                len = 8;

            if (buf->offset + len > buf->alloc_size)
            {
                /* Drain the transfer, or the next one lands out of order */
                UBYTE *scratch = AllocVec(len, MEMF_ANY);

                if (scratch)
                {
                    VCHIQBulkReceive(mmal->service, scratch, len, NULL,
                                     VCHIQ_BULK_MODE_BLOCKING);
                    FreeVec(scratch);
                }
                buf->length = 0;
                buf->flags |= MMAL_BUFFER_HEADER_FLAG_TRANSMISSION_FAILED;
            }
            else if (VCHIQBulkReceive(mmal->service, buf->data + buf->offset,
                                      len, NULL, VCHIQ_BULK_MODE_BLOCKING)
                     != VCHIQ_SUCCESS)
            {
                buf->length = 0;
                buf->flags |= MMAL_BUFFER_HEADER_FLAG_TRANSMISSION_FAILED;
            }
        }
    }

    VCHIQReleaseMessage(mmal->service, header);
    cookie_del(mmal, buf->cookie);
    buf->cookie = 0;

    D(bug("[VCMMAL] buffer back: %u bytes, flags 0x%x\n",
          (unsigned)buf->length, (unsigned)buf->flags));

    if (port && port->buffer_cb)
        port->buffer_cb(port, buf);
}

static void worker_event(struct VCMMAL *mmal, VCHIQ_HEADER_T *header)
{
    mmal_worker_event_to_host *msg = (mmal_worker_event_to_host *)header->data;
    struct VCMMALComponent *comp = cookie_get(mmal, msg->client_component);
    struct VCMMALPort *port = comp ? port_by_number(comp, msg->port_type, msg->port_num) : NULL;
    ULONG cmd = msg->cmd, length = msg->length;
    UBYTE *data;

    data = AllocVec(((length + 3) & ~3) + 4, MEMF_ANY | MEMF_CLEAR);
    if (data)
    {
        if (length > MMAL_WORKER_EVENT_SPACE)
            VCHIQBulkReceive(mmal->service, data, (length + 3) & ~3, NULL,
                             VCHIQ_BULK_MODE_BLOCKING);
        else
            memcpy(data, msg->data, length);
    }
    VCHIQReleaseMessage(mmal->service, header);

    D(bug("[VCMMAL] event 0x%08x, %u bytes, port %u:%u\n",
          (unsigned)cmd, (unsigned)length, (unsigned)msg->port_type,
          (unsigned)msg->port_num));

    if (data && port && comp->event_cb)
        comp->event_cb(port, cmd, data, length);

    FreeVec(data);
}

static void vcmmal_worker(struct VCMMAL *mmal)
{
    struct VCMMALQueued *q;
    ULONG sigs;

    Signal(mmal->opener, SIGF_SINGLE);

    for (;;)
    {
        sigs = Wait(SIGF_WORKER | SIGF_QUIT);

        for (;;)
        {
            mmal_worker_msg_header *msg;

            ObtainSemaphore(&mmal->queue_lock);
            q = (struct VCMMALQueued *)RemHead((struct List *)&mmal->queue);
            ReleaseSemaphore(&mmal->queue_lock);
            if (!q)
                break;

            msg = (mmal_worker_msg_header *)q->header->data;
            if (msg->msgid == MMAL_WORKER_BUFFER_TO_HOST)
                worker_buffer(mmal, q->header);
            else
                worker_event(mmal, q->header);
            FreeVec(q);
        }

        if (sigs & SIGF_QUIT)
            break;
    }

    Forbid();
    Signal(mmal->opener, SIGF_SINGLE);
}

/*
 * VCHIQ callback, on the slot handler task: must not block
 */
static VCHIQ_STATUS_T vcmmal_callback(VCHIQ_REASON_T reason,
                                      VCHIQ_HEADER_T *header,
                                      VCHIQ_SERVICE_HANDLE_T service,
                                      void *bulk_userdata)
{
    struct VCMMAL *mmal = VCHIQGetServiceUserdata(service);
    mmal_worker_msg_header *msg;

    if (reason != VCHIQ_MESSAGE_AVAILABLE)
        return VCHIQ_SUCCESS;

    msg = (mmal_worker_msg_header *)header->data;
    if (msg->magic != MMAL_MAGIC)
    {
        bug("[VCMMAL] message without magic (0x%08x)\n", msg->magic);
        VCHIQReleaseMessage(service, header);
        return VCHIQ_SUCCESS;
    }

    if ((msg->msgid == MMAL_WORKER_BUFFER_TO_HOST) ||
        (msg->msgid == MMAL_WORKER_EVENT_TO_HOST))
    {
        struct VCMMALQueued *q = AllocVec(sizeof(*q), MEMF_ANY);

        if (!q)
        {
            VCHIQReleaseMessage(service, header);
            return VCHIQ_SUCCESS;
        }
        q->header = header;
        ObtainSemaphore(&mmal->queue_lock);
        AddTail((struct List *)&mmal->queue, (struct Node *)&q->node);
        ReleaseSemaphore(&mmal->queue_lock);
        Signal(mmal->worker, SIGF_WORKER);
    }
    else
    {
        struct VCMMALWaiter *w;
        ULONG len;

        /* Signal under the lock: the waiter unlinks itself under it too */
        ObtainSemaphore(&mmal->cookie_lock);
        w = (msg->waiter >= 1 && msg->waiter <= VCMMAL_COOKIES)
            ? mmal->cookies[msg->waiter - 1] : NULL;
        if (w)
        {
            len = (header->size < w->destlen) ? header->size : w->destlen;
            memcpy(w->dest, msg, len);
            w->destlen = len;
            w->done = TRUE;
            Signal(w->task, w->sigmask);
        }
        ReleaseSemaphore(&mmal->cookie_lock);

        if (!w)
            bug("[VCMMAL] reply %u for no waiter\n", msg->msgid);
        VCHIQReleaseMessage(service, header);
    }

    return VCHIQ_SUCCESS;
}

/* Send a message and wait for the reply, which lands in reply */
static LONG sendwait(struct VCMMAL *mmal, mmal_worker_msg_header *msg,
                     ULONG size, ULONG msgid, void *reply, ULONG replylen)
{
    struct VCMMALWaiter w;
    BYTE sig;
    ULONG cookie;
    LONG status;

    sig = AllocSignal(-1);
    w.task = FindTask(NULL);
    w.sigmask = (sig != -1) ? (1UL << sig) : SIGF_SINGLE;
    w.done = FALSE;
    w.dest = reply;
    w.destlen = replylen;
    if (sig == -1)
        SetSignal(0, SIGF_SINGLE);

    cookie = cookie_add(mmal, &w);
    if (!cookie)
    {
        if (sig != -1)
            FreeSignal(sig);
        return MMAL_ENOSPC;
    }

    msg->magic = MMAL_MAGIC;
    msg->msgid = msgid;
    msg->control_service = 0;
    msg->waiter = cookie;
    msg->status = 0;
    msg->dummy = 0;

    ObtainSemaphore(&mmal->send_lock);
    status = VCHIQQueueMessage(mmal->service, msg, size);
    ReleaseSemaphore(&mmal->send_lock);

    if (status == VCHIQ_SUCCESS)
    {
        while (!w.done)
            Wait(w.sigmask);
    }

    cookie_del(mmal, cookie);
    if (sig != -1)
    {
        SetSignal(0, w.sigmask);
        FreeSignal(sig);
    }

    return (status == VCHIQ_SUCCESS) ? MMAL_SUCCESS : MMAL_EIO;
}

/*
 * Client
 */
struct VCMMAL *VCMMAL_Open(void)
{
    VCHIQ_SERVICE_PARAMS_T params;
    struct VCMMAL *mmal;

    mmal = AllocVec(sizeof(*mmal), MEMF_ANY | MEMF_CLEAR);
    if (!mmal)
        return NULL;

    mmal->vchiqbase = OpenLibrary("vchiq.library", 1);
    if (!mmal->vchiqbase)
        goto fail_free;
    VCHIQBase = mmal->vchiqbase;

    InitSemaphore(&mmal->send_lock);
    InitSemaphore(&mmal->cookie_lock);
    InitSemaphore(&mmal->queue_lock);
    NEWLIST((struct List *)&mmal->queue);

    if (VCHIQInitialise(&mmal->instance) != VCHIQ_SUCCESS)
        goto fail_lib;
    if (VCHIQConnect(mmal->instance) != VCHIQ_SUCCESS)
        goto fail_instance;

    /* Before the service opens: messages may arrive right away */
    mmal->opener = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    mmal->worker = NewCreateTask(TASKTAG_PC,        vcmmal_worker,
                                 TASKTAG_NAME,      "VCMMAL worker",
                                 TASKTAG_STACKSIZE, VCMMAL_WORKER_STACK,
                                 TASKTAG_PRI,       VCMMAL_WORKER_PRI,
                                 TASKTAG_ARG1,      mmal,
                                 TAG_DONE);
    if (!mmal->worker)
        goto fail_instance;
    Wait(SIGF_SINGLE);

    memset(&params, 0, sizeof(params));
    params.fourcc = MMAL_CONTROL_FOURCC;
    params.callback = vcmmal_callback;
    params.userdata = mmal;
    params.version = WORKER_VER_MAJOR;
    params.version_min = WORKER_VER_MINIMUM;

    if (VCHIQOpenService(mmal->instance, &params, &mmal->service) != VCHIQ_SUCCESS)
        goto fail_worker;

    return mmal;

fail_worker:
    Signal(mmal->worker, SIGF_QUIT);
    Wait(SIGF_SINGLE);
fail_instance:
    VCHIQShutdown(mmal->instance);
fail_lib:
    CloseLibrary(mmal->vchiqbase);
fail_free:
    FreeVec(mmal);
    return NULL;
}

void VCMMAL_Close(struct VCMMAL *mmal)
{
    if (!mmal)
        return;

    VCHIQCloseService(mmal->service);

    mmal->opener = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    Signal(mmal->worker, SIGF_QUIT);
    Wait(SIGF_SINGLE);

    VCHIQShutdown(mmal->instance);
    CloseLibrary(mmal->vchiqbase);
    FreeVec(mmal);
}

LONG VCMMAL_GetVersion(struct VCMMAL *mmal, ULONG *major, ULONG *minor)
{
    mmal_worker_version msg, reply;
    LONG status;

    memset(&msg, 0, sizeof(msg));
    status = sendwait(mmal, &msg.header, sizeof(msg), MMAL_WORKER_GET_VERSION,
                      &reply, sizeof(reply));
    if (status == MMAL_SUCCESS)
    {
        *major = reply.major;
        *minor = reply.minor;
    }
    return status;
}

/*
 * Components
 */
static LONG component_op(struct VCMMALComponent *comp, ULONG msgid)
{
    mmal_worker_component_op msg;
    mmal_worker_reply reply;
    LONG status;

    memset(&msg, 0, sizeof(msg));
    msg.component_handle = comp->handle;
    status = sendwait(comp->client, &msg.header, sizeof(msg), msgid,
                      &reply, sizeof(reply));
    return (status == MMAL_SUCCESS) ? (LONG)reply.status : status;
}

static void port_init(struct VCMMALComponent *comp, struct VCMMALPort *port,
                      ULONG type, ULONG index)
{
    port->component = comp;
    port->type = type;
    port->index = index;
}

LONG VCMMAL_ComponentCreate(struct VCMMAL *mmal, const char *name,
                            struct VCMMALComponent **pcomp)
{
    mmal_worker_component_create msg;
    mmal_worker_component_create_reply reply;
    struct VCMMALComponent *comp;
    LONG status;
    ULONG i;

    if (strlen(name) >= sizeof(msg.name))
        return MMAL_EINVAL;

    comp = AllocVec(sizeof(*comp), MEMF_ANY | MEMF_CLEAR);
    if (!comp)
        return MMAL_ENOMEM;
    comp->client = mmal;
    comp->cookie = cookie_add(mmal, comp);
    if (!comp->cookie)
    {
        FreeVec(comp);
        return MMAL_ENOSPC;
    }

    memset(&msg, 0, sizeof(msg));
    msg.client_component = comp->cookie;
    strcpy(msg.name, name);

    status = sendwait(mmal, &msg.header, sizeof(msg), MMAL_WORKER_COMPONENT_CREATE,
                      &reply, sizeof(reply));
    if (status == MMAL_SUCCESS)
        status = reply.status;
    if (status != MMAL_SUCCESS)
    {
        cookie_del(mmal, comp->cookie);
        FreeVec(comp);
        return status;
    }

    comp->handle = reply.component_handle;
    comp->input_num = (reply.input_num < VCMMAL_MAX_PORTS) ? reply.input_num : VCMMAL_MAX_PORTS;
    comp->output_num = (reply.output_num < VCMMAL_MAX_PORTS) ? reply.output_num : VCMMAL_MAX_PORTS;
    comp->clock_num = (reply.clock_num < VCMMAL_MAX_PORTS) ? reply.clock_num : VCMMAL_MAX_PORTS;

    D(bug("[VCMMAL] %s: handle 0x%x, %u in, %u out, %u clock\n", name,
          (unsigned)comp->handle, (unsigned)comp->input_num,
          (unsigned)comp->output_num, (unsigned)comp->clock_num));

    port_init(comp, &comp->control, MMAL_PORT_TYPE_CONTROL, 0);
    status = VCMMAL_PortInfoGet(&comp->control);
    for (i = 0; (status == MMAL_SUCCESS) && (i < comp->input_num); i++)
    {
        port_init(comp, &comp->input[i], MMAL_PORT_TYPE_INPUT, i);
        status = VCMMAL_PortInfoGet(&comp->input[i]);
    }
    for (i = 0; (status == MMAL_SUCCESS) && (i < comp->output_num); i++)
    {
        port_init(comp, &comp->output[i], MMAL_PORT_TYPE_OUTPUT, i);
        status = VCMMAL_PortInfoGet(&comp->output[i]);
    }
    for (i = 0; (status == MMAL_SUCCESS) && (i < comp->clock_num); i++)
    {
        port_init(comp, &comp->clock[i], MMAL_PORT_TYPE_CLOCK, i);
        status = VCMMAL_PortInfoGet(&comp->clock[i]);
    }

    if (status != MMAL_SUCCESS)
    {
        VCMMAL_ComponentDestroy(comp);
        return status;
    }

    *pcomp = comp;
    return MMAL_SUCCESS;
}

LONG VCMMAL_ComponentDestroy(struct VCMMALComponent *comp)
{
    LONG status;

    if (!comp)
        return MMAL_SUCCESS;

    status = component_op(comp, MMAL_WORKER_COMPONENT_DESTROY);
    cookie_del(comp->client, comp->cookie);
    FreeVec(comp);

    return status;
}

LONG VCMMAL_ComponentEnable(struct VCMMALComponent *comp)
{
    return component_op(comp, MMAL_WORKER_COMPONENT_ENABLE);
}

LONG VCMMAL_ComponentDisable(struct VCMMALComponent *comp)
{
    return component_op(comp, MMAL_WORKER_COMPONENT_DISABLE);
}

/*
 * Ports
 */
static void port_update(struct VCMMALPort *port, const mmal_worker_port_info *reply)
{
    port->handle = reply->port_handle;
    port->port = reply->port;
    port->format = reply->format;
    port->es = reply->es;
    memcpy(port->extradata, reply->extradata, sizeof(port->extradata));
}

LONG VCMMAL_PortInfoGet(struct VCMMALPort *port)
{
    mmal_worker_port_info_get msg;
    mmal_worker_port_info reply;
    LONG status;

    memset(&msg, 0, sizeof(msg));
    msg.component_handle = port->component->handle;
    msg.port_type = port->type;
    msg.index = port->index;

    status = sendwait(port->component->client, &msg.header, sizeof(msg),
                      MMAL_WORKER_PORT_INFO_GET, &reply, sizeof(reply));
    if (status == MMAL_SUCCESS)
        status = reply.status;
    if (status == MMAL_SUCCESS)
        port_update(port, &reply);

    return status;
}

LONG VCMMAL_PortFormatCommit(struct VCMMALPort *port)
{
    mmal_worker_port_info_set msg;
    mmal_worker_port_info reply;
    LONG status;

    memset(&msg, 0, sizeof(msg));
    msg.component_handle = port->component->handle;
    msg.port_type = port->type;
    msg.index = port->index;
    msg.port = port->port;
    msg.format = port->format;
    msg.es = port->es;
    if (msg.format.extradata_size > MMAL_FORMAT_EXTRADATA_MAX_SIZE)
        msg.format.extradata_size = MMAL_FORMAT_EXTRADATA_MAX_SIZE;
    memcpy(msg.extradata, port->extradata, msg.format.extradata_size);

    /* The VideoCore's own pointers; ours mean nothing there */
    msg.port.priv = msg.port.name = msg.port.format = 0;
    msg.port.component = msg.port.userdata = 0;
    msg.format.es = msg.format.extradata = 0;

    status = sendwait(port->component->client, &msg.header, sizeof(msg),
                      MMAL_WORKER_PORT_INFO_SET, &reply, sizeof(reply));
    if (status == MMAL_SUCCESS)
        status = reply.status;
    if (status == MMAL_SUCCESS)
        port_update(port, &reply);

    return status;
}

static LONG port_action(struct VCMMALPort *port, ULONG action)
{
    mmal_worker_port_action msg;
    mmal_worker_reply reply;
    LONG status;

    memset(&msg, 0, sizeof(msg));
    msg.component_handle = port->component->handle;
    msg.port_handle = port->handle;
    msg.action = action;
    if (action == MMAL_WORKER_PORT_ACTION_ENABLE)
    {
        msg.param.enable.port = port->port;
        msg.param.enable.port.priv = msg.param.enable.port.name = 0;
        msg.param.enable.port.format = msg.param.enable.port.component = 0;
        msg.param.enable.port.userdata = 0;
    }

    status = sendwait(port->component->client, &msg.header, sizeof(msg),
                      MMAL_WORKER_PORT_ACTION, &reply, sizeof(reply));
    return (status == MMAL_SUCCESS) ? (LONG)reply.status : status;
}

LONG VCMMAL_PortEnable(struct VCMMALPort *port, VCMMAL_BUFFER_CB cb)
{
    LONG status;

    port->buffer_cb = cb;
    status = port_action(port, MMAL_WORKER_PORT_ACTION_ENABLE);
    if (status == MMAL_SUCCESS)
        port->enabled = TRUE;

    return status;
}

LONG VCMMAL_PortDisable(struct VCMMALPort *port)
{
    LONG status = port_action(port, MMAL_WORKER_PORT_ACTION_DISABLE);

    port->enabled = FALSE;
    return status;
}

LONG VCMMAL_PortFlush(struct VCMMALPort *port)
{
    return port_action(port, MMAL_WORKER_PORT_ACTION_FLUSH);
}

LONG VCMMAL_PortParameterSet(struct VCMMALPort *port, ULONG id,
                             const void *value, ULONG size)
{
    mmal_worker_port_param msg;
    mmal_worker_reply reply;
    LONG status;

    if (size > sizeof(msg.space))
        return MMAL_EINVAL;

    memset(&msg, 0, sizeof(msg));
    msg.component_handle = port->component->handle;
    msg.port_handle = port->handle;
    msg.param.id = id;
    msg.param.size = sizeof(msg.param) + size;
    memcpy(msg.space, value, size);

    status = sendwait(port->component->client, &msg.header, sizeof(msg),
                      MMAL_WORKER_PORT_PARAMETER_SET, &reply, sizeof(reply));
    return (status == MMAL_SUCCESS) ? (LONG)reply.status : status;
}

LONG VCMMAL_PortParameterGet(struct VCMMALPort *port, ULONG id,
                             void *value, ULONG *size)
{
    mmal_worker_port_param msg;
    mmal_worker_port_param_get_reply reply;
    LONG status;
    ULONG got;

    if (*size > sizeof(msg.space))
        return MMAL_EINVAL;

    memset(&msg, 0, sizeof(msg));
    msg.component_handle = port->component->handle;
    msg.port_handle = port->handle;
    msg.param.id = id;
    msg.param.size = sizeof(msg.param) + *size;

    status = sendwait(port->component->client, &msg.header, sizeof(msg),
                      MMAL_WORKER_PORT_PARAMETER_GET, &reply, sizeof(reply));
    if (status == MMAL_SUCCESS)
        status = reply.status;
    if (status == MMAL_SUCCESS)
    {
        got = (reply.param.size > sizeof(reply.param))
            ? reply.param.size - sizeof(reply.param) : 0;
        if (got > *size)
            got = *size;
        memcpy(value, reply.space, got);
        *size = got;
    }

    return status;
}

/*
 * Buffers
 */
LONG VCMMAL_SendBuffer(struct VCMMALPort *port, struct VCMMALBuffer *buf)
{
    static UBYTE zeros[8];
    struct VCMMAL *mmal = port->component->client;
    mmal_worker_buffer_from_host msg;
    ULONG msgid = MMAL_WORKER_BUFFER_FROM_HOST;
    const UBYTE *bulk = NULL;
    ULONG length;
    LONG status;

    buf->port = port;
    buf->cookie = cookie_add(mmal, buf);
    if (!buf->cookie)
        return MMAL_ENOSPC;

    memset(&msg, 0, sizeof(msg));
    msg.header.magic = MMAL_MAGIC;
    msg.drvbuf.magic = MMAL_MAGIC;
    msg.drvbuf.component_handle = port->component->handle;
    msg.drvbuf.port_handle = port->handle;
    msg.drvbuf.client_context = buf->cookie;

    length = buf->length;
    msg.buffer_header.data = buf->cookie;
    msg.buffer_header.alloc_size = buf->alloc_size;
    msg.buffer_header.length = length;
    msg.buffer_header.offset = buf->offset;
    msg.buffer_header.flags = buf->flags;
    msg.buffer_header.pts = buf->pts;
    msg.buffer_header.dts = buf->dts;
    msg.buffer_header_type_specific = buf->video;

    if (length)
    {
        bulk = buf->data + buf->offset;
        /* The data arrives at the start of the VideoCore's buffer */
        msg.buffer_header.offset = 0;
    }
    else if (buf->flags & MMAL_BUFFER_HEADER_FLAG_EOS)
    {
        bulk = zeros;
        length = sizeof(zeros);
        msgid = MMAL_WORKER_BUFFER_FROM_HOST_ZEROLEN;
    }
    msg.header.msgid = msgid;

    ObtainSemaphore(&mmal->send_lock);
    status = VCHIQQueueMessage(mmal->service, &msg, sizeof(msg));
    if ((status == VCHIQ_SUCCESS) && bulk)
        status = VCHIQBulkTransmit(mmal->service, bulk, (length + 3) & ~3,
                                   NULL, VCHIQ_BULK_MODE_NOCALLBACK);
    ReleaseSemaphore(&mmal->send_lock);

    if (status != VCHIQ_SUCCESS)
    {
        cookie_del(mmal, buf->cookie);
        buf->cookie = 0;
        return MMAL_EIO;
    }

    return MMAL_SUCCESS;
}

BOOL VCMMAL_FormatChangedGet(const UBYTE *data, ULONG length,
                             struct VCMMALFormatChanged *fc)
{
    const MMAL_EVENT_FORMAT_CHANGED_WIRE_T *ev = (const void *)data;
    ULONG need = sizeof(*ev) + sizeof(fc->format) + sizeof(fc->es);

    if (length < need)
        return FALSE;

    fc->buffer_size_min = ev->buffer_size_min;
    fc->buffer_num_min = ev->buffer_num_min;
    fc->buffer_size_recommended = ev->buffer_size_recommended;
    fc->buffer_num_recommended = ev->buffer_num_recommended;
    memcpy(&fc->format, data + sizeof(*ev), sizeof(fc->format));
    memcpy(&fc->es, data + sizeof(*ev) + sizeof(fc->format), sizeof(fc->es));

    return TRUE;
}
