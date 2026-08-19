/* DRM fourcc stub for AROS */
#ifndef DRM_FOURCC_H
#define DRM_FOURCC_H
#include <stdint.h>
#define fourcc_code(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#define DRM_FORMAT_MOD_INVALID 0x00ffffffffffffffULL
#define DRM_FORMAT_MOD_LINEAR 0
#define DRM_FORMAT_MOD_BROADCOM_UIF (((uint64_t)0x09) << 56 | 2)
#define DRM_FORMAT_MOD_BROADCOM_SAND128 (((uint64_t)0x09) << 56 | 3)
#define DRM_FORMAT_ARGB8888 fourcc_code('A','R','2','4')
#define DRM_FORMAT_XRGB8888 fourcc_code('X','R','2','4')
#define DRM_FORMAT_ABGR8888 fourcc_code('A','B','2','4')
#define DRM_FORMAT_XBGR8888 fourcc_code('X','B','2','4')
#define DRM_FORMAT_RGB565 fourcc_code('R','G','1','6')
#endif
