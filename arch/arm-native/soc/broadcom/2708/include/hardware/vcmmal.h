/*
Copyright (c) 2012, Broadcom Europe Ltd
Copyright (C) 2026, The AROS Development Team.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * vcmmal: a client for the VideoCore MMAL components over vchiq.library
 * (link with -lvcmmal). The constants and the format layouts follow
 * raspberrypi/userland interface/mmal; the structures are the firmware's
 * 32-bit wire layout, so they carry no pointers.
 */

#ifndef HARDWARE_VCMMAL_H
#define HARDWARE_VCMMAL_H

#include <exec/types.h>
#include <stdint.h>

/*
 * With the MMAL headers (libmmal) included first, their definitions of
 * these names are used: same values and layouts.
 */
#ifndef MMAL_COMMON_H

#define MMAL_FOURCC(a, b, c, d) \
    ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

/* Status codes */
#define MMAL_SUCCESS            0
#define MMAL_ENOMEM             1
#define MMAL_ENOSPC             2
#define MMAL_EINVAL             3
#define MMAL_ENOSYS             4
#define MMAL_ENOENT             5
#define MMAL_ENXIO              6
#define MMAL_EIO                7
#define MMAL_ESPIPE             8
#define MMAL_ECORRUPT           9
#define MMAL_ENOTREADY          10
#define MMAL_ECONFIG            11
#define MMAL_EISCONN            12
#define MMAL_ENOTCONN           13
#define MMAL_EAGAIN             14
#define MMAL_EFAULT             15

/* Port types */
#define MMAL_PORT_TYPE_CONTROL  1
#define MMAL_PORT_TYPE_INPUT    2
#define MMAL_PORT_TYPE_OUTPUT   3
#define MMAL_PORT_TYPE_CLOCK    4

/* Elementary stream types */
#define MMAL_ES_TYPE_UNKNOWN    0
#define MMAL_ES_TYPE_CONTROL    1
#define MMAL_ES_TYPE_AUDIO      2
#define MMAL_ES_TYPE_VIDEO      3
#define MMAL_ES_TYPE_SUBPICTURE 4

#define MMAL_ES_FORMAT_FLAG_FRAMED  0x1

/* Encodings */
#define MMAL_ENCODING_H264      MMAL_FOURCC('H','2','6','4')
#define MMAL_ENCODING_MP4V      MMAL_FOURCC('M','P','4','V')
#define MMAL_ENCODING_MP2V      MMAL_FOURCC('M','P','2','V')
#define MMAL_ENCODING_MJPEG     MMAL_FOURCC('M','J','P','G')
#define MMAL_ENCODING_JPEG      MMAL_FOURCC('J','P','E','G')
#define MMAL_ENCODING_I420      MMAL_FOURCC('I','4','2','0')
#define MMAL_ENCODING_YV12      MMAL_FOURCC('Y','V','1','2')
#define MMAL_ENCODING_I422      MMAL_FOURCC('I','4','2','2')
#define MMAL_ENCODING_YUYV      MMAL_FOURCC('Y','U','Y','V')
#define MMAL_ENCODING_NV12      MMAL_FOURCC('N','V','1','2')
#define MMAL_ENCODING_RGBA      MMAL_FOURCC('R','G','B','A')
#define MMAL_ENCODING_RGB24     MMAL_FOURCC('R','G','B','3')
#define MMAL_ENCODING_BGR24     MMAL_FOURCC('B','G','R','3')
#define MMAL_ENCODING_OPAQUE    MMAL_FOURCC('O','P','Q','V')

#define MMAL_ENCODING_VARIANT_H264_DEFAULT  0
#define MMAL_ENCODING_VARIANT_H264_RAW      MMAL_FOURCC('R','A','W',' ')

#define MMAL_COLOR_SPACE_UNKNOWN        0
#define MMAL_COLOR_SPACE_ITUR_BT601     MMAL_FOURCC('Y','6','0','1')
#define MMAL_COLOR_SPACE_ITUR_BT709     MMAL_FOURCC('Y','7','0','9')

/* Events */
#define MMAL_EVENT_ERROR                MMAL_FOURCC('E','R','R','O')
#define MMAL_EVENT_EOS                  MMAL_FOURCC('E','E','O','S')
#define MMAL_EVENT_FORMAT_CHANGED       MMAL_FOURCC('E','F','C','H')
#define MMAL_EVENT_PARAMETER_CHANGED    MMAL_FOURCC('E','P','C','H')

/* Buffer header flags */
#define MMAL_BUFFER_HEADER_FLAG_EOS                 (1 << 0)
#define MMAL_BUFFER_HEADER_FLAG_FRAME_START         (1 << 1)
#define MMAL_BUFFER_HEADER_FLAG_FRAME_END           (1 << 2)
#define MMAL_BUFFER_HEADER_FLAG_FRAME               (MMAL_BUFFER_HEADER_FLAG_FRAME_START | \
                                                     MMAL_BUFFER_HEADER_FLAG_FRAME_END)
#define MMAL_BUFFER_HEADER_FLAG_KEYFRAME            (1 << 3)
#define MMAL_BUFFER_HEADER_FLAG_DISCONTINUITY       (1 << 4)
#define MMAL_BUFFER_HEADER_FLAG_CONFIG              (1 << 5)
#define MMAL_BUFFER_HEADER_FLAG_CORRUPTED           (1 << 9)
#define MMAL_BUFFER_HEADER_FLAG_TRANSMISSION_FAILED (1 << 10)
#define MMAL_BUFFER_HEADER_FLAG_DECODEONLY          (1 << 11)
#define MMAL_BUFFER_HEADER_FLAG_NAL_END             (1 << 12)

#define MMAL_TIME_UNKNOWN       ((int64_t)1 << 63)

/* Parameters (MMAL_PARAMETER_GROUP_VIDEO + n) */
#define MMAL_PARAMETER_VIDEO_INTERPOLATE_TIMESTAMPS 0x2002f  /* MMAL_BOOL_T */

/*
 * Wire layouts
 */
typedef struct {
    int32_t x, y, width, height;
} MMAL_RECT_T;

typedef struct {
    int32_t num, den;
} MMAL_RATIONAL_T;

typedef struct {
    uint32_t width;
    uint32_t height;
    MMAL_RECT_T crop;
    MMAL_RATIONAL_T frame_rate;
    MMAL_RATIONAL_T par;
    uint32_t color_space;
} MMAL_VIDEO_FORMAT_T;

typedef struct {
    uint32_t channels;
    uint32_t sample_rate;
    uint32_t bits_per_sample;
    uint32_t block_align;
} MMAL_AUDIO_FORMAT_T;

typedef union {
    MMAL_AUDIO_FORMAT_T audio;
    MMAL_VIDEO_FORMAT_T video;
} MMAL_ES_SPECIFIC_FORMAT_T;

#endif /* MMAL_COMMON_H */

/* MMAL_ES_FORMAT_T; es and extradata are pointers on the VideoCore */
typedef struct {
    uint32_t type;
    uint32_t encoding;
    uint32_t encoding_variant;
    uint32_t es;
    uint32_t bitrate;
    uint32_t flags;
    uint32_t extradata_size;
    uint32_t extradata;
} MMAL_ES_FORMAT_WIRE_T;

/* MMAL_PORT_T, likewise */
typedef struct {
    uint32_t priv;
    uint32_t name;
    uint32_t type;
    uint16_t index;
    uint16_t index_all;
    uint32_t is_enabled;
    uint32_t format;
    uint32_t buffer_num_min;
    uint32_t buffer_size_min;
    uint32_t buffer_alignment_min;
    uint32_t buffer_num_recommended;
    uint32_t buffer_size_recommended;
    uint32_t buffer_num;
    uint32_t buffer_size;
    uint32_t component;
    uint32_t userdata;
    uint32_t capabilities;
} MMAL_PORT_WIRE_T;

#define MMAL_FORMAT_EXTRADATA_MAX_SIZE  128

#ifndef MMAL_COMMON_H
typedef struct {
    uint32_t planes;
    uint32_t offset[4];
    uint32_t pitch[4];
    uint32_t flags;
} MMAL_BUFFER_HEADER_VIDEO_SPECIFIC_T;

#endif /* MMAL_COMMON_H */

/*
 * The client
 */
struct VCMMAL;
struct VCMMALPort;

/* A buffer the client owns; data must stay valid while the VideoCore has it */
struct VCMMALBuffer {
    UBYTE *data;
    ULONG alloc_size;
    ULONG length;
    ULONG offset;
    ULONG flags;
    int64_t pts;
    int64_t dts;
    MMAL_BUFFER_HEADER_VIDEO_SPECIFIC_T video;
    APTR user_data;
    /* private */
    struct VCMMALPort *port;
    ULONG cookie;
};

/* Called on the client's worker task when the VideoCore hands a buffer back */
typedef void (*VCMMAL_BUFFER_CB)(struct VCMMALPort *port, struct VCMMALBuffer *buf);

/* Called on the worker task for events; data is the event payload */
typedef void (*VCMMAL_EVENT_CB)(struct VCMMALPort *port, ULONG cmd,
                                const UBYTE *data, ULONG length);

struct VCMMALPort {
    struct VCMMALComponent *component;
    ULONG type;
    ULONG index;
    ULONG handle;
    BOOL enabled;
    MMAL_PORT_WIRE_T port;
    MMAL_ES_FORMAT_WIRE_T format;
    MMAL_ES_SPECIFIC_FORMAT_T es;
    UBYTE extradata[MMAL_FORMAT_EXTRADATA_MAX_SIZE];
    VCMMAL_BUFFER_CB buffer_cb;
    APTR user_data;
};

#define VCMMAL_MAX_PORTS    8

struct VCMMALComponent {
    struct VCMMAL *client;
    ULONG handle;
    ULONG cookie;
    struct VCMMALPort control;
    ULONG input_num, output_num, clock_num;
    struct VCMMALPort input[VCMMAL_MAX_PORTS];
    struct VCMMALPort output[VCMMAL_MAX_PORTS];
    struct VCMMALPort clock[VCMMAL_MAX_PORTS];
    VCMMAL_EVENT_CB event_cb;
    APTR user_data;
};

/* Event payload of MMAL_EVENT_FORMAT_CHANGED, decoded */
struct VCMMALFormatChanged {
    ULONG buffer_size_min;
    ULONG buffer_num_min;
    ULONG buffer_size_recommended;
    ULONG buffer_num_recommended;
    MMAL_ES_FORMAT_WIRE_T format;
    MMAL_ES_SPECIFIC_FORMAT_T es;
};

/* All return an MMAL_* status unless noted */
struct VCMMAL *VCMMAL_Open(void);
void VCMMAL_Close(struct VCMMAL *mmal);
LONG VCMMAL_GetVersion(struct VCMMAL *mmal, ULONG *major, ULONG *minor);

/* name without the "vc." prefix, e.g. "ril.video_decode" */
LONG VCMMAL_ComponentCreate(struct VCMMAL *mmal, const char *name,
                            struct VCMMALComponent **pcomp);
LONG VCMMAL_ComponentDestroy(struct VCMMALComponent *comp);
LONG VCMMAL_ComponentEnable(struct VCMMALComponent *comp);
LONG VCMMAL_ComponentDisable(struct VCMMALComponent *comp);

LONG VCMMAL_PortInfoGet(struct VCMMALPort *port);
/* Sends port->format, es and extradata, and buffer_num/size from port->port */
LONG VCMMAL_PortFormatCommit(struct VCMMALPort *port);
LONG VCMMAL_PortEnable(struct VCMMALPort *port, VCMMAL_BUFFER_CB cb);
LONG VCMMAL_PortDisable(struct VCMMALPort *port);
LONG VCMMAL_PortFlush(struct VCMMALPort *port);
LONG VCMMAL_PortParameterSet(struct VCMMALPort *port, ULONG id,
                             const void *value, ULONG size);
LONG VCMMAL_PortParameterGet(struct VCMMALPort *port, ULONG id,
                             void *value, ULONG *size);

/* Input: buf->length bytes from buf->data + offset. Output: an empty buffer */
LONG VCMMAL_SendBuffer(struct VCMMALPort *port, struct VCMMALBuffer *buf);

/* Decode an EFCH event payload */
BOOL VCMMAL_FormatChangedGet(const UBYTE *data, ULONG length,
                             struct VCMMALFormatChanged *fc);

#endif /* HARDWARE_VCMMAL_H */
