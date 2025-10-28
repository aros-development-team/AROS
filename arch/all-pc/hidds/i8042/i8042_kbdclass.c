/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc: The main keyboard class.
*/

/****************************************************************************************/

#ifndef DEBUG
#define DEBUG 0
#endif
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
#include <hidd/input.h>
#include <hidd/keyboard.h>
#include <hardware/custom.h>
#include <devices/inputevent.h>
#include <devices/rawkeycodes.h>

#include "i8042_kbd.h"
#include "i8042_common.h"
#include "keys.h"

#include LC_LIBDEFS_FILE

/****************************************************************************************/

/* Predefinitions */

static void i8042_kbd_process_irq_key(struct kbd_data *, UBYTE, struct ExecBase *);
static int  i8042_kbd_reset(struct i8042base *, struct kbd_data *);

extern const char GM_UNIQUENAME(LibName)[];
static const char *i8042hwname = "IBM AT-compatible keyboard";
static const char *i8042ctname = "i8042 Keyboard controller";

/****************************************************************************************/

#define DINT(x)
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
static void i8042_kbd_irq_handler(struct kbd_data *data, void *unused)
{
    UBYTE           keycode;        /* Recent Keycode get */
    UBYTE           info = 0;       /* Data from info reg */
    WORD            work = 10000;

    DINT(
        bug("[i8042:Kbd] %s()\n", __func__);
        bug("[i8042:Kbd] %s: ki - {\n", __func__);
    )
    for(; ((info = i8042_read_status_port()) & KBD_STATUS_OBF) && work; work--) {
        /* data from information port */
        if (info & KBD_STATUS_MOUSE_OBF) {
            /*
            ** Data from PS/2 mouse. Hopefully this gets through to mouse interrupt
            ** if we break out of loop here :-\
            */
            break;
        }
        keycode = i8042_read_data_port();

        DINT(bug("[i8042:Kbd] %s:   ki - keycode %d (%x)\n", __func__, keycode, keycode));
        if (info & (KBD_STATUS_GTO | KBD_STATUS_PERR)) {
            /* Ignore errors and messages for mouse -> eat status/error byte */
            continue;
        }

        i8042_kbd_process_irq_key(data, keycode, SysBase);
    } /* for(; ((info = i8042_read_status_port()) & KBD_STATUS_OBF) && work; work--) */

    DINT(
        if (!work)
        bug("[i8042:Kbd] %s:   controller jammed (0x%02X).\n", __func__, info);
        bug("[i8042:Kbd] %s: ki - }\n", __func__);
    )
}

static void i8042_kbd_flush_output_buffer(void)
{
    UBYTE info;
    D(int flush_count = 0;)
    int work = 1000;  /* safety loop */

    while ((info = i8042_read_status_port()) & KBD_STATUS_OBF && work--) {
        (void)i8042_read_data_port();   /* discard data */
        D(flush_count++;)
    }

    D(
        if (flush_count > 0)
        bug("[i8042:Kbd] Flushed %d leftover bytes from controller\n", flush_count);
    )
}

/****************************************************************************************
 * Keyboard Controller Task
 * It is safe to use functions that take the timer IORequest as input
 * from here on ....
 * initializes the controller/keyboard then waits for signals to
 * update the LED's.
 ****************************************************************************************/
static void i8042_kbd_update_leds(struct kbd_data *data)
{
    i8042_write_data_with_wait(data->hwdata.ioTimer, KBD_OUTCMD_SET_LEDS);
    i8042_wait_for_input(data->hwdata.ioTimer);
    i8042_read_data_port();
    i8042_write_data_with_wait(data->hwdata.ioTimer, data->kbd_ledstate & 0x07);
    i8042_wait_for_input(data->hwdata.ioTimer);
    i8042_read_data_port();
}

void i8042_kbd_controller_task(OOP_Class *cl, OOP_Object *o)
{
    struct kbd_data *data = OOP_INST_DATA(cl, o);
    struct MsgPort *p = CreateMsgPort();
    int reset_success;
    int last_code;
    ULONG sig;

    D(bug("[i8042:Kbd] Task starting..\n"));

    if (!p) {
        D(bug("[i8042:Kbd] Failed to create Timer MsgPort..\n"));
        data->LEDSigBit = (ULONG)-1;
        Signal(data->CtrlTask->tc_UserData, SIGF_SINGLE);
        return;
    }

    data->hwdata.ioTimer = CreateIORequest(p, sizeof(struct timerequest));
    if (!data->hwdata.ioTimer) {
        D(bug("[i8042:Kbd] Failed to create Timer MsgPort..\n"));
        DeleteMsgPort(p);
        data->LEDSigBit = (ULONG)-1;
        Signal(data->CtrlTask->tc_UserData, SIGF_SINGLE);
        return;
    }

    if (0 != OpenDevice("timer.device", UNIT_MICROHZ, data->hwdata.ioTimer, 0)) {
        D(bug("[i8042:Kbd] Failed to open timer.device, unit MICROHZ\n");)
        DeleteIORequest(data->hwdata.ioTimer);
        data->hwdata.ioTimer = NULL;
        DeleteMsgPort(p);
        data->LEDSigBit = (ULONG)-1;
        Signal(data->CtrlTask->tc_UserData, SIGF_SINGLE);
        return;
    }

    /* Get the signal used for updating leds */
    data->CtrlTask = FindTask(0);
    data->LEDSigBit = AllocSignal(-1);
    /* Failed to get it? Use SIGBREAKB_CTRL_E instead */

    if (data->LEDSigBit == (ULONG)-1)
        data->LEDSigBit = SIGBREAKB_CTRL_E;

    sig = 1L << data->LEDSigBit;

    /* Only continue if there appears to be a keyboard controller */
    Disable();
    last_code = i8042_kbd_clear_input();
    i8042_write_command_with_wait(data->hwdata.ioTimer, KBD_CTRLCMD_SELF_TEST);
    reset_success = i8042_wait_for_input(data->hwdata.ioTimer);
    Enable();

    if (reset_success != 0x55) {
        struct IORequest *io = data->hwdata.ioTimer;
        data->hwdata.ioTimer = NULL;

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
    D(UBYTE status = )i8042_kbd_reset((struct i8042base *)cl->UserData, data);            /* Reset the keyboard */
    D(
        bug("[i8042:Kbd] %s: reset returned %x\n", __func__, status);
        bug("[i8042:Kbd] %s: updating leds ...\n", __func__);
    )
    Enable();
    i8042_kbd_update_leds(data);

    D(bug("[i8042:Kbd] %s: init sequence complete <reset OK, LED sigbit %lu>\n",
          __func__, data->LEDSigBit));

    /* Flush any stray data in i8042 output buffer before enabling IRQ */
    i8042_kbd_flush_output_buffer();

    /*
     * Report last key received before keyboard was reset, so that
     * keyboard.device knows about any key currently held down
     */
    if (last_code > 0)
        i8042_kbd_process_irq_key(data, (UBYTE)last_code, SysBase);

    D(bug("[i8042:Kbd] %s: registering handler for IRQ %u\n", __func__, KEYBOARDIRQ));

    /* Install keyboard interrupt */
    data->irq = KrnAddIRQHandler(KEYBOARDIRQ, i8042_kbd_irq_handler, data, NULL);

    D(bug("[i8042:Kbd] Controller setup finished\n"));

    /* Signal the launching task we are done .. */
    Signal(data->CtrlTask->tc_UserData, SIGF_SINGLE);

    /* Wait forever and process signals */
    for (;;) {
        ULONG signals = Wait(sig);

        if (signals & data->LEDSigBit) {
            D(bug("[i8042:Kbd] %s: updating led's\n", __func__));
            i8042_kbd_update_leds(data);
        }
    }
}

OOP_Object * i8042Kbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    OOP_Object *kbd = NULL;
    struct TagItem *tag, *tstate;

    D(bug("[i8042:Kbd] %s()\n", __func__));

#if __WORDSIZE == 32 /* FIXME: REMOVEME: just a debugging thing for the weird s-key problem */
    SysBase->ex_Reserved2[1] = (ULONG)std_keytable;
#endif
    if (XSD(cl)->kbdhidd) {
        /* Cannot open twice */
        D(bug("[i8042:Kbd] %s: HW driver already instantiated!\n", __func__));
        return NULL;
    }

    D(bug("[i8042:Kbd] %s: creating input hw instance...\n", __func__));

    /* Add some descriptional tags to our attributes */
    struct TagItem kbd_tags[] = {
        {aHidd_Name        , (IPTR)GM_UNIQUENAME(LibName)   },
        {aHidd_HardwareName, (IPTR)i8042hwname              },
        {TAG_MORE          , (IPTR)msg->attrList            }
    };
    struct pRoot_New new_msg = {
        .mID = msg->mID,
        .attrList = kbd_tags
    };

    kbd = (OOP_Object *)OOP_DoSuperMethod(cl, o, &new_msg.mID);
    if (kbd) {
        struct kbd_data *data = OOP_INST_DATA(cl, kbd);
        UBYTE status;

        D(
            bug("[i8042:Kbd] %s: input hw obj @ 0x%p\n", __func__, kbd);
            bug("[i8042:Kbd] %s:         data @ 0x%p\n", __func__, data);
        )

        data->hwdata.base = (struct i8042base *)cl->UserData;
        data->hwdata.self = kbd;

        data->prev_amigacode = -2;
        data->prev_keycode   = 0;

        NewCreateTask(TASKTAG_PC,        i8042_kbd_controller_task,
                      TASKTAG_NAME,      (IPTR)i8042ctname,
                      TASKTAG_STACKSIZE, 1024,
                      TASKTAG_PRI,       100,
                      TASKTAG_ARG1,      cl,
                      TASKTAG_ARG2,      kbd,
                      TASKTAG_USERDATA,  FindTask(NULL),
                      TAG_DONE);
        Wait(SIGF_SINGLE);
        if (data->LEDSigBit == (ULONG)-1) {
            D(bug("[i8042:Kbd] %s: controller initialization failed\n", __func__));
            OOP_MethodID disp_mid = msg->mID - moRoot_New + moRoot_Dispose;
            OOP_DoSuperMethod(cl, kbd, &disp_mid);
            kbd = NULL;
        }
    } /* if (kbd) */
    D(else bug("[i8042:Kbd] %s: input hw instantiation failed\n", __func__);)

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

static void i8042_kbd_process_irq_key(struct kbd_data *data, UBYTE keycode, struct ExecBase *SysBase)
{
    struct pHidd_Kbd_Event  kevt;
    ULONG           kbd_keystate = data->kbd_keystate;
    UBYTE           downkeycode;
    UBYTE           releaseflag;
    UWORD           event;

    D(bug("[i8042:Kbd] %s()\n", __func__));

    if ((keycode == KBD_REPLY_ACK) || (keycode == KBD_REPLY_RESEND)) {
        /* Ignore these */
        return;
    }

    if ((keycode == 0xE0) || (keycode == 0xE1)) {
        /* Extended keycodes: E0 gets followed by one code, E1 by two */
        data->prev_keycode = keycode;
        return;
    }

    if ((keycode == 0x00) || (keycode == 0xFF)) {
        /* 00 is error. FF is sent by some keyboards -> ignore it. */
        data->prev_keycode = 0;
        return;
    }

    kevt.flags = 0;
    kevt.code = NOKEY;
    event = 0;
    downkeycode = keycode & 0x7F;
    releaseflag = keycode & 0x80;

    if (data->prev_keycode) {
        if (data->prev_keycode == 0xE0) {
            data->prev_keycode = 0;
            event = 0x4000 | keycode;

            if (downkeycode < NUM_E0KEYS) {
                kevt.code = e0_keytable[downkeycode];
                if (kevt.code != NOKEY) kevt.code |= releaseflag;
            }
        } /* if (data->prev_keycode == 0xE0) */
        else {
            /* Check Pause key: 0xE1 0x1D 0x45   0xE1 0x9D 0xC5 */
            if ((data->prev_keycode == 0xE1) && (downkeycode == 0x1D)) {
                /* let's remember, that we still need third key */
                data->prev_keycode = 0x1234;
                return;
            } else if ((data->prev_keycode == 0x1234) && (downkeycode == 0x45)) {
                /* Got third key and yes, it is Pause */
                kevt.code = 0x6E | releaseflag;
                data->prev_keycode = 0;
            } else {
                /* Unknown */
                data->prev_keycode = 0;
                return;
            }

        } /* if (data->prev_keycode == 0xE0) else ... */

    } /* if (data->prev_keycode) */
    else {
        /* Normal single byte keycode */
        event = keycode;
        if (downkeycode < NUM_STDKEYS) {
            kevt.code = std_keytable[downkeycode];
            if (kevt.code != NOKEY) kevt.code |= releaseflag;
        }
    }

    switch(event) {
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
            && (kbd_keystate & FLAG_DEL) != 0) {
        ShutdownA(SD_ACTION_COLDREBOOT);
    }

    if ((kbd_keystate & (FLAG_LCTRL | FLAG_LMETA | FLAG_RMETA))
            == (FLAG_LCTRL | FLAG_LMETA | FLAG_RMETA)) {
        kevt.code = 0x78; /* Reset */
    }

    D(bug("[i8042:Kbd] %s: ki - keycode %d (%x) last %d (%x)\n", __func__, kevt.code, kevt.code,
          data->prev_amigacode, data->prev_amigacode));

    /* Update keystate */
    data->kbd_keystate = kbd_keystate;

#if 0
    if (kevt.code == 0x78)    // Reset request
        ColdReboot();
#endif

    if (kevt.code == NOKEY)
        return;

    if (kevt.code == data->prev_amigacode) {
        /*
        ** Must be a repeated key. Ignore it, because we have our
        ** own kbd repeating in input.device
        */
        return;
    }

    data->prev_amigacode = kevt.code;

    D(bug("[i8042:Kbd] %s: ki - ********************* c %d (%x)\n", __func__, kevt.code, kevt.code));

    /* Broadcast the input event ..*/
    OOP_Object *kbdhw = data->hwdata.base->csd.kbdhw;
    D(bug("[i8042:Kbd] %s: kbdhw @ 0x%p, driver @ 0x%p\n", __func__, kbdhw, data->hwdata.self));

    KBDPUSHEVENT(OOP_OCLASS(data->hwdata.self), kbdhw, data->hwdata.self, &kevt);

    return;
}

/****************************************************************************************/

/*
 * Please leave this routine as is for now.
 * It works and that is all that matters right now.
 */

/****************************************************************************************/

static int i8042_kbd_reset(struct i8042base *i8042Base, struct kbd_data *data)
{
    UBYTE status;

    D(
        bug("[i8042:Kbd] %s()\n", __func__);
        bug("[i8042:Kbd] %s: sending self test...\n", __func__);
    )
    i8042_write_command_with_wait(data->hwdata.ioTimer, KBD_CTRLCMD_SELF_TEST); /* Initialize and test keyboard controller */
    D(bug("[i8042:Kbd] %s:     done!, waiting for completion ..\n", __func__));
    status = i8042_wait_for_input(data->hwdata.ioTimer);
    if (status != 0x55) {
        DFAIL(bug("[i8042:Kbd] %s: Controller test failed! (%x)\n", __func__, status));
        return FALSE;
    }

    D(bug("[i8042:Kbd] %s: sending kbd test...\n", __func__));
    i8042_write_command_with_wait(data->hwdata.ioTimer, KBD_CTRLCMD_KBD_TEST);
    D(bug("[i8042:Kbd] %s:     done!, waiting for completion ..\n", __func__));
    status = i8042_wait_for_input(data->hwdata.ioTimer);
    if (status != 0) {
        DFAIL(bug("[i8042:Kbd] %s: Keyboard test failed! (%x)\n", __func__, status));
        return FALSE;
    }

    D(bug("[i8042:Kbd] %s: sending enable...\n", __func__));
    i8042_write_command_with_wait(data->hwdata.ioTimer, KBD_CTRLCMD_KBD_ENABLE);  /* enable keyboard */
    D(
        bug("[i8042:Kbd] %s:     done!\n", __func__);
        bug("[i8042:Kbd] %s: starting reset cycle ..\n", __func__);
    )
    do {
        D(bug("[i8042:Kbd] %s: sending reset...\n", __func__));
        i8042_write_data_with_wait(data->hwdata.ioTimer, KBD_OUTCMD_RESET);

        D(bug("[i8042:Kbd] %s:     done!, waiting for completion ..\n", __func__));
        status = i8042_wait_for_input(data->hwdata.ioTimer);
        if (status == KBD_REPLY_ACK)
            break;

        if (status != KBD_REPLY_RESEND) {
            DFAIL(bug("[i8042:Kbd] %s: Keyboard reset failed! (%x)\n", __func__, status));
            return FALSE;
        }
    } while(1);


    D(bug("[i8042:Kbd] %s: waiting for power-on-reset...\n", __func__);)
    status = i8042_wait_for_input_with_timeout(data->hwdata.ioTimer, 500);
    if (status != KBD_REPLY_POR) {
        DFAIL(bug("[i8042:Kbd] %s: Keyboard power-on-reset failed! (%x)\n", __func__, status));
        return FALSE;
    }

    D(bug("[i8042:Kbd] %s: starting disable cycle ..\n", __func__);)
    do {
        D(bug("[i8042:Kbd] %s: sending disable...\n", __func__));
        i8042_write_data_with_wait(data->hwdata.ioTimer, KBD_OUTCMD_DISABLE);
        D(bug("[i8042:Kbd] %s:     done!, waiting for completion ..\n", __func__));
        status = i8042_wait_for_input(data->hwdata.ioTimer);
        if (status == KBD_REPLY_ACK)
            break;

        if (status != KBD_REPLY_RESEND) {
            DFAIL(bug("[i8042:Kbd] %s: Keyboard disable failed! (%x)\n", __func__, status));
            return FALSE;
        }
    } while (1);

    D(bug("[i8042:Kbd] %s: sending write mode...\n", __func__);)
    i8042_write_command_with_wait(data->hwdata.ioTimer, KBD_CTRLCMD_WRITE_MODE);  /* Write mode */

    i8042Base->csd.cs_intbits |= KBD_MODE_KBD_INT;
    i8042_write_data_with_wait(data->hwdata.ioTimer,  (KBD_MODE_KCC | KBD_MODE_SYS) | i8042Base->csd.cs_intbits);

    D(bug("[i8042:Kbd] %s: sending enable...\n", __func__));
    i8042_write_data_with_wait(data->hwdata.ioTimer, KBD_OUTCMD_ENABLE);

    D(bug("[i8042:Kbd] %s: enabled ints\n", __func__));

    status = i8042_wait_for_input(data->hwdata.ioTimer);
    if (status != KBD_REPLY_ACK) {
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
