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
 * The MMAL worker messages of raspberrypi/userland
 * interface/mmal/vc/mmal_vc_msgs.h in the VideoCore's 32-bit layout:
 * every host pointer is a 32-bit cookie here.
 */

#ifndef VCMMAL_MSGS_H
#define VCMMAL_MSGS_H

#include <hardware/vcmmal.h>

#define MMAL_MAGIC                  MMAL_FOURCC('m','m','a','l')
#define MMAL_CONTROL_FOURCC         VCHIQ_MAKE_FOURCC('m','m','a','l')

#define WORKER_VER_MAJOR            16
#define WORKER_VER_MINIMUM          10

#define MMAL_WORKER_MAX_MSG_LEN     512
#define MMAL_VC_SHORT_DATA          128
#define MMAL_WORKER_EVENT_SPACE     256
#define MMAL_WORKER_PORT_PARAMETER_SPACE 96

enum {
    MMAL_WORKER_QUIT = 1,
    MMAL_WORKER_SERVICE_CLOSED,
    MMAL_WORKER_GET_VERSION,
    MMAL_WORKER_COMPONENT_CREATE,
    MMAL_WORKER_COMPONENT_DESTROY,
    MMAL_WORKER_COMPONENT_ENABLE,
    MMAL_WORKER_COMPONENT_DISABLE,
    MMAL_WORKER_PORT_INFO_GET,
    MMAL_WORKER_PORT_INFO_SET,
    MMAL_WORKER_PORT_ACTION,
    MMAL_WORKER_BUFFER_FROM_HOST,
    MMAL_WORKER_BUFFER_TO_HOST,
    MMAL_WORKER_GET_STATS,
    MMAL_WORKER_PORT_PARAMETER_SET,
    MMAL_WORKER_PORT_PARAMETER_GET,
    MMAL_WORKER_EVENT_TO_HOST,
    MMAL_WORKER_GET_CORE_STATS_FOR_PORT,
    MMAL_WORKER_OPAQUE_ALLOCATOR,
    MMAL_WORKER_CONSUME_MEM,
    MMAL_WORKER_LMK,
    MMAL_WORKER_OPAQUE_ALLOCATOR_DESC,
    MMAL_WORKER_DRM_GET_LHS32,
    MMAL_WORKER_DRM_GET_TIME,
    MMAL_WORKER_BUFFER_FROM_HOST_ZEROLEN,
    MMAL_WORKER_PORT_FLUSH,
    MMAL_WORKER_HOST_LOG,
    MMAL_WORKER_COMPACT,
};

enum {
    MMAL_WORKER_PORT_ACTION_UNKNOWN = 0,
    MMAL_WORKER_PORT_ACTION_ENABLE,
    MMAL_WORKER_PORT_ACTION_DISABLE,
    MMAL_WORKER_PORT_ACTION_FLUSH,
    MMAL_WORKER_PORT_ACTION_CONNECT,
    MMAL_WORKER_PORT_ACTION_DISCONNECT,
    MMAL_WORKER_PORT_ACTION_SET_REQUIREMENTS,
};

typedef struct {
    uint32_t magic;
    uint32_t msgid;
    uint32_t control_service;
    uint32_t waiter;
    uint32_t status;
    uint32_t dummy;
} mmal_worker_msg_header;

typedef struct {
    mmal_worker_msg_header header;
    uint32_t flags;
    uint32_t major;
    uint32_t minor;
    uint32_t minimum;
} mmal_worker_version;

typedef struct {
    mmal_worker_msg_header header;
    uint32_t client_component;
    char name[128];
    uint32_t pid;
} mmal_worker_component_create;

typedef struct {
    mmal_worker_msg_header header;
    uint32_t status;
    uint32_t component_handle;
    uint32_t input_num;
    uint32_t output_num;
    uint32_t clock_num;
} mmal_worker_component_create_reply;

/* destroy, enable and disable */
typedef struct {
    mmal_worker_msg_header header;
    uint32_t component_handle;
} mmal_worker_component_op;

typedef struct {
    mmal_worker_msg_header header;
    uint32_t component_handle;
    uint32_t port_type;
    uint32_t index;
} mmal_worker_port_info_get;

typedef struct {
    mmal_worker_msg_header header;
    uint32_t component_handle;
    uint32_t port_type;
    uint32_t index;
    MMAL_PORT_WIRE_T port;
    MMAL_ES_FORMAT_WIRE_T format;
    MMAL_ES_SPECIFIC_FORMAT_T es;
    uint8_t extradata[MMAL_FORMAT_EXTRADATA_MAX_SIZE];
} mmal_worker_port_info_set;

typedef struct {
    mmal_worker_msg_header header;
    uint32_t status;
    uint32_t component_handle;
    uint32_t port_type;
    uint32_t index;
    int32_t found;
    uint32_t port_handle;
    MMAL_PORT_WIRE_T port;
    MMAL_ES_FORMAT_WIRE_T format;
    MMAL_ES_SPECIFIC_FORMAT_T es;
    uint8_t extradata[MMAL_FORMAT_EXTRADATA_MAX_SIZE];
} mmal_worker_port_info;

typedef struct {
    mmal_worker_msg_header header;
    uint32_t status;
} mmal_worker_reply;

typedef struct {
    mmal_worker_msg_header header;
    uint32_t component_handle;
    uint32_t port_handle;
    uint32_t action;
    union {
        struct {
            MMAL_PORT_WIRE_T port;
        } enable;
        struct {
            uint32_t component_handle;
            uint32_t port_handle;
        } connect;
    } param;
} mmal_worker_port_action;

typedef struct {
    uint32_t id;
    uint32_t size;
} MMAL_PARAMETER_HEADER_T;

/* set, and get */
typedef struct {
    mmal_worker_msg_header header;
    uint32_t component_handle;
    uint32_t port_handle;
    MMAL_PARAMETER_HEADER_T param;
    uint32_t space[MMAL_WORKER_PORT_PARAMETER_SPACE];
} mmal_worker_port_param;

typedef struct {
    mmal_worker_msg_header header;
    uint32_t status;
    MMAL_PARAMETER_HEADER_T param;
    uint32_t space[MMAL_WORKER_PORT_PARAMETER_SPACE];
} mmal_worker_port_param_get_reply;

typedef struct {
    uint32_t magic;
    uint32_t component_handle;
    uint32_t port_handle;
    uint32_t client_context;
} MMAL_DRIVER_BUFFER_T;

/* MMAL_BUFFER_HEADER_T */
typedef struct {
    uint32_t next;
    uint32_t priv;
    uint32_t cmd;
    uint32_t data;
    uint32_t alloc_size;
    uint32_t length;
    uint32_t offset;
    uint32_t flags;
    int64_t pts;
    int64_t dts;
    uint32_t type;
    uint32_t user_data;
} MMAL_BUFFER_HEADER_WIRE_T;

typedef struct {
    mmal_worker_msg_header header;
    MMAL_DRIVER_BUFFER_T drvbuf;
    MMAL_DRIVER_BUFFER_T drvbuf_ref;
    MMAL_BUFFER_HEADER_WIRE_T buffer_header;
    MMAL_BUFFER_HEADER_VIDEO_SPECIFIC_T buffer_header_type_specific;
    uint32_t is_zero_copy;
    uint32_t has_reference;
    uint32_t payload_in_message;
    uint8_t short_data[MMAL_VC_SHORT_DATA];
} mmal_worker_buffer_from_host;

typedef struct {
    mmal_worker_msg_header header;
    uint32_t client_component;
    uint32_t port_type;
    uint32_t port_num;
    uint32_t cmd;
    uint32_t length;
    uint8_t data[MMAL_WORKER_EVENT_SPACE];
    uint32_t delayed_buffer;
} mmal_worker_event_to_host;

/* MMAL_EVENT_FORMAT_CHANGED_T, followed by the format and its es */
typedef struct {
    uint32_t buffer_size_min;
    uint32_t buffer_num_min;
    uint32_t buffer_size_recommended;
    uint32_t buffer_num_recommended;
    uint32_t format;
} MMAL_EVENT_FORMAT_CHANGED_WIRE_T;

#define VCMMAL_WIRE_SIZE(t, n) \
    _Static_assert(sizeof(t) == (n), #t " is not the VideoCore's size")

VCMMAL_WIRE_SIZE(mmal_worker_msg_header, 24);
VCMMAL_WIRE_SIZE(MMAL_PORT_WIRE_T, 64);
VCMMAL_WIRE_SIZE(MMAL_ES_FORMAT_WIRE_T, 32);
VCMMAL_WIRE_SIZE(MMAL_ES_SPECIFIC_FORMAT_T, 44);
VCMMAL_WIRE_SIZE(MMAL_BUFFER_HEADER_WIRE_T, 56);
VCMMAL_WIRE_SIZE(mmal_worker_component_create, 160);
VCMMAL_WIRE_SIZE(mmal_worker_port_info_set, 304);
VCMMAL_WIRE_SIZE(mmal_worker_port_info, 316);
VCMMAL_WIRE_SIZE(mmal_worker_port_action, 100);
VCMMAL_WIRE_SIZE(mmal_worker_port_param, 424);
VCMMAL_WIRE_SIZE(mmal_worker_event_to_host, 304);
VCMMAL_WIRE_SIZE(MMAL_EVENT_FORMAT_CHANGED_WIRE_T, 20);

#endif /* VCMMAL_MSGS_H */
