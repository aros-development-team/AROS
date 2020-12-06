/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The main keyboard class.
    Lang: English.
*/

/****************************************************************************************/

#include <aros/debug.h>

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

#include "i8042_kbd.h"
#include "i8042_common.h"
#include "keys.h"

/****************************************************************************************/

/* Predefinitions */

static void kbd_irq_process_key(struct kbd_data *, UBYTE, struct ExecBase *SysBase);
//void kbd_updateleds();
static int  kbd_reset(struct kbd_data *data);

/****************************************************************************************/

#define DFAIL(x)        x
#define KEYBOARDIRQ     1
#define NOKEY           -1

/****************************************************************************************/

#include "stdkeytable.h"

/****************************************************************************************/

#include "e0keytable.h"

/****************************************************************************************
 * Keyboard Interrupt handler
 * NB: Do NOT use any functions that take the timer IORequest as input
 * in this code or from any functions it may call!.
 ****************************************************************************************/
static void Keyboard_IntHandler(struct kbd_data *data, void *unused)
{
    UBYTE           keycode;        /* Recent Keycode get */
    UBYTE           info = 0;       /* Data from info reg */
    WORD            work = 10000;

    D(
        bug("[i8042:Kbd] %s()\n", __func__);
        bug("[i8042:Kbd] %s: ki - {\n", __func__); 
    )
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

        D(bug("[i8042:Kbd] %s: ki - keycode %d (%x)\n", __func__, keycode, keycode));
        if (info & (KBD_STATUS_GTO | KBD_STATUS_PERR))
        {
            /* Ignore errors and messages for mouse -> eat status/error byte */
            continue;
        }

        kbd_irq_process_key(data, keycode, SysBase);
    } /* for(; ((info = kbd_read_status()) & KBD_STATUS_OBF) && work; work--) */

    D(
        if (!work)
            bug("[i8042:Kbd] %s: controller jammed (0x%02X).\n", __func__, info);
        bug("[i8042:Kbd] %s: ki - }\n", __func__);
    )
}

/****************************************************************************************
 * Keyboard Controller Task
 * It is safe to use functions that take the timer IORequest as input
 * from here on ....
 * initializes the controller/keyboard then waits for signals to
 * update the LED's.
 ****************************************************************************************/
static void kbdUpdateLEDs(struct kbd_data *data)
{
    kbd_write_output_w(data->ioTimer, KBD_OUTCMD_SET_LEDS);
    kbd_wait_for_input(data->ioTimer);
    kbd_read_input();
    kbd_write_output_w(data->ioTimer, data->kbd_ledstate & 0x07);
    kbd_wait_for_input(data->ioTimer);
    kbd_read_input();
}

void KbdCntrlTask(OOP_Class *cl, OOP_Object *o)
{
    struct kbd_data *data = OOP_INST_DATA(cl, o);
    struct MsgPort *p = CreateMsgPort();
    int reset_success;
    int last_code;
    ULONG sig;

    D(bug("[i8042:Kbd] Task starting..\n"));

    if (!p)
    {
        D(bug("[i8042:Kbd] Failed to create Timer MsgPort..\n"));
        data->LEDSigBit = (ULONG)-1;
        Signal(data->CtrlTask->tc_UserData, SIGF_SINGLE);
        return;
    }

    data->ioTimer = CreateIORequest(p, sizeof(struct timerequest));
    if (!data->ioTimer)
    {
        D(bug("[i8042:Kbd] Failed to create Timer MsgPort..\n"));
	DeleteMsgPort(p);
        data->LEDSigBit = (ULONG)-1;
        Signal(data->CtrlTask->tc_UserData, SIGF_SINGLE);
        return;
    }

    if (0 != OpenDevice("timer.device", UNIT_MICROHZ, data->ioTimer, 0))	
    {
        D(bug("[i8042:Kbd] Failed to open timer.device, unit MICROHZ\n");)
        DeleteIORequest(data->ioTimer);
        data->ioTimer = NULL;
	DeleteMsgPort(p);
        data->LEDSigBit = (ULONG)-1;
        Signal(data->CtrlTask->tc_UserData, SIGF_SINGLE);
        return;
    }

    /* Get the signal used for updating leds */
    data->CtrlTask = FindTask(0);
    data->LEDSigBit = AllocSignal(-1);
    /* Failed to get it? Use SIGBREAKB_CTRL_E instead */

    if (data->LEDSigBit < 0)
        data->LEDSigBit = SIGBREAKB_CTRL_E;

    sig = 1L << data->LEDSigBit;

    /* Only continue if there appears to be a keyboard controller */
    Disable();
    last_code = kbd_clear_input();
    kbd_write_command_w(data->ioTimer, KBD_CTRLCMD_SELF_TEST);
    reset_success = kbd_wait_for_input(data->ioTimer);
    Enable();

    if (reset_success != 0x55)
    {
        struct IORequest *io = data->ioTimer;
        data->ioTimer = NULL;

        /* Signal the launching task we are done .. */
        FreeSignal(data->LEDSigBit);
        data->LEDSigBit = (ULONG)-1;
        Signal(data->CtrlTask->tc_UserData, SIGF_SINGLE);
        DeleteIORequest(io);
	DeleteMsgPort(p);
        return;
    }

    Disable();
    D(bug("[i8042:Kbd] %s: performing keyboard reset ...\n", __func__));
    D(UBYTE status = )kbd_reset(data);            /* Reset the keyboard */
    D(
        bug("[i8042:Kbd] %s: reset returned %x\n", __func__, status);
        bug("[i8042:Kbd] %s: updating leds ...\n", __func__);
    )
    Enable();
    kbdUpdateLEDs(data);

    D(bug("[i8042:Kbd] %s: ready\n", __func__));

    /*
     * Report last key received before keyboard was reset, so that
     * keyboard.device knows about any key currently held down
     */
    if (last_code > 0)
        kbd_irq_process_key(data, (UBYTE)last_code, SysBase);

    D(bug("[i8042:Kbd] %s: registering handler for IRQ %u\n", __func__, KEYBOARDIRQ));

    /* Install keyboard interrupt */
     data->irq = KrnAddIRQHandler(KEYBOARDIRQ, Keyboard_IntHandler, data, NULL);

    D(bug("[i8042:Kbd] Controller setup finished\n"));

    /* Signal the launching task we are done .. */
    Signal(data->CtrlTask->tc_UserData, SIGF_SINGLE);

    /* Wait forever and process signals */
    for (;;)
    {
        ULONG signals = Wait(sig);

        if (signals & data->LEDSigBit)
        {
            D(bug("[i8042:Kbd] %s: updating led's\n", __func__));
            kbdUpdateLEDs(data);
        }
    }
}

OOP_Object * i8042Kbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    OOP_Object *kbd = NULL;
    struct TagItem *tag, *tstate;
    APTR callback = NULL;
    APTR callbackdata = NULL;

    D(bug("[i8042:Kbd] %s()\n", __func__));

#if __WORDSIZE == 32 /* FIXME: REMOVEME: just a debugging thing for the weird s-key problem */
    SysBase->ex_Reserved2[1] = (ULONG)std_keytable;
#endif
    if (XSD(cl)->kbdhidd)
    {
        /* Cannot open twice */
        D(bug("[i8042:Kbd] %s: already instantiated!\n", __func__));
        return NULL;
    }

    tstate = msg->attrList;
    D(bug("[i8042:Kbd] %s: tstate: %p, tag=%x\n", __func__, tstate, tstate->ti_Tag));

    while ((tag = NextTagItem(&tstate)))
    {
        ULONG idx;
        
        D(bug("[i8042:Kbd] %s: Got tag %d, data %x\n", __func__, tag->ti_Tag, tag->ti_Data));
            
        if (IS_HIDDKBD_ATTR(tag->ti_Tag, idx))
        {
            D(bug("[i8042:Kbd] %s:   Kbd hidd tag\n", __func__));
            switch (idx)
            {
                case aoHidd_Kbd_IrqHandler:
                    callback = (APTR)tag->ti_Data;
                    D(bug("[i8042:Kbd] %s:     Got callback %p\n", __func__, (APTR)tag->ti_Data));
                    break;
                        
                case aoHidd_Kbd_IrqHandlerData:
                    callbackdata = (APTR)tag->ti_Data;
                    D(bug("[i8042:Kbd] %s:     Got data %p\n", __func__, (APTR)tag->ti_Data));
                    break;
            }
        }
            
    } /* while (tags to process) */
    
    if (NULL == callback)
        ReturnPtr("Kbd::New", OOP_Object *, NULL); /* Should have some error code here */

    D(bug("[i8042:Kbd] %s: checking for controller ...\n", __func__));

    /* Add some descriptional tags to our attributes */
    struct TagItem kbd_tags[] =
    {
        {aHidd_Name        , (IPTR)"i8042.hidd"                     },
        {aHidd_HardwareName, (IPTR)"IBM AT-compatible keyboard"},
        {TAG_MORE          , (IPTR)msg->attrList               }
    };
    struct pRoot_New new_msg =
    {
        .mID = msg->mID,
        .attrList = kbd_tags
    };

    D(bug("[i8042:Kbd] %s: found!\n", __func__));

    kbd = (OOP_Object *)OOP_DoSuperMethod(cl, o, &new_msg.mID);
    if (kbd)
    {
        struct kbd_data *data = OOP_INST_DATA(cl, kbd);
        UBYTE status;

        D(bug("[i8042:Kbd] %s: controller obj @ 0x%p, data @ 0x%p\n", __func__, kbd, data));

        data->kbd_callback   = (VOID (*)(APTR, UWORD))callback;
        data->callbackdata   = callbackdata;
        data->prev_amigacode = -2;
        data->prev_keycode   = 0;

        NewCreateTask(TASKTAG_PC,           KbdCntrlTask,
                         TASKTAG_NAME,      "i8042 Keyboard controller",
                         TASKTAG_STACKSIZE, 1024,
                         TASKTAG_PRI,       100,
                         TASKTAG_ARG1,      cl,
                         TASKTAG_ARG2,      kbd,
                         TASKTAG_USERDATA,  FindTask(NULL),
                         TAG_DONE);
        Wait(SIGF_SINGLE);
        if (data->LEDSigBit == (ULONG)-1)
        {
            D(bug("[i8042:Kbd] %s: controller initialization failed\n", __func__));
            OOP_MethodID disp_mid = msg->mID - moRoot_New + moRoot_Dispose;
            OOP_DoSuperMethod(cl, kbd, &disp_mid);
            kbd = NULL;
        }
    } /* if (kbd) */
    D(else bug("[i8042:Kbd] %s: Keyboard controller not detected\n", __func__);)

    XSD(cl)->kbdhidd = kbd;
    D(bug("[i8042:Kbd] %s: returning 0x%p\n", __func__, kbd);)

    return kbd;
}

VOID i8042Kbd__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct kbd_data *data = OOP_INST_DATA(cl, o);

    D(bug("[i8042:Kbd] %s()\n", __func__));

    KrnRemIRQHandler(data->irq);
    XSD(cl)->kbdhidd = NULL;

    OOP_DoSuperMethod(cl, o, msg);
}

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

#undef SysBase

static void kbd_irq_process_key(struct kbd_data *data, UBYTE keycode, struct ExecBase *SysBase)
{
    ULONG           kbd_keystate = data->kbd_keystate;
    UBYTE           downkeycode;
    UBYTE           releaseflag;
    UWORD           event;
    WORD            amigacode;

    D(bug("[i8042:Kbd] %s()\n", __func__));

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
            data->kbd_ledstate = kbd_keystate;
            Signal(data->CtrlTask, data->LEDSigBit);
            //kbd_updateleds(kbd_keystate);
            break;

        case K_Scroll_Lock:
            kbd_keystate ^= 0x01;    /* Toggle Scrolllock bit */
            data->kbd_ledstate = kbd_keystate;
            Signal(data->CtrlTask, data->LEDSigBit);
            //kbd_updateleds(kbd_keystate);
            break;

        case K_CapsLock:
            kbd_keystate ^= 0x04;    /* Toggle Capslock bit */
            data->kbd_ledstate = kbd_keystate;
            Signal(data->CtrlTask, data->LEDSigBit);
            //kbd_updateleds(kbd_keystate);
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

    D(bug("[i8042:Kbd] %s: ki - amigacode %d (%x) last %d (%x)\n", __func__, amigacode, amigacode,
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

    D(bug("[i8042:Kbd] %s: ki - ********************* c %d (%x)\n", __func__, amigacode, amigacode));

    /* Pass the code to handler */
    data->kbd_callback(data->callbackdata, amigacode);

    return;
}

/****************************************************************************************/

/*
 * Please leave this routine as is for now.
 * It works and that is all that matters right now.
 */

/****************************************************************************************/

static int kbd_reset(struct kbd_data *data)
{
    UBYTE status;

    D(
        bug("[i8042:Kbd] %s()\n", __func__);
        bug("[i8042:Kbd] %s: sending self test...\n", __func__);
    )
    kbd_write_command_w(data->ioTimer, KBD_CTRLCMD_SELF_TEST); /* Initialize and test keyboard controller */
    D(bug("[i8042:Kbd] %s:     done!, waiting for completion ..\n", __func__));
    status = kbd_wait_for_input(data->ioTimer);
    if (status != 0x55)
    {
        DFAIL(bug("[i8042:Kbd] %s: Controller test failed! (%x)\n", __func__, status));
        return FALSE;
    }

    D(bug("[i8042:Kbd] %s: sending kbd test...\n", __func__));
    kbd_write_command_w(data->ioTimer, KBD_CTRLCMD_KBD_TEST);
    D(bug("[i8042:Kbd] %s:     done!, waiting for completion ..\n", __func__));
    status = kbd_wait_for_input(data->ioTimer);
    if (status != 0)
    {
        DFAIL(bug("[i8042:Kbd] %s: Keyboard test failed! (%x)\n", __func__, status));
        return FALSE;
    }

    D(bug("[i8042:Kbd] %s: sending enable...\n", __func__));
    kbd_write_command_w(data->ioTimer, KBD_CTRLCMD_KBD_ENABLE);  /* enable keyboard */
    D(
        bug("[i8042:Kbd] %s:     done!\n", __func__);
        bug("[i8042:Kbd] %s: starting reset cycle ..\n", __func__);
    )
    do
    {
        D(bug("[i8042:Kbd] %s: sending reset...\n", __func__));
        kbd_write_output_w(data->ioTimer, KBD_OUTCMD_RESET);

        D(bug("[i8042:Kbd] %s:     done!, waiting for completion ..\n", __func__));
        status = kbd_wait_for_input(data->ioTimer);
        if (status == KBD_REPLY_ACK)
            break;

        if (status != KBD_REPLY_RESEND)
        {
            DFAIL(bug("[i8042:Kbd] %s: Keyboard reset failed! (%x)\n", __func__, status));
            return FALSE;
        }
    } while(1);


    D(bug("[i8042:Kbd] %s: waiting for power-on-reset...\n", __func__);)
    status = kbd_wait_for_input(data->ioTimer);
    if (status != KBD_REPLY_POR)
    {
        DFAIL(bug("[i8042:Kbd] %s: Keyboard power-on-reset failed! (%x)\n", __func__, status));
        return FALSE;
    }

    D(bug("[i8042:Kbd] %s: starting disable cycle ..\n", __func__);)
    do
    {
        D(bug("[i8042:Kbd] %s: sending disable...\n", __func__));
        kbd_write_output_w(data->ioTimer, KBD_OUTCMD_DISABLE);
        D(bug("[i8042:Kbd] %s:     done!, waiting for completion ..\n", __func__));
        status = kbd_wait_for_input(data->ioTimer);
        if (status == KBD_REPLY_ACK)
            break;

        if (status != KBD_REPLY_RESEND)
        {
            DFAIL(bug("[i8042:Kbd] %s: Keyboard disable failed! (%x)\n", __func__, status));
            return FALSE;
        }
    } while (1);

    D(bug("[i8042:Kbd] %s: sending write mode...\n", __func__);)
    kbd_write_command_w(data->ioTimer, KBD_CTRLCMD_WRITE_MODE);  /* Write mode */

#if 0
    kbd_write_output_w(data->ioTimer,  KBD_MODE_KCC    | // set parameters: scan code to pc conversion, 
                            KBD_MODE_KBD_INT    | //                enable mouse and keyboard,
                     KBD_MODE_DISABLE_MOUSE | //                enable IRQ 1 & 12.
                     KBD_MODE_SYS);
#else
    kbd_write_output_w(data->ioTimer,  KBD_MODE_KCC | KBD_MODE_KBD_INT);
#endif

    D(bug("[i8042:Kbd] %s: sending enable...\n", __func__));
    kbd_write_output_w(data->ioTimer, KBD_OUTCMD_ENABLE);

    D(bug("[i8042:Kbd] %s: enabled ints\n", __func__));

    status = kbd_wait_for_input(data->ioTimer);
    if (status != KBD_REPLY_ACK)
    {
        DFAIL(
            bug("[i8042:Kbd] %s: No REPLY_ACK (%x) !!!\n", __func__, status);
            bug("[i8042:Kbd] %s: Returning FALSE !!!!\n", __func__);
        )
        return FALSE;
    }

    D(bug("[i8042:Kbd] %s: Successfully reset keyboard!\n", __func__));

    return TRUE;
}

/****************************************************************************************/
