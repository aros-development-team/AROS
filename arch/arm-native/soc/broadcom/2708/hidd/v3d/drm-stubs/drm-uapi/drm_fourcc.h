/* DRM fourcc stub for AROS */
#ifndef DRM_FOURCC_H
#define DRM_FOURCC_H
#include <stdint.h>
#define fourcc_code(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#define DRM_FORMAT_MOD_INVALID 0x00ffffffffffffffULL
#define DRM_FORMAT_MOD_LINEAR 0

/* Real DRM encoding: vendor in the top 8 bits, 48 bits of parameter above an
 * 8 bit type. The driver both compares against these and takes them apart. */
#define DRM_FORMAT_MOD_VENDOR_BROADCOM 0x07
#define fourcc_mod_code(vendor, val) \
    ((((uint64_t)DRM_FORMAT_MOD_VENDOR_##vendor) << 56) | ((val) & 0x00ffffffffffffffULL))

#define __fourcc_mod_broadcom_param_shift 8
#define __fourcc_mod_broadcom_param_bits 48
#define fourcc_mod_broadcom_code(val, params) \
    fourcc_mod_code(BROADCOM, ((((uint64_t)(params)) << __fourcc_mod_broadcom_param_shift) | (val)))
#define fourcc_mod_broadcom_param(m) \
    ((int)(((m) >> __fourcc_mod_broadcom_param_shift) & \
           ((1ULL << __fourcc_mod_broadcom_param_bits) - 1)))
#define fourcc_mod_broadcom_mod(m) \
    ((m) & ~(((1ULL << __fourcc_mod_broadcom_param_bits) - 1) << \
             __fourcc_mod_broadcom_param_shift))

#define DRM_FORMAT_MOD_BROADCOM_UIF fourcc_mod_code(BROADCOM, 6)
#define DRM_FORMAT_MOD_BROADCOM_SAND128 fourcc_mod_broadcom_code(4, 0)
#define DRM_FORMAT_ARGB8888 fourcc_code('A','R','2','4')
#define DRM_FORMAT_XRGB8888 fourcc_code('X','R','2','4')
#define DRM_FORMAT_ABGR8888 fourcc_code('A','B','2','4')
#define DRM_FORMAT_XBGR8888 fourcc_code('X','B','2','4')
#define DRM_FORMAT_RGB565 fourcc_code('R','G','1','6')
#endif
