#ifndef LINUX_INTERN_H
#define LINUX_INTERN_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Linux framebuffer hidd for AROS
    Lang: English.
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif
#ifndef HIDD_GRAPHICS_H
#   include <hidd/graphics.h>
#endif

/* hack: prevent linux include header <bits/time.h> to re-define timeval struct */
#  define _STRUCT_TIMEVAL 1

#include <linux/fb.h>
#include <linux/kd.h>
#include <termio.h>

#define BUFFERED_VRAM 1

/* Private Attrs and methods for the LinuxFB Hidd */

#define CLID_Hidd_LinuxFB "hidd.gfx.linuxfb"
#define IID_Hidd_LinuxFB  "hidd.gfx.linuxfb"

#define HiddLinuxFBAttrBase  __abHidd_LinuxFB
extern OOP_AttrBase HiddLinuxFBAttrBase;

enum
{
    aoHidd_LinuxFB_File,
    num_Hidd_LinuxFB_Attrs   
};

#define aHidd_LinuxFB_File HiddLinuxFBAttrBase + aoHidd_LinuxFB_File

struct LinuxFB_data
{
    /* The device file */
    int fbdev;
    char *baseaddr;
    unsigned long mem_len;
#if BUFFERED_VRAM
    struct SignalSemaphore framebufferlock;
#endif    
};

struct LibCInterface
{
    int	  (*open)(char *path, int oflag, ...);
    int	  (*close)(int filedes);
    int	  (*ioctl)(int d, int request, ...);
    void *(*mmap)(void *start, size_t length, int prot, int flags, int fd, off_t offset);
    int   (*munmap)(void *start, size_t length);
};

/*** Shared data ***/
struct linux_staticdata
{
    struct SignalSemaphore sema;
    
    OOP_Class *gfxclass;
    OOP_Class *bmclass;
    APTR hostlibBase;
    struct LibCInterface *SysIFace;
};

struct linux_base
{
    struct Library library;
    APTR libcHandle;
    struct linux_staticdata lsd;
};

struct BitmapData;

VOID fbRefreshArea(struct BitmapData *data, LONG x1, LONG y1, LONG x2, LONG y2);

#if BUFFERED_VRAM
#define LOCK_FRAMEBUFFER(data)	ObtainSemaphore(&data->framebufferlock)
#define UNLOCK_FRAMEBUFFER(data) ReleaseSemaphore(&data->framebufferlock)
#endif

#define LSD(cl) (&((struct linux_base *)cl->UserData)->lsd)

#endif /* LINUX_INTERN_H */
