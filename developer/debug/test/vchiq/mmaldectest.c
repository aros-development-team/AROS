/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Test for libmmal: decode the built-in H.264 clips through the MMAL API
    the way FFmpeg's mmaldec.c drives it (pools, a queue of decoded
    frames, the format change event) and compare every frame with the
    encoder's reconstruction.

    Exit code 0 when all frames of all clips match, 20 otherwise.
*/

#include <dos/dos.h>
#include <proto/exec.h>

#include <bcm_host.h>
#include <interface/mmal/mmal.h>
#include <interface/mmal/util/mmal_util_params.h>
#include <interface/mmal/util/mmal_default_components.h>
#include <interface/mmal/vc/mmal_vc_api.h>

#include <stdio.h>
#include <string.h>

#include "clip_h264.h"

#define CHUNK       1000
#define NUM_CLIPS   (sizeof(clips) / sizeof(clips[0]))

struct Dec {
    MMAL_COMPONENT_T *decoder;
    MMAL_POOL_T *pool_in;
    MMAL_POOL_T *pool_out;
    MMAL_QUEUE_T *decoded;
};

static ULONG crc32_update(ULONG crc, const UBYTE *p, ULONG len)
{
    ULONG k;

    crc = ~crc;
    while (len--)
    {
        crc ^= *p++;
        for (k = 0; k < 8; k++)
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
    }
    return ~crc;
}

static void input_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
    mmal_buffer_header_release(buffer);
}

static void output_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
    struct Dec *dec = (struct Dec *)port->userdata;

    mmal_queue_put(dec->decoded, buffer);
}

static void control_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
    if (buffer->cmd == MMAL_EVENT_ERROR)
        printf("  MMAL error %d on the control port\n", (int)*(uint32_t *)buffer->data);
    mmal_buffer_header_release(buffer);
}

/* As mmaldec.c's ffmal_update_format() */
static MMAL_STATUS_T update_format(struct Dec *dec)
{
    MMAL_PORT_T *out = dec->decoder->output[0];
    MMAL_STATUS_T st;

    if (dec->pool_out)
        mmal_pool_destroy(dec->pool_out);
    dec->pool_out = NULL;

    if ((st = mmal_port_parameter_set_uint32(out, MMAL_PARAMETER_EXTRA_BUFFERS, 0)))
        return st;
    if ((st = mmal_port_parameter_set_boolean(out, MMAL_PARAMETER_VIDEO_INTERPOLATE_TIMESTAMPS, 0)))
        return st;
    out->format->encoding_variant = out->format->encoding = MMAL_ENCODING_I420;
    if ((st = mmal_port_format_commit(out)))
        return st;

    out->buffer_size = MMAL_MAX(out->buffer_size_min, out->buffer_size_recommended);
    out->buffer_num = MMAL_MAX(out->buffer_num_min, out->buffer_num_recommended);
    dec->pool_out = mmal_pool_create(out->buffer_num, out->buffer_size);
    return dec->pool_out ? MMAL_SUCCESS : MMAL_ENOMEM;
}

static void fill_output(struct Dec *dec)
{
    MMAL_BUFFER_HEADER_T *b;

    while (dec->pool_out && (b = mmal_queue_get(dec->pool_out->queue)))
    {
        if (mmal_port_send_buffer(dec->decoder->output[0], b))
        {
            mmal_buffer_header_release(b);
            break;
        }
    }
}

static ULONG decode_clip(ULONG c)
{
    struct Dec dec;
    MMAL_PORT_T *in, *out;
    MMAL_ES_FORMAT_T *fmt;
    MMAL_STATUS_T st;
    ULONG pos = 0, frames = 0, matched = 0, waits = 0;
    BOOL eos_sent = FALSE, done = FALSE;

    printf("clip %u: %s, %u bytes\n", (unsigned)c, clips[c].name, (unsigned)clips[c].len);
    memset(&dec, 0, sizeof(dec));

    if ((st = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_DECODER, &dec.decoder)))
    {
        printf("  mmal_component_create: %d\n", (int)st);
        return 0;
    }
    in = dec.decoder->input[0];
    out = dec.decoder->output[0];

    fmt = in->format;
    fmt->type = MMAL_ES_TYPE_VIDEO;
    fmt->encoding = MMAL_ENCODING_H264;
    fmt->es->video.width = 320;
    fmt->es->video.height = 240;
    fmt->es->video.crop.width = 320;
    fmt->es->video.crop.height = 240;
    fmt->es->video.frame_rate.num = 24000;
    fmt->es->video.frame_rate.den = 1001;
    if ((st = mmal_port_format_commit(in)))
    {
        printf("  input commit: %d\n", (int)st);
        goto out;
    }
    in->buffer_num = MMAL_MAX(in->buffer_num_min, 20);
    in->buffer_size = MMAL_MAX(in->buffer_size_min, 64 * 1024);
    dec.pool_in = mmal_pool_create(in->buffer_num, 0);
    dec.decoded = mmal_queue_create();
    if (!dec.pool_in || !dec.decoded || (st = update_format(&dec)))
    {
        printf("  setup failed (%d)\n", (int)st);
        goto out;
    }

    in->userdata = out->userdata = dec.decoder->control->userdata =
        (struct MMAL_PORT_USERDATA_T *)&dec;
    if ((st = mmal_port_enable(dec.decoder->control, control_cb)) ||
        (st = mmal_port_enable(in, input_cb)) ||
        (st = mmal_port_enable(out, output_cb)) ||
        (st = mmal_component_enable(dec.decoder)))
    {
        printf("  enable failed (%d)\n", (int)st);
        goto out;
    }

    while (!done)
    {
        MMAL_BUFFER_HEADER_T *b;

        /* Feed the stream, then an empty end-of-stream buffer */
        while (!eos_sent && (b = mmal_queue_get(dec.pool_in->queue)))
        {
            mmal_buffer_header_reset(b);
            b->cmd = 0;
            if (pos < clips[c].len)
            {
                b->data = (uint8_t *)clips[c].data + pos;
                b->length = MMAL_MIN(CHUNK, clips[c].len - pos);
                pos += b->length;
            }
            else
            {
                b->data = NULL;
                b->length = 0;
                b->flags = MMAL_BUFFER_HEADER_FLAG_EOS;
                eos_sent = TRUE;
            }
            b->alloc_size = in->buffer_size;
            if (mmal_port_send_buffer(in, b))
            {
                printf("  input send failed\n");
                mmal_buffer_header_release(b);
                goto out;
            }
        }
        fill_output(&dec);

        b = mmal_queue_timedwait(dec.decoded, 100);
        if (!b)
        {
            if (++waits > 50)
            {
                printf("  timeout\n");
                break;
            }
            continue;
        }
        waits = 0;

        if (b->cmd == MMAL_EVENT_FORMAT_CHANGED)
        {
            MMAL_EVENT_FORMAT_CHANGED_T *ev = mmal_event_format_changed_get(b);
            MMAL_BUFFER_HEADER_T *stale;

            printf("  format changed: %ux%u, crop %ux%u\n",
                   (unsigned)ev->format->es->video.width,
                   (unsigned)ev->format->es->video.height,
                   (unsigned)ev->format->es->video.crop.width,
                   (unsigned)ev->format->es->video.crop.height);
            mmal_port_disable(out);
            while ((stale = mmal_queue_get(dec.decoded)))
                mmal_buffer_header_release(stale);
            mmal_format_copy(out->format, ev->format);
            mmal_buffer_header_release(b);
            if ((st = update_format(&dec)) || (st = mmal_port_enable(out, output_cb)))
            {
                printf("  output reconfiguration failed (%d)\n", (int)st);
                break;
            }
            continue;
        }

        if (b->flags & MMAL_BUFFER_HEADER_FLAG_EOS)
            done = TRUE;

        if (!b->cmd && b->length)
        {
            ULONG w = (out->format->es->video.width + 31) & ~31;
            ULONG h = (out->format->es->video.height + 15) & ~15;
            const UBYTE *d = b->data + b->type->video.offset[0];
            ULONG y, crc = 0;
            BOOL ok;

            for (y = 0; y < 240; y++)
                crc = crc32_update(crc, d + y * w, 320);
            for (y = 0; y < 120; y++)
                crc = crc32_update(crc, d + w * h + y * (w / 2), 160);
            for (y = 0; y < 120; y++)
                crc = crc32_update(crc, d + w * h * 5 / 4 + y * (w / 2), 160);

            ok = (frames < CLIP_FRAMES && crc == clips[c].crc[frames]);
            printf("  frame %u: %u bytes, crc 0x%08x %s\n", (unsigned)frames,
                   (unsigned)b->length, (unsigned)crc, ok ? "ok" : "MISMATCH");
            if (ok)
                matched++;
            frames++;
        }
        mmal_buffer_header_release(b);
    }

    printf("  %u frames, %u of %u match%s\n", (unsigned)frames, (unsigned)matched,
           CLIP_FRAMES, done ? "" : ", no end of stream");
    if (!done || frames != CLIP_FRAMES)
        matched = 0;

out:
    /* As mmaldec.c's ffmmal_stop_decoder() and close */
    if (dec.decoder)
    {
        MMAL_BUFFER_HEADER_T *b;

        mmal_port_disable(dec.decoder->input[0]);
        mmal_port_disable(dec.decoder->output[0]);
        mmal_port_disable(dec.decoder->control);
        mmal_port_flush(dec.decoder->input[0]);
        mmal_port_flush(dec.decoder->output[0]);
        mmal_port_flush(dec.decoder->control);
        if (dec.decoded)
            while ((b = mmal_queue_get(dec.decoded)))
                mmal_buffer_header_release(b);
        mmal_component_destroy(dec.decoder);
    }
    if (dec.decoded)
        mmal_queue_destroy(dec.decoded);
    if (dec.pool_in)
        mmal_pool_destroy(dec.pool_in);
    if (dec.pool_out)
        mmal_pool_destroy(dec.pool_out);

    return matched;
}

int main(void)
{
    uint32_t major = 0, minor = 0, minimum = 0;
    ULONG c, good = 0;

    bcm_host_init();
    if (mmal_vc_init())
    {
        printf("FAIL: mmal_vc_init\n");
        return RETURN_FAIL;
    }
    mmal_vc_get_version(&major, &minor, &minimum);
    printf("MMAL server %u.%u\n", (unsigned)major, (unsigned)minor);

    for (c = 0; c < NUM_CLIPS; c++)
    {
        if (decode_clip(c) == CLIP_FRAMES)
            good++;
    }
    mmal_vc_deinit();

    printf("%u of %u clips decoded bit-exact\n", (unsigned)good, (unsigned)NUM_CLIPS);
    printf("%s\n", good == NUM_CLIPS ? "PASS" : "FAIL");
    return (good == NUM_CLIPS) ? RETURN_OK : RETURN_FAIL;
}
