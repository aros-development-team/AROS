/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Test for the vcmmal client: decode a JPEG on the VideoCore
    (vc.ril.image_decode) to I420. This exercises component and port
    setup, the format change event and bulk transfers both ways.

    mmaltest [in.jpg [out.yuv]]

    Without arguments it decodes a built-in 320x240 picture of red,
    green, blue and white quadrants and checks the colours.
    Exit code 0 on success, 20 on a failure.
*/

#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <hardware/vcmmal.h>

#include <stdio.h>
#include <string.h>

#include "quad_jpg.h"

#define SIGF_CB         SIGBREAKF_CTRL_F
#define TIMEOUT_TICKS   (10 * TICKS_PER_SECOND)

struct TBuf {
    struct MinNode node;
    struct TBuf *next_all;
    struct VCMMALBuffer b;
    int generation;     /* -1 for input buffers */
};

static struct Task *main_task;
static struct SignalSemaphore lock;
static struct MinList returned;
static struct VCMMALFormatChanged fc;
static volatile BOOL format_changed;
static volatile ULONG error_event;
static struct TBuf *all_bufs;

/* Both run on the vcmmal worker task */
static void buffer_cb(struct VCMMALPort *port, struct VCMMALBuffer *buf)
{
    struct TBuf *tb = buf->user_data;

    ObtainSemaphore(&lock);
    AddTail((struct List *)&returned, (struct Node *)&tb->node);
    ReleaseSemaphore(&lock);
    Signal(main_task, SIGF_CB);
}

static void event_cb(struct VCMMALPort *port, ULONG cmd, const UBYTE *data,
                     ULONG length)
{
    if (cmd == MMAL_EVENT_FORMAT_CHANGED)
    {
        if (VCMMAL_FormatChangedGet(data, length, &fc))
            format_changed = TRUE;
    }
    else if (cmd == MMAL_EVENT_ERROR)
        error_event = (length >= 4) ? *(const ULONG *)data : (ULONG)-1;
    Signal(main_task, SIGF_CB);
}

static struct TBuf *tbuf_alloc(ULONG size, int generation)
{
    struct TBuf *tb = AllocVec(sizeof(*tb), MEMF_ANY | MEMF_CLEAR);

    if (!tb)
        return NULL;
    /* Room for the word the VideoCore rounds transfers up to */
    tb->b.data = AllocVec(size + 4, MEMF_ANY);
    if (!tb->b.data)
    {
        FreeVec(tb);
        return NULL;
    }
    tb->b.alloc_size = size;
    tb->b.user_data = tb;
    tb->generation = generation;
    tb->next_all = all_bufs;
    all_bufs = tb;
    return tb;
}

static ULONG max3(ULONG a, ULONG b, ULONG c)
{
    ULONG m = (a > b) ? a : b;
    return (m > c) ? m : c;
}

static void print_port(const char *what, struct VCMMALPort *p)
{
    printf("%s: handle %u, enc %.4s, %ux%u crop %d,%d %dx%d, bufs %u x %u (min %u x %u)\n",
           what, (unsigned)p->handle, (const char *)&p->format.encoding,
           (unsigned)p->es.video.width, (unsigned)p->es.video.height,
           (int)p->es.video.crop.x, (int)p->es.video.crop.y,
           (int)p->es.video.crop.width, (int)p->es.video.crop.height,
           (unsigned)p->port.buffer_num, (unsigned)p->port.buffer_size,
           (unsigned)p->port.buffer_num_min, (unsigned)p->port.buffer_size_min);
}

static BOOL check_pixel(const UBYTE *frame, ULONG pitch, ULONG height,
                        int x, int y, int ey, int eu, int ev, const char *name)
{
    ULONG ysize = pitch * height;
    ULONG cpitch = pitch / 2;
    int Y = frame[y * pitch + x];
    int U = frame[ysize + (y / 2) * cpitch + x / 2];
    int V = frame[ysize + ysize / 4 + (y / 2) * cpitch + x / 2];
    BOOL ok = (Y - ey <= 10 && ey - Y <= 10 && U - eu <= 10 && eu - U <= 10 &&
               V - ev <= 10 && ev - V <= 10);

    printf("%-5s (%3d,%3d): Y %3d U %3d V %3d, want %3d %3d %3d %s\n", name,
           x, y, Y, U, V, ey, eu, ev, ok ? "ok" : "WRONG");
    return ok;
}

int main(int argc, char **argv)
{
    struct VCMMAL *mmal;
    struct VCMMALComponent *comp = NULL;
    struct VCMMALPort *in, *out;
    const UBYTE *src = quad_jpg;
    ULONG srclen = quad_jpg_len, srcpos = 0;
    UBYTE *filebuf = NULL, *frame = NULL;
    ULONG frame_size = 0, frame_len = 0;
    ULONG major = 0, minor = 0, i, idle = 0;
    int generation = 0;
    BOOL done = FALSE;
    LONG st;
    int rc = RETURN_FAIL;

    main_task = FindTask(NULL);
    InitSemaphore(&lock);
    NEWLIST((struct List *)&returned);

    if (argc > 1)
    {
        BPTR fh = Open(argv[1], MODE_OLDFILE);

        if (!fh)
        {
            printf("FAIL: can't open %s\n", argv[1]);
            return RETURN_FAIL;
        }
        Seek(fh, 0, OFFSET_END);
        srclen = Seek(fh, 0, OFFSET_BEGINNING);
        filebuf = AllocVec(srclen, MEMF_ANY);
        if (!filebuf || Read(fh, filebuf, srclen) != srclen)
        {
            printf("FAIL: can't read %s\n", argv[1]);
            Close(fh);
            return RETURN_FAIL;
        }
        Close(fh);
        src = filebuf;
    }

    mmal = VCMMAL_Open();
    if (!mmal)
    {
        printf("FAIL: VCMMAL_Open\n");
        goto out;
    }

    st = VCMMAL_GetVersion(mmal, &major, &minor);
    printf("MMAL server version: %ld, %u.%u\n", (long)st, (unsigned)major, (unsigned)minor);

    st = VCMMAL_ComponentCreate(mmal, "ril.image_decode", &comp);
    printf("create ril.image_decode: %ld\n", (long)st);
    if (st != MMAL_SUCCESS)
        goto close;
    comp->event_cb = event_cb;
    in = &comp->input[0];
    out = &comp->output[0];

    in->format.type = MMAL_ES_TYPE_VIDEO;
    in->format.encoding = MMAL_ENCODING_JPEG;
    in->port.buffer_num = max3(in->port.buffer_num_min, in->port.buffer_num_recommended, 1);
    in->port.buffer_size = max3(in->port.buffer_size_min, in->port.buffer_size_recommended, 4096);
    st = VCMMAL_PortFormatCommit(in);
    printf("input commit: %ld\n", (long)st);
    print_port("in ", in);

    out->format.type = MMAL_ES_TYPE_VIDEO;
    out->format.encoding = MMAL_ENCODING_I420;
    st = VCMMAL_PortFormatCommit(out);
    printf("output commit: %ld\n", (long)st);
    print_port("out", out);

    st = VCMMAL_PortEnable(&comp->control, NULL);
    printf("control enable: %ld\n", (long)st);
    st = VCMMAL_PortEnable(in, buffer_cb);
    printf("input enable: %ld\n", (long)st);
    st = VCMMAL_PortEnable(out, buffer_cb);
    printf("output enable: %ld\n", (long)st);
    st = VCMMAL_ComponentEnable(comp);
    printf("component enable: %ld\n", (long)st);
    if (st != MMAL_SUCCESS)
        goto destroy;

    for (i = 0; i < out->port.buffer_num; i++)
    {
        struct TBuf *tb = tbuf_alloc(out->port.buffer_size, generation);

        if (tb)
            VCMMAL_SendBuffer(out, &tb->b);
    }
    for (i = 0; i < in->port.buffer_num; i++)
    {
        struct TBuf *tb = tbuf_alloc(in->port.buffer_size, -1);

        if (tb)
        {
            ObtainSemaphore(&lock);
            AddTail((struct List *)&returned, (struct Node *)&tb->node);
            ReleaseSemaphore(&lock);
        }
    }

    while (!done && idle < TIMEOUT_TICKS)
    {
        struct TBuf *tb;
        ULONG sigs = SetSignal(0, SIGF_CB | SIGBREAKF_CTRL_C);
        BOOL busy = FALSE;

        if (sigs & SIGBREAKF_CTRL_C)
            break;
        if (error_event)
        {
            printf("FAIL: error event, status %d\n", (int)error_event);
            break;
        }

        if (format_changed)
        {
            format_changed = FALSE;
            busy = TRUE;
            printf("format changed: %ux%u, %u bufs of %u\n",
                   (unsigned)fc.es.video.width, (unsigned)fc.es.video.height,
                   (unsigned)fc.buffer_num_recommended,
                   (unsigned)fc.buffer_size_recommended);

            /* Old buffers come back through buffer_cb and are dropped */
            VCMMAL_PortDisable(out);
            generation++;
            out->format = fc.format;
            out->es = fc.es;
            out->port.buffer_num = max3(fc.buffer_num_min, fc.buffer_num_recommended, 1);
            out->port.buffer_size = max3(fc.buffer_size_min, fc.buffer_size_recommended, 0);
            st = VCMMAL_PortFormatCommit(out);
            printf("output re-commit: %ld\n", (long)st);
            print_port("out", out);
            st = VCMMAL_PortEnable(out, buffer_cb);
            printf("output re-enable: %ld\n", (long)st);
            for (i = 0; i < out->port.buffer_num; i++)
            {
                tb = tbuf_alloc(out->port.buffer_size, generation);
                if (tb)
                    VCMMAL_SendBuffer(out, &tb->b);
            }
        }

        for (;;)
        {
            ObtainSemaphore(&lock);
            tb = (struct TBuf *)RemHead((struct List *)&returned);
            ReleaseSemaphore(&lock);
            if (!tb)
                break;
            busy = TRUE;

            if (tb->generation >= 0)
            {
                if (tb->generation != generation)
                    continue;

                if (tb->b.length)
                {
                    if (frame_len + tb->b.length > frame_size)
                    {
                        UBYTE *nf = AllocVec(frame_len + tb->b.length, MEMF_ANY);

                        if (nf && frame)
                            CopyMem(frame, nf, frame_len);
                        FreeVec(frame);
                        frame = nf;
                        frame_size = frame_len + tb->b.length;
                    }
                    if (frame)
                        CopyMem(tb->b.data + tb->b.offset, frame + frame_len, tb->b.length);
                    frame_len += tb->b.length;
                    printf("output: %u bytes, flags 0x%x, planes %u pitch %u\n",
                           (unsigned)tb->b.length, (unsigned)tb->b.flags,
                           (unsigned)tb->b.video.planes, (unsigned)tb->b.video.pitch[0]);
                }
                if (tb->b.flags & (MMAL_BUFFER_HEADER_FLAG_FRAME_END |
                                   MMAL_BUFFER_HEADER_FLAG_EOS))
                {
                    if (frame_len)
                        done = TRUE;
                }
                else if (out->enabled)
                {
                    tb->b.length = tb->b.offset = tb->b.flags = 0;
                    VCMMAL_SendBuffer(out, &tb->b);
                }
            }
            else if (srcpos < srclen)
            {
                /* An input buffer: refill it */
                ULONG n = srclen - srcpos;

                if (n > tb->b.alloc_size)
                    n = tb->b.alloc_size;
                CopyMem((APTR)(src + srcpos), tb->b.data, n);
                srcpos += n;
                tb->b.offset = 0;
                tb->b.length = n;
                tb->b.pts = tb->b.dts = MMAL_TIME_UNKNOWN;
                tb->b.flags = (srcpos == srclen)
                    ? (MMAL_BUFFER_HEADER_FLAG_FRAME_END | MMAL_BUFFER_HEADER_FLAG_EOS)
                    : 0;
                st = VCMMAL_SendBuffer(in, &tb->b);
                printf("input: %u bytes, send %ld\n", (unsigned)n, (long)st);
            }
        }

        if (busy)
            idle = 0;
        else
        {
            Delay(1);
            idle++;
        }
    }

    if (!done)
    {
        printf("FAIL: no decoded frame (%s)\n",
               idle >= TIMEOUT_TICKS ? "timeout" : "stopped");
        goto disable;
    }

    printf("decoded %u bytes, %ux%u\n", (unsigned)frame_len,
           (unsigned)out->es.video.width, (unsigned)out->es.video.height);

    if (argc > 2)
    {
        BPTR fh = Open(argv[2], MODE_NEWFILE);

        if (fh)
        {
            Write(fh, frame, frame_len);
            Close(fh);
            printf("wrote %s (I420, %u wide, %u high including padding)\n", argv[2],
                   (unsigned)out->es.video.width, (unsigned)out->es.video.height);
        }
    }

    if (argc > 1)
        rc = RETURN_OK;
    else
    {
        ULONG pitch = out->es.video.width, height = out->es.video.height;

        if (frame_len < pitch * height * 3 / 2)
            printf("FAIL: frame too short for %ux%u\n", (unsigned)pitch, (unsigned)height);
        else
        {
            /* JFIF full-range YCbCr of the four colours */
            BOOL ok = TRUE;

            ok &= check_pixel(frame, pitch, height, 80, 60, 76, 85, 255, "red");
            ok &= check_pixel(frame, pitch, height, 240, 60, 150, 44, 21, "green");
            ok &= check_pixel(frame, pitch, height, 80, 180, 29, 255, 107, "blue");
            ok &= check_pixel(frame, pitch, height, 240, 180, 255, 128, 128, "white");
            if (ok)
                rc = RETURN_OK;
        }
    }

disable:
    VCMMAL_PortDisable(out);
    VCMMAL_PortDisable(in);
    VCMMAL_PortDisable(&comp->control);
    VCMMAL_ComponentDisable(comp);
destroy:
    VCMMAL_ComponentDestroy(comp);
close:
    VCMMAL_Close(mmal);
out:
    /* The VideoCore has given every buffer back by now */
    while (all_bufs)
    {
        struct TBuf *tb = all_bufs;

        all_bufs = tb->next_all;
        FreeVec(tb->b.data);
        FreeVec(tb);
    }
    FreeVec(frame);
    FreeVec(filebuf);
    printf("%s\n", rc == RETURN_OK ? "PASS" : "FAIL");

    return rc;
}
