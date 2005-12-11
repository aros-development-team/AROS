#ifndef LINUX_INTERN_H
#define LINUX_INTERN_H

/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Linux hidd for AROS
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

/* Private Attrs and methods for the X11Gfx Hidd */

#define CLID_Hidd_LinuxFB	"hidd.gfx.linuxfb"

#define IID_Hidd_LinuxFB "hidd.gfx.linuxfb"


#define HiddLinuxFBAttrBase  __abHidd_LinuxFB
extern OOP_AttrBase HiddLinuxFBAttrBase;

enum {
    
    num_Hidd_LinuxFB_Attrs
    
};

/***** Linux Kbd HIDD *******************/

/* IDs */
#define IID_Hidd_LinuxKbd	"hidd.kbd.linux"
#define CLID_Hidd_LinuxKbd	"hidd.kbd.linux"

/* Methods */
enum
{
    moHidd_LinuxKbd_HandleEvent
};

struct pHidd_LinuxKbd_HandleEvent
{
    OOP_MethodID mID;
    UBYTE scanCode;
};
VOID HIDD_LinuxKbd_HandleEvent(OOP_Object *o, UBYTE scanCode);

/* Data */
struct linuxkbd_data
{
    VOID (*kbd_callback)(APTR, UWORD);
    APTR callbackdata;
};


/***** Linux Mouse HIDD *******************/

/* IDs */
#define IID_Hidd_LinuxMouse	"hidd.mouse.linux"
#define CLID_Hidd_LinuxMouse	"hidd.mouse.linux"


/* Methods */
enum
{
    moHidd_LinuxMouse_HandleEvent
};

struct pHidd_LinuxMouse_HandleEvent
{
    OOP_MethodID mID;
    struct pHidd_Mouse_Event *mouseEvent;    
};

VOID HIDD_LinuxMouse_HandleEvent(OOP_Object *o, struct pHidd_Mouse_Event *mouseEvent);

/* Data */
struct mouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;
};

/*** Shared data ***/
struct linux_staticdata {
    struct SignalSemaphore sema;
    
    struct ExecBase *sysbase;
    struct Library *utilitybase;
    
    OOP_Class *gfxclass;
    OOP_Class *bmclass;
    OOP_Class *kbdclass;
    OOP_Class *mouseclass;
    
    /* The device file */
    int fbdev;
    struct fb_fix_screeninfo fsi;
    struct fb_var_screeninfo vsi;
    
    HIDDT_PixelFormat pf;
    
    char *baseaddr;
    
    BOOL kbd_inited;
    int kbdfd;
    
    BOOL mouse_inited;
    int mousefd;
    
    struct Task *input_task;
    OOP_Object *kbdhidd;
    OOP_Object *mousehidd;
#if BUFFERED_VRAM
    struct SignalSemaphore framebufferlock;
#endif    
};

struct linux_base
{
    struct Library library;
    struct ExecBase *sysbase;
    BPTR	seglist;
    struct linux_staticdata lsd;
};

struct Task *init_linuxinput_task(struct linux_staticdata *lsd);
VOID kill_linuxinput_task(struct linux_staticdata *lsd);

BOOL init_linuxkbd(struct linux_staticdata *lsd);
VOID cleanup_linuxkbd(struct linux_staticdata *lsd);

BOOL init_linuxmouse(struct linux_staticdata *lsd);
VOID cleanup_linuxmouse(struct linux_staticdata *lsd);

struct BitmapData;

VOID fbRefreshArea(struct BitmapData *data, LONG x1, LONG y1, LONG x2, LONG y2);

#if BUFFERED_VRAM
#define LOCK_FRAMEBUFFER(lsd)	ObtainSemaphore(&lsd->framebufferlock)
#define UNLOCK_FRAMEBUFFER(lsd) ReleaseSemaphore(&lsd->framebufferlock)
#endif

#define LSD(cl) (&((struct linux_base *)cl->UserData)->lsd)

#define UtilityBase	(LSD(cl)->utilitybase)

#endif /* LINUX_INTERN_H */
