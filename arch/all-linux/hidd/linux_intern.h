#ifndef LINUX_INTERN_H
#define LINUX_INTERN_H
/*
    (C) 1997 AROS - The Amiga Research OS
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

#include <linux/fb.h>
#include <linux/kd.h>
#include <termio.h>

/* Private Attrs and methods for the X11Gfx Hidd */

#define CLID_Hidd_LinuxFB	"hidd.gfx.linuxfb"

#define IID_Hidd_LinuxFB "hidd.gfx.linuxfb"


#define HiddLinuxFBAttrBase  __abHidd_LinuxFB
extern AttrBase HiddLinuxFBAttrBase;

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
    MethodID mID;
    UBYTE scanCode;
};
VOID HIDD_LinuxKbd_HandleEvent(Object *o, UBYTE scanCode);

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
    MethodID mID;
    struct pHidd_Mouse_Event *mouseEvent;    
};

VOID HIDD_LinuxMouse_HandleEvent(Object *o, struct pHidd_Mouse_Event *mouseEvent);


/*** Shared data ***/
struct linux_staticdata {
    struct SignalSemaphore sema;
    
    struct ExecBase *sysbase;
    struct Library *oopbase;
    struct Library *utilitybase;
    
    Class *gfxclass;
    Class *bmclass;
    Class *kbdclass;
    Class *mouseclass;
    
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
    Object *kbdhidd;
    Object *mousehidd;
};

Class *init_gfxclass (struct linux_staticdata *lsd);
VOID free_gfxclass(struct linux_staticdata *lsd);

Class *init_bmclass(struct linux_staticdata *lsd);
VOID free_bmclass(struct linux_staticdata *lsd);

Class *init_mouseclass(struct linux_staticdata *lsd);
VOID free_mouseclass(struct linux_staticdata *lsd);

Class *init_kbdclass(struct linux_staticdata *lsd);
VOID free_kbdclass(struct linux_staticdata *lsd);

struct Task *init_input_task(struct linux_staticdata *lsd);
VOID kill_input_task(struct linux_staticdata *lsd);

BOOL init_kbd(struct linux_staticdata *lsd);
VOID cleanup_kbd(struct linux_staticdata *lsd);

BOOL init_mouse(struct linux_staticdata *lsd);
VOID cleanup_mouse(struct linux_staticdata *lsd);


#define LSD(cl) ((struct linux_staticdata *)cl->UserData)

#define OOPBase		(LSD(cl)->oopbase)
#define UtilityBase	(LSD(cl)->utilitybase)

#define expunge() \
AROS_LC0(BPTR, expunge, struct linux_base *, LIBBASE, 3, Linux)

#endif /* LINUX_INTERN_H */
