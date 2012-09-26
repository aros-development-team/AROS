/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Linux hidd handling keyboard events.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <dos/dos.h>

#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/dos.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <devices/inputevent.h>
#include <devices/rawkeycodes.h>

#include <aros/symbolsets.h>

#include "linuxinput_intern.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

static UBYTE scancode2rawkey[256];
static BOOL havetable = FALSE;

static UWORD scancode2hidd(UBYTE scancode, struct LinuxInput_staticdata *lsd);

/***** Kbd::New()  ***************************************/
OOP_Object * LinuxKbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL            has_kbd_hidd = FALSE;
    struct TagItem *tag, *tstate;
    APTR            callback = NULL;
    APTR            callbackdata = NULL;
    
    EnterFunc(bug("[LinuxInput] Kbd::New()\n"));
 
    ObtainSemaphore(&LSD(cl)->sema);
    if (LSD(cl)->kbdhidd)
        has_kbd_hidd = TRUE;
    ReleaseSemaphore(&LSD(cl)->sema);
 
    if (has_kbd_hidd) /* Cannot open twice */
        ReturnPtr("[LinuxInput] Kbd::New", OOP_Object *, NULL); /* Should have some error code here */

    tstate = msg->attrList;
    D(bug("[LinuxInput] tstate: %p, tag=%x\n", tstate, tstate->ti_Tag));
    while ((tag = NextTagItem((struct TagItem **)&tstate)))
    {
        ULONG idx;
    
        D(bug("[LinuxInput] Got tag %d, data %x\n", tag->ti_Tag, tag->ti_Data));
        
        if (IS_HIDDKBD_ATTR(tag->ti_Tag, idx))
        {
            D(bug("[LinuxInput] Kbd hidd tag\n"));
            switch (idx)
            {
            case aoHidd_Kbd_IrqHandler:
                callback = (APTR)tag->ti_Data;
                D(bug("Got callback %p\n", (APTR)tag->ti_Data));
                break;

            case aoHidd_Kbd_IrqHandlerData:
                callbackdata = (APTR)tag->ti_Data;
                D(bug("Got data %p\n", (APTR)tag->ti_Data));
                break;
            }
        }
        
    } /* while (tags to process) */

    if (NULL == callback)
        ReturnPtr("[LinuxInput] Kbd::New", OOP_Object *, NULL); /* Should have some error code here */

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct LinuxKbd_data *data = OOP_INST_DATA(cl, o);
    
        data->kbd_callback = (VOID (*)(APTR, UWORD))callback;
        data->callbackdata = callbackdata;

        ObtainSemaphore(&LSD(cl)->sema);
        LSD(cl)->kbdhidd = o;
        Update_EventHandlers(LSD(cl));
        ReleaseSemaphore(&LSD(cl)->sema);
    }

    ReturnPtr("[LinuxInput] Kbd::New", OOP_Object *, o);
}


VOID LinuxKbd__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    ObtainSemaphore(&LSD(cl)->sema);
    LSD(cl)->kbdhidd = NULL;
    Update_EventHandlers(LSD(cl));
    ReleaseSemaphore(&LSD(cl)->sema);
    
    OOP_DoSuperMethod(cl, o, msg);  
}

/***** LinuxKbd::HandleEvent()  ***************************************/

VOID LinuxKbd__Hidd_LinuxKbd__HandleEvent(OOP_Class *cl, OOP_Object *o, struct pHidd_LinuxKbd_HandleEvent *msg)
{
    struct LinuxKbd_data  *data;
    UBYTE                  scancode;
    UWORD                  hiddcode;

    EnterFunc(bug("[LinuxInput] linuxkbd_handleevent()\n"));
    
    data = OOP_INST_DATA(cl, o);
    
    scancode = msg->scanCode;
    hiddcode = scancode2hidd(scancode, LSD(cl));
    
    if (hiddcode != 0xFF)
    {  
        if (scancode >= 0x80)
            hiddcode |= IECODE_UP_PREFIX;

        data->kbd_callback(data->callbackdata, hiddcode);
    }
    
    ReturnVoid("[LinuxInput] Kbd::HandleEvent");
}


#undef LSD
#define LSD(cl) lsd

/**************** scancode2hidd() ****************/
#define DEF_TAB_SIZE 128

const UBYTE deftable[] =
{
    0xff,
    RAWKEY_ESCAPE,
    RAWKEY_1,
    RAWKEY_2,
    RAWKEY_3,
    RAWKEY_4,
    RAWKEY_5,
    RAWKEY_6,
    RAWKEY_7,
    RAWKEY_8,
    RAWKEY_9,
    RAWKEY_0,
    RAWKEY_MINUS,
    RAWKEY_EQUAL,
    RAWKEY_BACKSPACE,
    RAWKEY_TAB,
    RAWKEY_Q,
    RAWKEY_W,
    RAWKEY_E,
    RAWKEY_R,
    RAWKEY_T,
    RAWKEY_Y,
    RAWKEY_U,
    RAWKEY_I,
    RAWKEY_O,
    RAWKEY_P,
    RAWKEY_LBRACKET,
    RAWKEY_RBRACKET,
    RAWKEY_RETURN,
    RAWKEY_CONTROL,
    RAWKEY_A,
    RAWKEY_S,
    RAWKEY_D,
    RAWKEY_F,
    RAWKEY_G,
    RAWKEY_H,
    RAWKEY_J,
    RAWKEY_K,
    RAWKEY_L,
    RAWKEY_SEMICOLON,
    RAWKEY_QUOTE,
    RAWKEY_TILDE,
    RAWKEY_LSHIFT,
    RAWKEY_2B,
    RAWKEY_Z,
    RAWKEY_X,
    RAWKEY_C,
    RAWKEY_V,
    RAWKEY_B,
    RAWKEY_N,
    RAWKEY_M,
    RAWKEY_COMMA,
    RAWKEY_PERIOD,
    RAWKEY_SLASH,
    RAWKEY_RSHIFT,
    0x5C,
    RAWKEY_LALT,
    RAWKEY_SPACE,
    RAWKEY_CAPSLOCK,
    RAWKEY_F1,
    RAWKEY_F2,
    RAWKEY_F3,
    RAWKEY_F4,
    RAWKEY_F5,
    RAWKEY_F6,
    RAWKEY_F7,
    RAWKEY_F8,
    RAWKEY_F9,
    RAWKEY_F10,
    0x5A,
    0xff,
    RAWKEY_KP_7,
    RAWKEY_KP_8,
    RAWKEY_KP_9,
    0x5D,
    RAWKEY_KP_4,
    RAWKEY_KP_5,
    RAWKEY_KP_6,
    RAWKEY_KP_PLUS,
    RAWKEY_KP_1,
    RAWKEY_KP_2,
    RAWKEY_KP_3,
    RAWKEY_KP_0,
    RAWKEY_KP_DECIMAL,
    0xff,
    0xff,
    RAWKEY_LESSGREATER,
    RAWKEY_F11,
    RAWKEY_F12,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    RAWKEY_KP_ENTER,
    RAWKEY_CONTROL,
    0x5B,
    0xff,
    RAWKEY_RALT,
    RAWKEY_PAUSE,
    RAWKEY_HOME,
    RAWKEY_UP,
    RAWKEY_PAGEUP,
    RAWKEY_LEFT,
    RAWKEY_RIGHT,
    RAWKEY_END,
    RAWKEY_DOWN,
    RAWKEY_PAGEDOWN,
    RAWKEY_INSERT,
    RAWKEY_DELETE,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    RAWKEY_PAUSE,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    RAWKEY_LAMIGA,
    RAWKEY_RAMIGA,
    0xff
};
static UWORD scancode2hidd(UBYTE scancode, struct LinuxInput_staticdata *lsd)
{
    UWORD hiddcode;
    
    if ((scancode & 0x80) == 0x80)
        scancode &= ~0x80;
    
    if (havetable)
    {
        hiddcode = scancode2rawkey[scancode];
    }
    else
    {
        if (scancode >= DEF_TAB_SIZE)
            hiddcode = 0xFF;
        else
            hiddcode = deftable[scancode];
    }
    
    return hiddcode;
}

VOID HIDD_LinuxKbd_HandleEvent(OOP_Object *o, UBYTE scanCode)
{
    static OOP_MethodID                 mid;
    struct pHidd_LinuxKbd_HandleEvent     p;
    
    if (!mid)
    mid = OOP_GetMethodID(IID_Hidd_LinuxKbd, moHidd_LinuxKbd_HandleEvent);
    
    p.mID    = mid;
    p.scanCode    = scanCode;
    
    OOP_DoMethod(o, (OOP_Msg)&p);
}
