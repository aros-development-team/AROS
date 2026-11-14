/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#ifndef UVC_PAYLOAD_H
#define UVC_PAYLOAD_H
#include <stddef.h>
#include <string.h>

/* A complete uncompressed frame must have one FID, no ERR payloads, an EOF,
 * and exactly the negotiated byte count. Never publish partial frames.
 * The counters record why frames were rejected, for the grab diagnostics. */
#define UVC_EOF_SAMPLES 6
struct uvc_payload {
    unsigned char *data;
    size_t capacity, used;
    unsigned int fid;
    int have_fid, damaged, saw_eof;
    int variable_length; /* compressed frames: EOF with a nonempty bounded frame */
    /* diagnostics */
    unsigned long fidflips;              /* FID toggles seen */
    unsigned long badhdr;                /* header length invalid */
    unsigned long errbit;                /* payloads with the ERR bit */
    unsigned long overshoot;             /* data beyond the frame's capacity */
    unsigned long afterEOF;              /* data after EOF with the same FID */
    unsigned long hdr2, hdr12, hdrother; /* header length histogram */
    unsigned long eofs;                  /* EOF payloads seen */
    size_t raw;                          /* data bytes seen for this FID, clipped or not */
    size_t eofused[UVC_EOF_SAMPLES];     /* frame bytes kept at the first EOFs */
    size_t eofraw[UVC_EOF_SAMPLES];      /* data bytes the device sent for those frames */
    unsigned char eofstate[UVC_EOF_SAMPLES]; /* 0 ok, 1 damaged, 2 short */
};
static int uvc_payload_feed(struct uvc_payload *s,
                           const unsigned char *p, size_t len)
{
    size_t n;
    unsigned int fid;
    if (len < 2 || p[0] < 2 || p[0] > len) {
        s->damaged = 1;
        s->badhdr++;
        return 0;
    }
    if (p[0] == 2) s->hdr2++;
    else if (p[0] == 12) s->hdr12++;
    else s->hdrother++;
    fid = p[1] & 1;
    if (!s->have_fid || fid != s->fid) {
        if (s->have_fid) s->fidflips++;
        s->fid = fid;
        s->have_fid = 1;
        s->used = 0;
        s->raw = 0;
        s->damaged = 0;
        s->saw_eof = 0;
    } else if (s->saw_eof && len > p[0]) {
        /* data for this FID after its EOF payload */
        s->afterEOF++;
    }
    if (p[1] & 0x40) { s->damaged = 1; s->errbit++; }
    n = len - p[0];
    s->raw += n;
    if (n > s->capacity - s->used) { s->damaged = 1; s->overshoot++; }
    if (!s->damaged) {
        memcpy(s->data + s->used, p + p[0], n);
        s->used += n;
    }
    if (p[1] & 2) {
        int complete = !s->damaged && (s->variable_length ?
            s->used > 0 : s->used == s->capacity);
        if (s->eofs < UVC_EOF_SAMPLES) {
            s->eofused[s->eofs] = s->used;
            s->eofraw[s->eofs] = s->raw;
            s->eofstate[s->eofs] = complete ? 0 : (s->damaged ? 1 : 2);
        }
        s->eofs++;
        /* Reject additional payloads with the same FID after EOF. */
        s->damaged = 1;
        s->saw_eof = 1;
        return complete;
    }
    return 0;
}
/* Bulk requests may split a UVC payload. Only its first bytes contain a
 * header; EOF is applied after the payload's last transfer. Start by dropping
 * data through a short transfer, since a continuously streaming device can
 * be opened in the middle of a payload. Never search pixels for headers. */
struct uvc_bulk_payload {
    unsigned char *data;
    size_t capacity, used;
    int synchronized;
};
static int uvc_bulk_feed(struct uvc_bulk_payload *b, struct uvc_payload *s,
                         const unsigned char *p, size_t len, size_t requested)
{
    int short_transfer = len < requested;
    int complete = 0;
    if (!requested || len > requested || !b->capacity) return 0;
    if (!b->synchronized) {
        if (short_transfer) b->synchronized = 1;
        b->used = 0;
        return 0;
    }
    if (len > b->capacity - b->used) {
        s->damaged = 1;
        b->used = 0;
        b->synchronized = short_transfer;
        return 0;
    }
    if (len) memcpy(b->data + b->used, p, len);
    b->used += len;
    if (short_transfer || b->used == b->capacity) {
        if (b->used) complete = uvc_payload_feed(s, b->data, b->used);
        b->used = 0;
    }
    return complete;
}
static void uvc_gray_rgb(const unsigned char *in, unsigned char *out,
                          size_t pixels, int sixteen_bit)
{
    size_t i;
    for (i = 0; i < pixels; i++) {
        unsigned char value = in[sixteen_bit ? 2*i+1 : i];
        out[3*i] = out[3*i+1] = out[3*i+2] = value;
    }
}
static unsigned char uvc_clip(int v)
{
    return (unsigned char)(v < 0 ? 0 : v > 255 ? 255 : v);
}
static void uvc_yuy2_rgb(const unsigned char *in, unsigned char *out, size_t n)
{
    size_t i;
    for (i = 0; i < n; i += 4) {
        int y0 = in[i] - 16, u = in[i+1] - 128;
        int y1 = in[i+2] - 16, v = in[i+3] - 128;
        out[0] = uvc_clip((298*y0 + 409*v + 128) >> 8);
        out[1] = uvc_clip((298*y0 - 100*u - 208*v + 128) >> 8);
        out[2] = uvc_clip((298*y0 + 516*u + 128) >> 8);
        out[3] = uvc_clip((298*y1 + 409*v + 128) >> 8);
        out[4] = uvc_clip((298*y1 - 100*u - 208*v + 128) >> 8);
        out[5] = uvc_clip((298*y1 + 516*u + 128) >> 8);
        out += 6;
    }
}
#endif
