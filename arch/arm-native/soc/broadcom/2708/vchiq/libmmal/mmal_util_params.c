/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    libmmal util: typed parameter helpers (interface/mmal/util/mmal_util_params.h).
*/

#include <string.h>

#include "libmmal_private.h"

#define PARAM_SET(name, ctype, ptype, field) \
MMAL_STATUS_T mmal_port_parameter_set_##name(MMAL_PORT_T *port, uint32_t id, ctype value) \
{ \
    ptype param = {{id, sizeof(param)}, value}; \
    return mmal_port_parameter_set(port, &param.hdr); \
}

#define PARAM_GET(name, ctype, ptype, field) \
MMAL_STATUS_T mmal_port_parameter_get_##name(MMAL_PORT_T *port, uint32_t id, ctype *value) \
{ \
    ptype param; \
    MMAL_STATUS_T status; \
    memset(&param, 0, sizeof(param)); \
    param.hdr.id = id; \
    param.hdr.size = sizeof(param); \
    status = mmal_port_parameter_get(port, &param.hdr); \
    if (status == MMAL_SUCCESS) \
        *value = param.field; \
    return status; \
}

PARAM_SET(boolean, MMAL_BOOL_T, MMAL_PARAMETER_BOOLEAN_T, enable)
PARAM_GET(boolean, MMAL_BOOL_T, MMAL_PARAMETER_BOOLEAN_T, enable)
PARAM_SET(uint32, uint32_t, MMAL_PARAMETER_UINT32_T, value)
PARAM_GET(uint32, uint32_t, MMAL_PARAMETER_UINT32_T, value)
PARAM_SET(int32, int32_t, MMAL_PARAMETER_INT32_T, value)
PARAM_GET(int32, int32_t, MMAL_PARAMETER_INT32_T, value)
PARAM_SET(uint64, uint64_t, MMAL_PARAMETER_UINT64_T, value)
PARAM_GET(uint64, uint64_t, MMAL_PARAMETER_UINT64_T, value)
PARAM_SET(int64, int64_t, MMAL_PARAMETER_INT64_T, value)
PARAM_GET(int64, int64_t, MMAL_PARAMETER_INT64_T, value)
PARAM_SET(rational, MMAL_RATIONAL_T, MMAL_PARAMETER_RATIONAL_T, value)
PARAM_GET(rational, MMAL_RATIONAL_T, MMAL_PARAMETER_RATIONAL_T, value)
