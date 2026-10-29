/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    libmmal core: queues, pools, buffer headers, formats, events, and
    ports and components backed by libvcmmal. Follows the semantics of
    raspberrypi/userland interface/mmal/core.

    Buffer and event callbacks run on the libvcmmal worker task.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <devices/timer.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

#include "libmmal_private.h"

#define EVENT_POOL_HEADERS      8
#define EVENT_PAYLOAD_SIZE      512
/* Room for the whole word the VideoCore rounds transfers up to */
#define PAYLOAD_SLACK           64
#define IDLE_TIMEOUT_MS         2000
#define WAIT_FOREVER            ((ULONG)~0)

#define PRIV_OF_VB(vb) \
    ((struct MMAL_BUFFER_HEADER_PRIVATE_T *) \
     ((char *)(vb) - offsetof(struct MMAL_BUFFER_HEADER_PRIVATE_T, vb)))

ULONG libmmal_wait(ULONG sigmask, ULONG ms)
{
    struct MsgPort *mp = CreateMsgPort();
    struct timerequest *tr = mp ? CreateIORequest(mp, sizeof(*tr)) : NULL;
    ULONG got;

    if (!tr || OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)tr, 0))
    {
        /* No timer: wait without one rather than spin */
        if (tr)
            DeleteIORequest((struct IORequest *)tr);
        if (mp)
            DeleteMsgPort(mp);
        return Wait(sigmask) & sigmask;
    }

    tr->tr_node.io_Command = TR_ADDREQUEST;
    tr->tr_time.tv_secs = ms / 1000;
    tr->tr_time.tv_micro = (ms % 1000) * 1000;
    SendIO((struct IORequest *)tr);

    got = Wait(sigmask | (1UL << mp->mp_SigBit));

    if (!CheckIO((struct IORequest *)tr))
        AbortIO((struct IORequest *)tr);
    WaitIO((struct IORequest *)tr);
    CloseDevice((struct IORequest *)tr);
    DeleteIORequest((struct IORequest *)tr);
    DeleteMsgPort(mp);

    return got & sigmask;
}

/*
 * Queues
 */
MMAL_QUEUE_T *mmal_queue_create(void)
{
    MMAL_QUEUE_T *q = AllocVec(sizeof(*q), MEMF_ANY | MEMF_CLEAR);

    if (q)
    {
        InitSemaphore(&q->lock);
        q->last = &q->first;
    }
    return q;
}

void mmal_queue_destroy(MMAL_QUEUE_T *queue)
{
    FreeVec(queue);
}

void mmal_queue_put(MMAL_QUEUE_T *queue, MMAL_BUFFER_HEADER_T *buffer)
{
    ObtainSemaphore(&queue->lock);
    buffer->next = NULL;
    *queue->last = buffer;
    queue->last = &buffer->next;
    queue->length++;
    if (queue->waiter)
        Signal(queue->waiter, queue->waitmask);
    ReleaseSemaphore(&queue->lock);
}

void mmal_queue_put_back(MMAL_QUEUE_T *queue, MMAL_BUFFER_HEADER_T *buffer)
{
    ObtainSemaphore(&queue->lock);
    buffer->next = queue->first;
    queue->first = buffer;
    if (queue->last == &queue->first)
        queue->last = &buffer->next;
    queue->length++;
    if (queue->waiter)
        Signal(queue->waiter, queue->waitmask);
    ReleaseSemaphore(&queue->lock);
}

MMAL_BUFFER_HEADER_T *mmal_queue_get(MMAL_QUEUE_T *queue)
{
    MMAL_BUFFER_HEADER_T *b;

    ObtainSemaphore(&queue->lock);
    b = queue->first;
    if (b)
    {
        queue->first = b->next;
        if (!queue->first)
            queue->last = &queue->first;
        queue->length--;
        b->next = NULL;
    }
    ReleaseSemaphore(&queue->lock);

    return b;
}

unsigned int mmal_queue_length(MMAL_QUEUE_T *queue)
{
    return queue->length;
}

static MMAL_BUFFER_HEADER_T *queue_wait(MMAL_QUEUE_T *queue, ULONG ms)
{
    MMAL_BUFFER_HEADER_T *b;
    BYTE sig;

    if ((b = mmal_queue_get(queue)) || !ms)
        return b;

    sig = AllocSignal(-1);
    if (sig == -1)
        return NULL;

    ObtainSemaphore(&queue->lock);
    queue->waiter = FindTask(NULL);
    queue->waitmask = 1UL << sig;
    ReleaseSemaphore(&queue->lock);

    /* WAIT_FOREVER: no timeout */
    while (!(b = mmal_queue_get(queue)))
    {
        if (ms == WAIT_FOREVER)
            Wait(1UL << sig);
        else if (!libmmal_wait(1UL << sig, ms))
            break;
    }

    ObtainSemaphore(&queue->lock);
    queue->waiter = NULL;
    ReleaseSemaphore(&queue->lock);
    FreeSignal(sig);

    if (!b)
        b = mmal_queue_get(queue);
    return b;
}

MMAL_BUFFER_HEADER_T *mmal_queue_wait(MMAL_QUEUE_T *queue)
{
    return queue_wait(queue, WAIT_FOREVER);
}

MMAL_BUFFER_HEADER_T *mmal_queue_timedwait(MMAL_QUEUE_T *queue, VCOS_UNSIGNED timeout)
{
    return queue_wait(queue, timeout);
}

/*
 * Buffer headers
 */
void mmal_buffer_header_acquire(MMAL_BUFFER_HEADER_T *header)
{
    __atomic_add_fetch(&header->priv->refcount, 1, __ATOMIC_ACQ_REL);
}

void mmal_buffer_header_reset(MMAL_BUFFER_HEADER_T *header)
{
    header->length = 0;
    header->offset = 0;
    header->flags = 0;
    header->pts = MMAL_TIME_UNKNOWN;
    header->dts = MMAL_TIME_UNKNOWN;
}

void mmal_buffer_header_release_continue(MMAL_BUFFER_HEADER_T *header)
{
    mmal_buffer_header_reset(header);
    if (header->priv->pf_release)
        header->priv->pf_release(header);
}

void mmal_buffer_header_release(MMAL_BUFFER_HEADER_T *header)
{
    if (__atomic_sub_fetch(&header->priv->refcount, 1, __ATOMIC_ACQ_REL) != 0)
        return;

    if (header->priv->pf_pre_release &&
        header->priv->pf_pre_release(header, header->priv->pre_release_userdata))
        return;     /* the client releases it later */

    mmal_buffer_header_release_continue(header);
}

void mmal_buffer_header_pre_release_cb_set(MMAL_BUFFER_HEADER_T *header,
                                           MMAL_BH_PRE_RELEASE_CB_T cb, void *userdata)
{
    header->priv->pf_pre_release = cb;
    header->priv->pre_release_userdata = userdata;
}

MMAL_STATUS_T mmal_buffer_header_replicate(MMAL_BUFFER_HEADER_T *dest,
                                           MMAL_BUFFER_HEADER_T *src)
{
    return MMAL_ENOSYS;
}

MMAL_STATUS_T mmal_buffer_header_mem_lock(MMAL_BUFFER_HEADER_T *header)
{
    return MMAL_SUCCESS;
}

void mmal_buffer_header_mem_unlock(MMAL_BUFFER_HEADER_T *header)
{
}

/*
 * Pools
 */
struct POOL_PRIVATE {
    MMAL_POOL_T pool;
    MMAL_POOL_BH_CB_T cb;
    void *userdata;
    uint32_t payload_size;
    mmal_pool_allocator_free_t pf_free;
    void *alloc_context;
};

static void pool_release(MMAL_BUFFER_HEADER_T *header)
{
    struct POOL_PRIVATE *pp = header->priv->owner;
    BOOL requeue = TRUE;

    header->priv->refcount = 1;
    header->cmd = 0;
    if (pp->cb)
        requeue = pp->cb(&pp->pool, header, pp->userdata);
    if (requeue)
        mmal_queue_put(pp->pool.queue, header);
}

static void *pool_default_alloc(void *context, uint32_t size)
{
    return AllocVec(size + PAYLOAD_SLACK, MEMF_ANY);
}

static void pool_default_free(void *context, void *mem)
{
    FreeVec(mem);
}

MMAL_POOL_T *mmal_pool_create_with_allocator(unsigned int headers, uint32_t payload_size,
    void *allocator_context, mmal_pool_allocator_alloc_t allocator_alloc,
    mmal_pool_allocator_free_t allocator_free)
{
    struct POOL_PRIVATE *pp;
    unsigned int i;

    pp = AllocVec(sizeof(*pp) + headers * sizeof(MMAL_BUFFER_HEADER_T *),
                  MEMF_ANY | MEMF_CLEAR);
    if (!pp)
        return NULL;
    pp->pool.header = (MMAL_BUFFER_HEADER_T **)&pp[1];
    pp->payload_size = payload_size;
    pp->pf_free = allocator_free;
    pp->alloc_context = allocator_context;

    pp->pool.queue = mmal_queue_create();
    if (!pp->pool.queue)
    {
        FreeVec(pp);
        return NULL;
    }

    for (i = 0; i < headers; i++)
    {
        MMAL_BUFFER_HEADER_T *h;
        struct MMAL_BUFFER_HEADER_PRIVATE_T *priv;

        h = AllocVec(sizeof(*h) + sizeof(*priv), MEMF_ANY | MEMF_CLEAR);
        if (!h)
            break;
        priv = (struct MMAL_BUFFER_HEADER_PRIVATE_T *)&h[1];
        h->priv = priv;
        h->type = &priv->type;
        priv->header = h;
        priv->refcount = 1;
        priv->pf_release = pool_release;
        priv->owner = pp;

        if (payload_size)
        {
            h->data = allocator_alloc(allocator_context, payload_size);
            if (!h->data)
            {
                FreeVec(h);
                break;
            }
            h->alloc_size = payload_size;
        }
        mmal_buffer_header_reset(h);

        pp->pool.header[i] = h;
        pp->pool.headers_num = i + 1;
        mmal_queue_put(pp->pool.queue, h);
    }

    if (pp->pool.headers_num != headers)
    {
        mmal_pool_destroy(&pp->pool);
        return NULL;
    }

    return &pp->pool;
}

MMAL_POOL_T *mmal_pool_create(unsigned int headers, uint32_t payload_size)
{
    return mmal_pool_create_with_allocator(headers, payload_size, NULL,
                                           pool_default_alloc, pool_default_free);
}

void mmal_pool_destroy(MMAL_POOL_T *pool)
{
    struct POOL_PRIVATE *pp = (struct POOL_PRIVATE *)pool;
    unsigned int i;

    if (!pool)
        return;

    for (i = 0; i < pool->headers_num; i++)
    {
        MMAL_BUFFER_HEADER_T *h = pool->header[i];

        if (pp->payload_size && h->priv->owner == pp)
            pp->pf_free(pp->alloc_context, h->data);
        FreeVec(h);
    }
    mmal_queue_destroy(pool->queue);
    FreeVec(pp);
}

MMAL_STATUS_T mmal_pool_resize(MMAL_POOL_T *pool, unsigned int headers, uint32_t payload_size)
{
    return MMAL_ENOSYS;
}

void mmal_pool_callback_set(MMAL_POOL_T *pool, MMAL_POOL_BH_CB_T cb, void *userdata)
{
    struct POOL_PRIVATE *pp = (struct POOL_PRIVATE *)pool;

    pp->cb = cb;
    pp->userdata = userdata;
}

void mmal_pool_pre_release_callback_set(MMAL_POOL_T *pool, MMAL_BH_PRE_RELEASE_CB_T cb,
                                        void *userdata)
{
    unsigned int i;

    for (i = 0; i < pool->headers_num; i++)
        mmal_buffer_header_pre_release_cb_set(pool->header[i], cb, userdata);
}

/*
 * Formats and events
 */
MMAL_ES_FORMAT_T *mmal_format_alloc(void)
{
    MMAL_ES_FORMAT_T *f = AllocVec(sizeof(*f) + sizeof(MMAL_ES_SPECIFIC_FORMAT_T),
                                   MEMF_ANY | MEMF_CLEAR);

    if (f)
        f->es = (MMAL_ES_SPECIFIC_FORMAT_T *)&f[1];
    return f;
}

void mmal_format_free(MMAL_ES_FORMAT_T *format)
{
    FreeVec(format);
}

MMAL_STATUS_T mmal_format_extradata_alloc(MMAL_ES_FORMAT_T *format, unsigned int size)
{
    /* Port formats carry a fixed buffer of the size the VideoCore takes */
    if (format->extradata && size <= LIBMMAL_EXTRADATA_MAX)
        return MMAL_SUCCESS;
    return MMAL_ENOMEM;
}

void mmal_format_copy(MMAL_ES_FORMAT_T *fmt_dst, MMAL_ES_FORMAT_T *fmt_src)
{
    void *backup = fmt_dst->es;

    *fmt_dst->es = *fmt_src->es;
    *fmt_dst = *fmt_src;
    fmt_dst->es = backup;
    fmt_dst->extradata = 0;
    fmt_dst->extradata_size = 0;
}

MMAL_STATUS_T mmal_format_full_copy(MMAL_ES_FORMAT_T *fmt_dst, MMAL_ES_FORMAT_T *fmt_src)
{
    void *extradata = fmt_dst->extradata;

    mmal_format_copy(fmt_dst, fmt_src);
    if (fmt_src->extradata_size)
    {
        if (!extradata || fmt_src->extradata_size > LIBMMAL_EXTRADATA_MAX)
            return MMAL_ENOMEM;
        memcpy(extradata, fmt_src->extradata, fmt_src->extradata_size);
    }
    fmt_dst->extradata = extradata;
    fmt_dst->extradata_size = fmt_src->extradata_size;
    return MMAL_SUCCESS;
}

MMAL_EVENT_FORMAT_CHANGED_T *mmal_event_format_changed_get(MMAL_BUFFER_HEADER_T *buffer)
{
    MMAL_EVENT_FORMAT_CHANGED_T *event;
    MMAL_ES_FORMAT_T *format;
    uint32_t size;

    size = sizeof(MMAL_EVENT_FORMAT_CHANGED_T);
    size += sizeof(MMAL_ES_FORMAT_T) + sizeof(MMAL_ES_SPECIFIC_FORMAT_T);

    if (!buffer || buffer->cmd != MMAL_EVENT_FORMAT_CHANGED || buffer->length < size)
        return 0;

    event = (MMAL_EVENT_FORMAT_CHANGED_T *)buffer->data;
    format = event->format = (MMAL_ES_FORMAT_T *)&event[1];
    format->es = (MMAL_ES_SPECIFIC_FORMAT_T *)&format[1];
    format->extradata = (uint8_t *)&format->es[1];
    format->extradata_size = buffer->length - size;
    return event;
}

/*
 * Ports
 */
static void port_from_vc(MMAL_PORT_T *port)
{
    struct VCMMALPort *vp = port->priv->vp;
    MMAL_ES_FORMAT_T *f = port->format;

    port->buffer_num_min = vp->port.buffer_num_min;
    port->buffer_size_min = vp->port.buffer_size_min;
    port->buffer_alignment_min = vp->port.buffer_alignment_min;
    port->buffer_num_recommended = vp->port.buffer_num_recommended;
    port->buffer_size_recommended = vp->port.buffer_size_recommended;
    port->buffer_num = vp->port.buffer_num;
    port->buffer_size = vp->port.buffer_size;
    port->capabilities = vp->port.capabilities;

    f->type = vp->format.type;
    f->encoding = vp->format.encoding;
    f->encoding_variant = vp->format.encoding_variant;
    f->bitrate = vp->format.bitrate;
    f->flags = vp->format.flags;
    *f->es = vp->es;
    f->extradata_size = (vp->format.extradata_size <= LIBMMAL_EXTRADATA_MAX)
        ? vp->format.extradata_size : LIBMMAL_EXTRADATA_MAX;
    memcpy(f->extradata, vp->extradata, f->extradata_size);
}

static void port_to_vc(MMAL_PORT_T *port)
{
    struct VCMMALPort *vp = port->priv->vp;
    MMAL_ES_FORMAT_T *f = port->format;
    uint32_t n = (f->extradata_size <= LIBMMAL_EXTRADATA_MAX)
        ? f->extradata_size : LIBMMAL_EXTRADATA_MAX;

    vp->port.buffer_num = port->buffer_num;
    vp->port.buffer_size = port->buffer_size;

    vp->format.type = f->type;
    vp->format.encoding = f->encoding;
    vp->format.encoding_variant = f->encoding_variant;
    vp->format.bitrate = f->bitrate;
    vp->format.flags = f->flags;
    vp->es = *f->es;
    vp->format.extradata_size = n;
    if (n && f->extradata && f->extradata != vp->extradata)
        memcpy(vp->extradata, f->extradata, n);
}

static void port_idle_signal(MMAL_PORT_T *port)
{
    struct MMAL_PORT_PRIVATE_T *pp = port->priv;

    if (__atomic_sub_fetch(&pp->inflight, 1, __ATOMIC_ACQ_REL) == 0)
    {
        Forbid();
        if (pp->idle_waiter)
            Signal(pp->idle_waiter, pp->idle_mask);
        Permit();
    }
}

/* Until the VideoCore has handed back every buffer of the port */
static void port_wait_idle(MMAL_PORT_T *port)
{
    struct MMAL_PORT_PRIVATE_T *pp = port->priv;
    BYTE sig;

    if (!__atomic_load_n(&pp->inflight, __ATOMIC_ACQUIRE))
        return;

    sig = AllocSignal(-1);
    if (sig == -1)
        return;

    Forbid();
    pp->idle_waiter = FindTask(NULL);
    pp->idle_mask = 1UL << sig;
    Permit();

    while (__atomic_load_n(&pp->inflight, __ATOMIC_ACQUIRE))
    {
        if (!libmmal_wait(1UL << sig, IDLE_TIMEOUT_MS))
        {
            bug("[libmmal] %s: %d buffers not returned\n", port->name,
                (int)pp->inflight);
            break;
        }
    }

    Forbid();
    pp->idle_waiter = NULL;
    Permit();
    FreeSignal(sig);
}

/* libvcmmal: a buffer is back from the VideoCore */
static void vc_buffer_cb(struct VCMMALPort *vp, struct VCMMALBuffer *vb)
{
    MMAL_PORT_T *port = vp->user_data;
    struct MMAL_BUFFER_HEADER_PRIVATE_T *priv = PRIV_OF_VB(vb);
    MMAL_BUFFER_HEADER_T *h = priv->header;
    MMAL_PORT_BH_CB_T cb = port->priv->cb;

    h->cmd = 0;
    h->length = vb->length;
    h->offset = vb->offset;
    h->flags = vb->flags;
    h->pts = vb->pts;
    h->dts = vb->dts;
    h->type->video = vb->video;

    /* Hand it over before a waiting disable may return */
    if (cb)
        cb(port, h);
    else
        mmal_buffer_header_release(h);

    port_idle_signal(port);
}

/* libvcmmal: an event; delivered like MMAL does, as a buffer with cmd set */
static void vc_event_cb(struct VCMMALPort *vp, ULONG cmd, const UBYTE *data, ULONG length)
{
    MMAL_PORT_T *port = vp->user_data;
    MMAL_COMPONENT_T *comp = port->component;
    MMAL_BUFFER_HEADER_T *h = mmal_queue_get(comp->priv->event_pool->queue);
    MMAL_PORT_BH_CB_T cb = port->priv->cb;

    if (!h)
    {
        bug("[libmmal] %s: no event buffer for 0x%08x\n", port->name, (unsigned)cmd);
        return;
    }

    h->cmd = cmd;
    if (cmd == MMAL_EVENT_FORMAT_CHANGED)
    {
        struct VCMMALFormatChanged fc;
        MMAL_EVENT_FORMAT_CHANGED_T *ev = (MMAL_EVENT_FORMAT_CHANGED_T *)h->data;
        MMAL_ES_FORMAT_T *f = (MMAL_ES_FORMAT_T *)&ev[1];

        if (!VCMMAL_FormatChangedGet(data, length, &fc))
        {
            mmal_buffer_header_release(h);
            return;
        }
        memset(ev, 0, sizeof(*ev) + sizeof(*f) + sizeof(MMAL_ES_SPECIFIC_FORMAT_T));
        ev->buffer_size_min = fc.buffer_size_min;
        ev->buffer_num_min = fc.buffer_num_min;
        ev->buffer_size_recommended = fc.buffer_size_recommended;
        ev->buffer_num_recommended = fc.buffer_num_recommended;
        f->type = fc.format.type;
        f->encoding = fc.format.encoding;
        f->encoding_variant = fc.format.encoding_variant;
        f->bitrate = fc.format.bitrate;
        f->flags = fc.format.flags;
        *(MMAL_ES_SPECIFIC_FORMAT_T *)&f[1] = fc.es;
        h->length = sizeof(*ev) + sizeof(*f) + sizeof(MMAL_ES_SPECIFIC_FORMAT_T);
        mmal_event_format_changed_get(h);
        bug("[libmmal] %s: format changed to %ux%u, %u buffers of %u (min %u of %u)\n",
            port->name, (unsigned)fc.es.video.width, (unsigned)fc.es.video.height,
            (unsigned)fc.buffer_num_recommended, (unsigned)fc.buffer_size_recommended,
            (unsigned)fc.buffer_num_min, (unsigned)fc.buffer_size_min);
    }
    else
    {
        h->length = (length < h->alloc_size) ? length : h->alloc_size;
        memcpy(h->data, data, h->length);
    }

    if (cb && port->is_enabled)
        cb(port, h);
    else
        mmal_buffer_header_release(h);
}

/* What the VideoCore refuses is worth seeing whatever the caller logs */
static MMAL_STATUS_T port_status(MMAL_PORT_T *port, const char *what, MMAL_STATUS_T st)
{
    if (st != MMAL_SUCCESS)
        bug("[libmmal] %s: %s failed (%d), %u buffers of %u (min %u of %u)\n",
            port->name, what, (int)st, (unsigned)port->buffer_num,
            (unsigned)port->buffer_size, (unsigned)port->buffer_num_min,
            (unsigned)port->buffer_size_min);
    return st;
}

/*
 * As userland does: the reply to a commit still has the old buffer
 * requirements, so read the port back, and an input's format changes the
 * outputs too.
 */
MMAL_STATUS_T mmal_port_format_commit(MMAL_PORT_T *port)
{
    MMAL_COMPONENT_T *comp = port->component;
    MMAL_STATUS_T st;
    ULONG i;

    port_to_vc(port);
    st = VCMMAL_PortFormatCommit(port->priv->vp);
    if (st == MMAL_SUCCESS)
        st = VCMMAL_PortInfoGet(port->priv->vp);
    if (st != MMAL_SUCCESS)
        return port_status(port, "format commit", st);
    port_from_vc(port);

    if (port->type == MMAL_PORT_TYPE_INPUT)
    {
        for (i = 0; i < comp->output_num; i++)
        {
            MMAL_PORT_T *out = comp->output[i];

            st = VCMMAL_PortInfoGet(out->priv->vp);
            if (st != MMAL_SUCCESS)
                return port_status(out, "info get", st);
            port_from_vc(out);
        }
    }
    return MMAL_SUCCESS;
}

MMAL_STATUS_T mmal_port_enable(MMAL_PORT_T *port, MMAL_PORT_BH_CB_T cb)
{
    struct VCMMALPort *vp = port->priv->vp;
    MMAL_STATUS_T st = MMAL_SUCCESS;

    if (port->is_enabled)
        return MMAL_EINVAL;

    port->priv->cb = cb;

    /* The control port stays enabled in the VideoCore; on and off is ours */
    if (port->type != MMAL_PORT_TYPE_CONTROL || !port->priv->vc_enabled)
    {
        vp->port.buffer_num = port->buffer_num;
        vp->port.buffer_size = port->buffer_size;
        st = port_status(port, "enable", VCMMAL_PortEnable(vp, vc_buffer_cb));
    }
    if (st == MMAL_SUCCESS)
    {
        port->is_enabled = 1;
        port->priv->vc_enabled = TRUE;
    }
    else
        port->priv->cb = NULL;
    return st;
}

MMAL_STATUS_T mmal_port_disable(MMAL_PORT_T *port)
{
    MMAL_STATUS_T st = MMAL_SUCCESS;

    if (!port->is_enabled)
        return MMAL_EINVAL;

    if (port->type != MMAL_PORT_TYPE_CONTROL)
    {
        st = port_status(port, "disable", VCMMAL_PortDisable(port->priv->vp));
        port_wait_idle(port);
        port->priv->vc_enabled = FALSE;
    }
    port->is_enabled = 0;
    port->priv->cb = NULL;
    return st;
}

MMAL_STATUS_T mmal_port_flush(MMAL_PORT_T *port)
{
    MMAL_STATUS_T st;

    if (!port->is_enabled)
        return MMAL_SUCCESS;

    if (port->type == MMAL_PORT_TYPE_CONTROL)
        return MMAL_SUCCESS;
    st = port_status(port, "flush", VCMMAL_PortFlush(port->priv->vp));
    port_wait_idle(port);
    return st;
}

MMAL_STATUS_T mmal_port_parameter_set(MMAL_PORT_T *port, const MMAL_PARAMETER_HEADER_T *param)
{
    if (param->size < sizeof(*param))
        return MMAL_EINVAL;
    return VCMMAL_PortParameterSet(port->priv->vp, param->id, &param[1],
                                   param->size - sizeof(*param));
}

MMAL_STATUS_T mmal_port_parameter_get(MMAL_PORT_T *port, MMAL_PARAMETER_HEADER_T *param)
{
    ULONG size;
    MMAL_STATUS_T st;

    if (param->size < sizeof(*param))
        return MMAL_EINVAL;
    size = param->size - sizeof(*param);
    st = VCMMAL_PortParameterGet(port->priv->vp, param->id, &param[1], &size);
    if (st == MMAL_SUCCESS)
        param->size = sizeof(*param) + size;
    return st;
}

MMAL_STATUS_T mmal_port_send_buffer(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
    struct VCMMALBuffer *vb = &buffer->priv->vb;
    MMAL_STATUS_T st;

    if (!port->is_enabled)
        return MMAL_EINVAL;

    memset(vb, 0, sizeof(*vb));
    vb->data = buffer->data;
    vb->alloc_size = buffer->alloc_size;
    vb->length = buffer->length;
    vb->offset = buffer->offset;
    vb->flags = buffer->flags;
    vb->pts = buffer->pts;
    vb->dts = buffer->dts;
    vb->video = buffer->type->video;

    __atomic_add_fetch(&port->priv->inflight, 1, __ATOMIC_ACQ_REL);
    st = VCMMAL_SendBuffer(port->priv->vp, vb);
    if (st != MMAL_SUCCESS)
        __atomic_sub_fetch(&port->priv->inflight, 1, __ATOMIC_ACQ_REL);
    return port_status(port, "send buffer", st);
}

MMAL_STATUS_T mmal_port_connect(MMAL_PORT_T *port, MMAL_PORT_T *other_port)
{
    return MMAL_ENOSYS;
}

MMAL_STATUS_T mmal_port_disconnect(MMAL_PORT_T *port)
{
    return MMAL_ENOSYS;
}

MMAL_STATUS_T mmal_port_event_get(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T **buffer,
                                  uint32_t event)
{
    MMAL_BUFFER_HEADER_T *h = mmal_queue_get(port->component->priv->event_pool->queue);

    if (!h)
        return MMAL_ENOSPC;
    h->cmd = event;
    *buffer = h;
    return MMAL_SUCCESS;
}

/*
 * Components
 */
static const char *port_type_name(MMAL_PORT_TYPE_T type)
{
    switch (type)
    {
    case MMAL_PORT_TYPE_CONTROL:    return "ctr";
    case MMAL_PORT_TYPE_INPUT:      return "in";
    case MMAL_PORT_TYPE_OUTPUT:     return "out";
    case MMAL_PORT_TYPE_CLOCK:      return "clk";
    default:                        return "?";
    }
}

static MMAL_PORT_T *port_new(MMAL_COMPONENT_T *comp, struct VCMMALPort *vp,
                             MMAL_PORT_TYPE_T type, unsigned int index,
                             unsigned int index_all)
{
    MMAL_PORT_T *port;
    struct MMAL_PORT_PRIVATE_T *pp;

    port = AllocVec(sizeof(*port) + sizeof(*pp), MEMF_ANY | MEMF_CLEAR);
    if (!port)
        return NULL;
    pp = (struct MMAL_PORT_PRIVATE_T *)&port[1];

    port->priv = pp;
    pp->vp = vp;
    vp->user_data = port;

    snprintf(pp->name, sizeof(pp->name), "%s:%s:%u", comp->name,
             port_type_name(type), index);
    port->name = pp->name;
    port->type = type;
    port->index = index;
    port->index_all = index_all;
    port->component = comp;

    pp->format.es = &pp->es;
    pp->format.extradata = pp->extradata;
    port->format = &pp->format;

    port_from_vc(port);
    return port;
}

MMAL_STATUS_T mmal_component_create(const char *name, MMAL_COMPONENT_T **component)
{
    static ULONG next_id;
    struct VCMMALComponent *vc;
    MMAL_COMPONENT_T *comp;
    struct MMAL_COMPONENT_PRIVATE_T *cp;
    MMAL_STATUS_T st;
    ULONG nports, i, n;
    MMAL_PORT_T **ports;

    if (strncmp(name, "vc.", 3) != 0)
        return MMAL_ENOSYS;

    if (mmal_vc_init() != MMAL_SUCCESS)
        return MMAL_ENXIO;

    st = VCMMAL_ComponentCreate(libmmal_vc_client(), name + 3, &vc);
    if (st != MMAL_SUCCESS)
    {
        mmal_vc_deinit();
        return st;
    }

    nports = 1 + vc->input_num + vc->output_num + vc->clock_num;
    comp = AllocVec(sizeof(*comp) + sizeof(*cp) + nports * sizeof(MMAL_PORT_T *)
                    + strlen(name) + 1, MEMF_ANY | MEMF_CLEAR);
    if (!comp)
    {
        VCMMAL_ComponentDestroy(vc);
        mmal_vc_deinit();
        return MMAL_ENOMEM;
    }
    cp = (struct MMAL_COMPONENT_PRIVATE_T *)&comp[1];
    ports = (MMAL_PORT_T **)&cp[1];
    comp->priv = cp;
    comp->name = strcpy((char *)&ports[nports], name);
    comp->id = ++next_id;
    comp->port = ports;
    comp->port_num = nports;
    cp->vc = vc;
    vc->user_data = comp;
    vc->event_cb = vc_event_cb;

    n = 0;
    st = MMAL_ENOMEM;
    if (!(ports[n++] = comp->control = port_new(comp, &vc->control, MMAL_PORT_TYPE_CONTROL, 0, 0)))
        goto fail;

    comp->input = &ports[n];
    comp->input_num = vc->input_num;
    for (i = 0; i < vc->input_num; i++, n++)
        if (!(ports[n] = port_new(comp, &vc->input[i], MMAL_PORT_TYPE_INPUT, i, n)))
            goto fail;

    comp->output = &ports[n];
    comp->output_num = vc->output_num;
    for (i = 0; i < vc->output_num; i++, n++)
        if (!(ports[n] = port_new(comp, &vc->output[i], MMAL_PORT_TYPE_OUTPUT, i, n)))
            goto fail;

    comp->clock = &ports[n];
    comp->clock_num = vc->clock_num;
    for (i = 0; i < vc->clock_num; i++, n++)
        if (!(ports[n] = port_new(comp, &vc->clock[i], MMAL_PORT_TYPE_CLOCK, i, n)))
            goto fail;

    cp->event_pool = mmal_pool_create(EVENT_POOL_HEADERS, EVENT_PAYLOAD_SIZE);
    if (!cp->event_pool)
        goto fail;

    D(bug("[libmmal] %s: %u in, %u out\n", name, (unsigned)comp->input_num,
          (unsigned)comp->output_num));

    *component = comp;
    return MMAL_SUCCESS;

fail:
    mmal_component_destroy(comp);
    return st;
}

MMAL_STATUS_T mmal_component_destroy(MMAL_COMPONENT_T *component)
{
    struct MMAL_COMPONENT_PRIVATE_T *cp;
    ULONG i;

    if (!component)
        return MMAL_EINVAL;
    cp = component->priv;

    for (i = 0; i < component->port_num; i++)
    {
        if (component->port[i] && component->port[i]->is_enabled)
            mmal_port_disable(component->port[i]);
    }
    if (component->is_enabled)
        VCMMAL_ComponentDisable(cp->vc);
    VCMMAL_ComponentDestroy(cp->vc);

    for (i = 0; i < component->port_num; i++)
        FreeVec(component->port[i]);
    mmal_pool_destroy(cp->event_pool);
    FreeVec(component);

    mmal_vc_deinit();
    return MMAL_SUCCESS;
}

MMAL_STATUS_T mmal_component_release(MMAL_COMPONENT_T *component)
{
    return mmal_component_destroy(component);
}

void mmal_component_acquire(MMAL_COMPONENT_T *component)
{
}

MMAL_STATUS_T mmal_component_enable(MMAL_COMPONENT_T *component)
{
    MMAL_STATUS_T st = VCMMAL_ComponentEnable(component->priv->vc);

    if (st == MMAL_SUCCESS)
        component->is_enabled = 1;
    return st;
}

MMAL_STATUS_T mmal_component_disable(MMAL_COMPONENT_T *component)
{
    MMAL_STATUS_T st = VCMMAL_ComponentDisable(component->priv->vc);

    if (st == MMAL_SUCCESS)
        component->is_enabled = 0;
    return st;
}
