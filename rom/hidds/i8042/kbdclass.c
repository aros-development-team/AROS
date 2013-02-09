/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The main keyboard class.
    Lang: English.
*/

/****************************************************************************************/

#define AROS_ALMOST_COMPATIBLE

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/kernel.h>
#include <aros/system.h>
#include <aros/symbolsets.h>
#include <oop/oop.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <hardware/custom.h>
#include <devices/inputevent.h>
#include <devices/rawkeycodes.h>

#include "kbd.h"
#include "kbd_common.h"
#include "keys.h"

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

/* Predefinitions */

void kbd_process_key(struct kbd_data *, UBYTE,
    struct ExecBase *SysBase);

void kbd_updateleds();
int  kbd_reset(void);

/****************************************************************************************/

#define NOKEY -1

/****************************************************************************************/

#include "stdkeytable.h"

/****************************************************************************************/

#include "e0keytable.h"

/****************************************************************************************/
static void kbd_keyint(struct kbd_data *data, void *unused)
{
    UBYTE           keycode;        /* Recent Keycode get */
    UBYTE           info = 0;       /* Data from info reg */
    WORD            work = 10000;

    D(bug("ki: {\n")); 
    for(; ((info = kbd_read_status()) & KBD_STATUS_OBF) && work; work--)
    {
        /* data from information port */
        if (info & KBD_STATUS_MOUSE_OBF)
        {
            /*
            ** Data from PS/2 mouse. Hopefully this gets through to mouse interrupt
            ** if we break out of loop here :-\
            */
            break;
        }
        keycode = kbd_read_input();

        D(bug("ki: keycode %d (%x)\n", keycode, keycode));
        if (info & (KBD_STATUS_GTO | KBD_STATUS_PERR))
        {
            /* Ignore errors and messages for mouse -> eat status/error byte */
            continue;
        }

        kbd_process_key(data, keycode, SysBase);
    } /* for(; ((info = kbd_read_status()) & KBD_STATUS_OBF) && work; work--) */

    D(if (!work) bug("kbd.hidd: controller jammed (0x%02X).\n", info);)
    D(bug("ki: }\n"));
}


OOP_Object * PCKbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem *tag, *tstate;
    APTR callback = NULL;
    APTR callbackdata = NULL;
    int reset_success;
    int last_code;
    
    EnterFunc(bug("Kbd::New()\n"));

#if __WORDSIZE == 32 /* FIXME: REMOVEME: just a debugging thing for the weird s-key problem */
    SysBase->ex_Reserved2[1] = (ULONG)std_keytable;
#endif
    if (XSD(cl)->kbdhidd) /* Cannot open twice */
        ReturnPtr("Kbd::New", OOP_Object *, NULL); /* Should have some error code here */

    tstate = msg->attrList;
    D(bug("Kbd: tstate: %p, tag=%x\n", tstate, tstate->ti_Tag));

    while ((tag = NextTagItem(&tstate)))
    {
        ULONG idx;
        
        D(bug("Kbd: Got tag %d, data %x\n", tag->ti_Tag, tag->ti_Data));
            
        if (IS_HIDDKBD_ATTR(tag->ti_Tag, idx))
        {
            D(bug("Kbd hidd tag\n"));
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
        ReturnPtr("Kbd::New", OOP_Object *, NULL); /* Should have some error code here */

    /* Only continue if there appears to be a keyboard controller */
    Disable();
    last_code = kbd_clear_input();
    kbd_write_command_w(KBD_CTRLCMD_SELF_TEST);
    reset_success = kbd_wait_for_input();
    Enable();

    if (reset_success == 0x55)
    {
        /* Add some descriptional tags to our attributes */
        struct TagItem kbd_tags[] =
        {
            {aHidd_Name        , (IPTR)"ATKbd"                     },
            {aHidd_HardwareName, (IPTR)"IBM AT-compatible keyboard"},
            {TAG_MORE          , (IPTR)msg->attrList               }
        };
        struct pRoot_New new_msg =
        {
            .mID = msg->mID,
            .attrList = kbd_tags
        };

        o = (OOP_Object *)OOP_DoSuperMethod(cl, o, &new_msg.mID);
        if (o)
        {
            struct kbd_data *data = OOP_INST_DATA(cl, o);

            data->kbd_callback   = (VOID (*)(APTR, UWORD))callback;
            data->callbackdata   = callbackdata;
            data->prev_amigacode = -2;
            data->prev_keycode   = 0;

            Disable();
            kbd_reset();            /* Reset the keyboard */
            kbd_updateleds(0);
            Enable();

            /*
             * Report last key received before keyboard was reset, so that
             * keyboard.device knows about any key currently held down
             */
            if (last_code > 0)
                kbd_process_key(data, (UBYTE)last_code, SysBase);

            /* Install keyboard interrupt */
             data->irq = KrnAddIRQHandler(1, kbd_keyint, data, NULL);

             ReturnPtr("Kbd::New", OOP_Object *, o);
        } /* if (o) */
    }
    D(else bug("Keyboard controller not detected\n");)

    return NULL;
}

VOID PCKbd__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct kbd_data *data = OOP_INST_DATA(cl, o);

    KrnRemIRQHandler(data->irq);
    XSD(cl)->kbdhidd = NULL;

    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

#define WaitForInput                    \
    ({ int timeout=1000;        \
       do                               \
       {                                \
        info = kbd_read_status();       \
        if (!--timeout)                 \
          break;                        \
       } while((info & KBD_STATUS_OBF));\
       inb(0x80);  /* Read from port 0x80, the debug port, to add some delay */ \
     })

/****************************************************************************************/

#define FLAG_LCTRL      0x00000008
#define FLAG_RCTRL      0x00000010
#define FLAG_LALT       0x00000020
#define FLAG_RALT       0x00000040
#define FLAG_LSHIFT     0x00000080
#define FLAG_RSHIFT     0x00000100
#define FLAG_LMETA      0x00000200
#define FLAG_RMETA      0x00000400
#define FLAG_DEL        0x00000800

/****************************************************************************************/

void kbd_updateleds(ULONG kbd_keystate)
{
    UBYTE info;
    kbd_write_output_w(KBD_OUTCMD_SET_LEDS);
    WaitForInput;
    kbd_read_input();
    kbd_write_output_w(kbd_keystate & 0x07);
    WaitForInput;
    kbd_read_input();
}

#undef SysBase

void kbd_process_key(struct kbd_data *data, UBYTE keycode,
    struct ExecBase *SysBase)
{
    ULONG           kbd_keystate = data->kbd_keystate;
    UBYTE           downkeycode;
    UBYTE           releaseflag;
    UWORD           event;
    WORD            amigacode;

    if ((keycode == KBD_REPLY_ACK) || (keycode == KBD_REPLY_RESEND))
    {
        /* Ignore these */
        return;
    }

    if ((keycode == 0xE0) || (keycode == 0xE1))
    {
        /* Extended keycodes: E0 gets followed by one code, E1 by two */
        data->prev_keycode = keycode;
        return;
    }

    if ((keycode == 0x00) || (keycode == 0xFF))
    {
        /* 00 is error. FF is sent by some keyboards -> ignore it. */
        data->prev_keycode = 0;
        return;
    }

    amigacode = NOKEY;
    event = 0;
    downkeycode = keycode & 0x7F;
    releaseflag = keycode & 0x80;

    if (data->prev_keycode)
    {
        if (data->prev_keycode == 0xE0)
        {
            data->prev_keycode = 0;
            event = 0x4000 | keycode;       

            if (downkeycode < NUM_E0KEYS)
            {
                amigacode = e0_keytable[downkeycode];
                if (amigacode != NOKEY) amigacode |= releaseflag;
            }
        } /* if (data->prev_keycode == 0xE0) */
        else
        {
            /* Check Pause key: 0xE1 0x1D 0x45   0xE1 0x9D 0xC5 */
            if ((data->prev_keycode == 0xE1) && (downkeycode == 0x1D))
            {
                /* let's remember, that we still need third key */
                data->prev_keycode = 0x1234;
                return;
            }
            else if ((data->prev_keycode == 0x1234) && (downkeycode == 0x45))
            {
                /* Got third key and yes, it is Pause */
                amigacode = 0x6E | releaseflag;
                data->prev_keycode = 0;
            }
            else
            {
                /* Unknown */
                data->prev_keycode = 0;
                return;
            }

        } /* if (data->prev_keycode == 0xE0) else ... */

    } /* if (data->prev_keycode) */
    else
    {
        /* Normal single byte keycode */
        event = keycode;
        if (downkeycode < NUM_STDKEYS)
        {
            amigacode = std_keytable[downkeycode];
            if (amigacode != NOKEY) amigacode |= releaseflag;
        }           
    }

    switch(event)
    {
        case K_KP_Numl:
            kbd_keystate ^= 0x02;    /* Toggle Numlock bit */
            kbd_updateleds(kbd_keystate);
            break;

        case K_Scroll_Lock:
            kbd_keystate ^= 0x01;    /* Toggle Scrolllock bit */
            kbd_updateleds(kbd_keystate);
            break;

        case K_CapsLock:
            kbd_keystate ^= 0x04;    /* Toggle Capslock bit */
            kbd_updateleds(kbd_keystate);
            break;

        case K_LShift:
            kbd_keystate |= FLAG_LSHIFT;
            break;

        case (K_LShift | 0x80):
            kbd_keystate &= ~FLAG_LSHIFT;
            break;

        case K_RShift:
            kbd_keystate |= FLAG_RSHIFT;
            break;

        case (K_RShift | 0x80):
            kbd_keystate &= ~FLAG_RSHIFT;
            break;

        case K_LCtrl:
            kbd_keystate |= FLAG_LCTRL;
            break;

        case (K_LCtrl | 0x80):
            kbd_keystate &= ~FLAG_LCTRL;
            break;

        case K_RCtrl:
            kbd_keystate |= FLAG_RCTRL;
            break;

        case (K_RCtrl | 0x80):
            kbd_keystate &= ~FLAG_RCTRL;
            break;

        case K_LMeta:
            kbd_keystate |= FLAG_LMETA;
            break;

        case (K_LMeta | 0x80):
            kbd_keystate &= ~FLAG_LMETA;
            break;

        case K_RMeta:
        case K_Menu:
            kbd_keystate |= FLAG_RMETA;
            break;

        case (K_RMeta | 0x80):
        case (K_Menu | 0x80):
            kbd_keystate &= ~FLAG_RMETA;
            break;

        case K_LAlt:
            kbd_keystate |= FLAG_LALT;
            break;

        case (K_LAlt | 0x80):
            kbd_keystate &= ~FLAG_LALT;
            break;

        case K_RAlt:
            kbd_keystate |= FLAG_RALT;
            break;

        case (K_RAlt | 0x80):
            kbd_keystate &= ~FLAG_RALT;
            break;

        case K_Del:
            kbd_keystate |= FLAG_DEL;
            break;

        case (K_Del | 0x80):
            kbd_keystate &= ~FLAG_DEL;
            break;

    } /* switch(event) */

    if ((kbd_keystate & (FLAG_LCTRL | FLAG_RCTRL)) != 0
        && (kbd_keystate & (FLAG_LALT | FLAG_RALT)) != 0
        && (kbd_keystate & FLAG_DEL) != 0)
    {
        ShutdownA(SD_ACTION_COLDREBOOT);
    }

    if ((kbd_keystate & (FLAG_LCTRL | FLAG_LMETA | FLAG_RMETA))
        == (FLAG_LCTRL | FLAG_LMETA | FLAG_RMETA))
    {
        amigacode = 0x78; /* Reset */
    }

    D(bug("ki: amigacode %d (%x) last %d (%x)\n", amigacode, amigacode,
        data->prev_amigacode, data->prev_amigacode));

    /* Update keystate */
    data->kbd_keystate = kbd_keystate;

#if 0
    if (amigacode == 0x78)    // Reset request
        ColdReboot();
#endif

    if (amigacode == NOKEY) return;

    if (amigacode == data->prev_amigacode)
    {
        /*
        ** Must be a repeated key. Ignore it, because we have our
        ** own kbd repeating in input.device
        */          
        return;
    }

    data->prev_amigacode = amigacode;

    D(bug("ki: ********************* c %d (%x)\n", amigacode, amigacode));

    /* Pass the code to handler */
    data->kbd_callback(data->callbackdata, amigacode);

    return;
}

/****************************************************************************************/

/* FIXME: This should go somewhere higher but D(bug()) is not possible there */
#undef D
#undef BUG
#define D(x)
#define BUG

/****************************************************************************************/

/*
 * Please leave this routine as is for now.
 * It works and that is all that matters right now.
 */

/****************************************************************************************/

int kbd_reset(void)
{
    UBYTE status;

    kbd_write_command_w(KBD_CTRLCMD_SELF_TEST); /* Initialize and test keyboard controller */

    if (kbd_wait_for_input() != 0x55)
    {
        D(bug("Kbd: Controller test failed!\n"));
        return FALSE;
    }

    kbd_write_command_w(KBD_CTRLCMD_KBD_TEST);
    
    if (kbd_wait_for_input() != 0)
    {
        D(bug("Kbd: Keyboard test failed!\n"));
        return FALSE;
    }
    
    kbd_write_command_w(KBD_CTRLCMD_KBD_ENABLE);  /* enable keyboard */

    D(bug("Kbd: Keyboard enabled!\n"));

    do
    {
        kbd_write_output_w(KBD_OUTCMD_RESET);
        status = kbd_wait_for_input();
        
        if (status == KBD_REPLY_ACK)
            break;
            
        if (status != KBD_REPLY_RESEND)
        {
            D(bug("Kbd: Keyboard reset failed! (1)\n"));
            return FALSE;
        }
    } while(1);

    if (kbd_wait_for_input() != KBD_REPLY_POR)
    {
        D(bug("Kbd: Keyboard reset failed! (2)\n"));
        return FALSE;
    }

    do
    {
        kbd_write_output_w(KBD_OUTCMD_DISABLE);
        status = kbd_wait_for_input();
        
        if (status == KBD_REPLY_ACK)
            break;
            
        if (status != KBD_REPLY_RESEND)
        {
            D(bug("Kbd: Keyboard disable failed!\n"));
            return FALSE;
        }
    } while (1);

    kbd_write_command_w(KBD_CTRLCMD_WRITE_MODE);  /* Write mode */

#if 0
    kbd_write_output_w( KBD_MODE_KCC    | // set parameters: scan code to pc conversion, 
                            KBD_MODE_KBD_INT    | //                enable mouse and keyboard,
                     KBD_MODE_DISABLE_MOUSE | //                enable IRQ 1 & 12.
                     KBD_MODE_SYS);
#else
    kbd_write_output_w( KBD_MODE_KCC | KBD_MODE_KBD_INT);
#endif

    kbd_write_output_w(KBD_OUTCMD_ENABLE);

    D(bug("Kbd: enabled ints\n"));
    
    if (kbd_wait_for_input() != KBD_REPLY_ACK)
    {
        D(bug("Kbd: No REPLY_ACK !!!\nReturning FALSE !!!!\n"));
        return FALSE;
    }
    
    D(bug("Kbd: Successfully reset keyboard!\n"));

    return TRUE;
}

/****************************************************************************************/
