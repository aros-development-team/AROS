/*
 * Copyright 1999, 2000 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#if !defined(AROSDRM_H)
#define AROSDRM_H

#include <drm.h>

typedef unsigned int  drmSize,     *drmSizePtr;     /**< For mapped regions */
typedef void          *drmAddress, **drmAddressPtr; /**< For mapped regions */

typedef struct _drmLock {
    volatile unsigned int lock;
    char                      padding[60];
    /* This is big enough for most current (and future?) architectures:
       DEC Alpha:              32 bytes
       Intel Merced:           ?
       Intel P5/PPro/PII/PIII: 32 bytes
       Intel StrongARM:        32 bytes
       Intel i386/i486:        16 bytes
       MIPS:                   32 bytes (?)
       Motorola 68k:           16 bytes
       Motorola PowerPC:       32 bytes
       Sun SPARC:              32 bytes
    */
} drmLock, *drmLockPtr;

typedef struct _drmVersion {
    int     version_major;        /**< Major version */
    int     version_minor;        /**< Minor version */
    int     version_patchlevel;   /**< Patch level */
    int     name_len;             /**< Length of name buffer */
    char    *name;            /**< Name of driver */
    int     date_len;             /**< Length of date buffer */
    char    *date;                /**< User-space buffer to hold date */
    int     desc_len;             /**< Length of desc buffer */
    char    *desc;                /**< User-space buffer to hold desc */
} drmVersion, *drmVersionPtr;

extern int           drmOpen(const char *name, const char *busid);
extern int           drmClose(int fd);
extern drmVersionPtr drmGetVersion(int fd);
extern void          drmFreeVersion(drmVersionPtr);
extern int           drmCommandNone(int fd, unsigned long drmCommandIndex);
extern int           drmCommandRead(int fd, unsigned long drmCommandIndex,
                                    void *data, unsigned long size);
extern int           drmCommandWrite(int fd, unsigned long drmCommandIndex,
                                     void *data, unsigned long size);
extern int           drmCommandWriteRead(int fd, unsigned long drmCommandIndex,
                                         void *data, unsigned long size);
extern int           drmCreateContext(int fd, drm_context_t * handle);
extern int           drmDestroyContext(int fd, drm_context_t handle);
extern int           drmIoctl(int fd, unsigned long request, void *arg);

extern void          *drmMalloc(int size);
extern void          drmFree(void *pt);

/* AROS specific */
extern void *       drmMMap(int fd, uint32_t handle);
extern void         drmMUnmap(int fd, uint32_t handle);

#endif
