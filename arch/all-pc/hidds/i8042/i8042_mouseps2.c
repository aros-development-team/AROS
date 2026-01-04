/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: PS/2 mouse driver.
*/

#ifndef DEBUG
#define DEBUG 0
#endif
#include <aros/debug.h>

#define DRESET(x)

/****************************************************************************************/

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <oop/oop.h>
#include <hidd/hidd.h>
#include <hidd/mouse.h>
#include <devices/inputevent.h>

#include "i8042_mouse.h"
#include "i8042_common.h"

/****************************************************************************************/

#define PS2MOUSEIRQ     12

/* defines for buttonstate */

#define LEFT_BUTTON     1
#define RIGHT_BUTTON    2
#define MIDDLE_BUTTON   4

/****************************************************************************************/

int i8042_mouse_reset(struct i8042base *, struct mouse_data *);

/****************************************************************************************
 * PS/2 Mouse Interrupt handler
 * NB: Do NOT use any functions that take the timer IORequest as input
 * in this code or from any functions it may call!.
 ****************************************************************************************/
static void i8042_mouse_irq_handler(struct mouse_data *data, void *unused)
{
    struct pHidd_Mouse_Event    *e = &data->event;
    OOP_Object                  *mousehw = data->hwdata.base->csd.mousehw;
    UWORD                       buttonstate;
    WORD                        work = 10000;
    UBYTE                       info, mousecode, *mouse_data;

    info = i8042_read_status_port();

    for(; ((info = i8042_read_status_port()) & KBD_STATUS_OBF) && work; work--) {
        if (!(info & KBD_STATUS_MOUSE_OBF)) {
            /*
            ** Data from keyboard. Hopefully this gets through to keyboard interrupt
            ** if we break out of for loop here :-\
            */
            break;
        }

        mousecode = i8042_read_data_port();

        if (info & (KBD_STATUS_GTO | KBD_STATUS_PERR)) {
            /* Ignore errors and messages for keyboard -> eat status/error byte */
            continue;
        }

        /* Mouse Packet Byte */

        mouse_data = data->mouse_data;

        data->expected_mouse_acks = 0;
        mouse_data[data->mouse_collected_bytes] = mousecode;

        /* Packet validity check. Bit 3 of first mouse packet byte must be set */

        if ((mouse_data[0] & 8) == 0) {
            data->mouse_collected_bytes = 0;
            continue;
        }

        data->mouse_collected_bytes++;

        if (data->mouse_collected_bytes != data->mouse_packetsize) {
            /* Mouse Packet not yet complete */
            continue;
        }

        /* We have a complete mouse packet :-) */

        data->mouse_collected_bytes = 0;

        /*
         * Let's see whether these data can be right...
         *
         * D7 D6 D5 D4 D3 D2 D1 D0
         * YV XV YS XS  1  M  R  L
         * X7 .  .  .  .  .  .  X1   (X)
         * Y7 .  .  .  .  .  .  Y1   (Y)
         *
         * XV,YV : overflow in x/y direction
         * XS,YS : most significant bit of X and Y: represents sign
         * X,Y   : displacement in x and y direction (8 least significant bits).
         *
         * X, XS, Y and YS make up two 9-bit two's complement fields.
         */

#if 0
        D(bug("Got the following: 1. byte: 0x%x, dx=%d, dy=%d\n",
              mouse_data[0],
              mouse_data[1],
              mouse_data[2]));
#endif

        e->x = mouse_data[1];
        e->y = mouse_data[2];

        if (mouse_data[0] & 0x10) e->x -= 256;
        if (mouse_data[0] & 0x20) e->y -= 256;

        /* dy is reversed! */
        e->y = -(e->y);

        if (e->x || e->y) {
            e->button   = vHidd_Mouse_NoButton;
            e->type     = vHidd_Mouse_Motion;
            MOUSEPUSHEVENT(OOP_OCLASS(data->hwdata.self), mousehw, data->hwdata.self, e);
        }

        buttonstate = mouse_data[0] & 0x07;

        if ((buttonstate & LEFT_BUTTON) != (data->buttonstate & LEFT_BUTTON)) {
            e->button = vHidd_Mouse_Button1;
            e->type   = (buttonstate & LEFT_BUTTON) ? vHidd_Mouse_Press : vHidd_Mouse_Release;
            MOUSEPUSHEVENT(OOP_OCLASS(data->hwdata.self), mousehw, data->hwdata.self, e);
        }

        if ((buttonstate & RIGHT_BUTTON) != (data->buttonstate & RIGHT_BUTTON)) {
            e->button = vHidd_Mouse_Button2;
            e->type   = (buttonstate & RIGHT_BUTTON) ? vHidd_Mouse_Press : vHidd_Mouse_Release;
            MOUSEPUSHEVENT(OOP_OCLASS(data->hwdata.self), mousehw, data->hwdata.self, e);
        }

        if ((buttonstate & MIDDLE_BUTTON) != (data->buttonstate & MIDDLE_BUTTON)) {
            e->button = vHidd_Mouse_Button3;
            e->type = (buttonstate & MIDDLE_BUTTON) ? vHidd_Mouse_Press : vHidd_Mouse_Release;
            MOUSEPUSHEVENT(OOP_OCLASS(data->hwdata.self), mousehw, data->hwdata.self, e);
        }

        data->buttonstate = buttonstate;

#if INTELLIMOUSE_SUPPORT
        /* mouse wheel */
        e->y = (mouse_data[3] & 8) ? (mouse_data[3] & 15) - 16 : (mouse_data[3] & 15);
        if (e->y) {
            e->x = 0;
            e->type  = vHidd_Mouse_WheelMotion;
            e->button = vHidd_Mouse_NoButton;
            MOUSEPUSHEVENT(OOP_OCLASS(data->hwdata.self), mousehw, data->hwdata.self, e);
        }
#endif

    } /* for(; ((info = i8042_read_status_port()) & KBD_STATUS_OBF) && work; work--) */

    D(if (!work) bug("mouse.hidd: controller jammed (0x%02X).\n", info);)
    }

/****************************************************************************************
 * PS/2 Mouse Setup Task
 * It is safe to use functions that take the timer IORequest as input
 * from here on ....
 ****************************************************************************************/
void i8042_mouse_init_task(OOP_Class *cl, OOP_Object *o)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);
    struct Task *thisTask = FindTask(NULL);
    struct MsgPort *p = CreateMsgPort();
    int result;

    if (!p) {
        D(bug("[i8042:PS2Mouse] Failed to create Timer MsgPort..\n"));
        data->irq = 0;
        Signal(thisTask->tc_UserData, SIGF_SINGLE);
        return;
    }

    data->hwdata.ioTimer = CreateIORequest(p, sizeof(struct timerequest));
    if (!data->hwdata.ioTimer) {
        D(bug("[i8042:PS2Mouse] Failed to create Timer MsgPort..\n"));
        DeleteMsgPort(p);
        data->irq = 0;
        Signal(thisTask->tc_UserData, SIGF_SINGLE);
        return;
    }

    if (0 != OpenDevice("timer.device", UNIT_MICROHZ, data->hwdata.ioTimer, 0)) {
        D(bug("[i8042:PS2Mouse] Failed to open timer.device, unit MICROHZ\n");)
        DeleteIORequest(data->hwdata.ioTimer);
        DeleteMsgPort(p);
        data->irq = 0;
        Signal(thisTask->tc_UserData, SIGF_SINGLE);
        return;
    }

    D(bug("[i8042:PS2Mouse] registering interrupt handler for IRQ #%u\n", PS2MOUSEIRQ);)
    data->irq = KrnAddIRQHandler(PS2MOUSEIRQ, i8042_mouse_irq_handler, data, NULL);

    D(bug("[i8042:PS2Mouse] attempting reset to detect mouse ...\n");)
    Disable();
    result = i8042_mouse_reset((struct i8042base *)cl->UserData, data);
    Enable();

    /* If no valid PS/2 mouse detected, release the IRQ */
    if (!result) {
        D(bug("[i8042:PS2Mouse] No PS/2 Mouse detected!\n");)
        KrnRemIRQHandler(data->irq);
        data->irq = 0;
    }

    D(bug("[i8042:PS2Mouse] finished - signaling waiting task and cleaning up\n");)
    /* Signals the waiting task before we exit ... */
    Signal(thisTask->tc_UserData, SIGF_SINGLE);
    /* Timer request is not freed as it is needed for reset handler */

    return ;
}

/****************************************************************************************/

void i8042_mouse_get_state(OOP_Class *cl, OOP_Object *o, struct pHidd_Mouse_Event *event)
{
#if 0
    struct mouse_data *data = OOP_INST_DATA(cl, o);
    UBYTE ack;

    /* The following doesn't seem to do anything useful */
    aux_write(KBD_OUTCMD_DISABLE);
    /* switch to remote mode */
    aux_write(KBD_OUTCMD_SET_REMOTE_MODE);
    /* we want data */
    ack = data->expected_mouse_acks+1;
    aux_write(KBD_OUTCMD_READ_DATA);
    while (data->expected_mouse_acks>=ack)
        i8042_delay(tmr, 1000);
    /* switch back to stream mode */
    aux_write(KBD_OUTCMD_SET_STREAM_MODE);
    aux_write(KBD_OUTCMD_ENABLE);
#endif
}

/****************************************************************************************/

static int i8042_mouse_detect_aux_port(struct IORequest* tmr)
{
    int loops = 10;
    int retval = 0;

    i8042_wait_for_write_ready(tmr, 1000);

    i8042_write_command_port(KBD_CTRLCMD_WRITE_AUX_OBUF);

    i8042_wait_for_write_ready(tmr, 1000);
    i8042_write_data_port(0x5a);

    do {
        unsigned char status = i8042_read_status_port();

        if (status & KBD_STATUS_OBF) {
            (void) i8042_read_data_port();
            if (status & KBD_STATUS_MOUSE_OBF) {
                retval = 1;
            }
            break;
        }

        i8042_delay(tmr, 1000);

    } while (--loops);

    D(bug("PS/2 Auxilliary port %sdetected\n", retval ? "" : "not "));
    return retval;
}

/****************************************************************************************/

static int i8042_mouse_query(struct IORequest* tmr, UBYTE *buf, int size, int timeout)
{
    int ret = 0;

    do {
        UBYTE status = i8042_read_status_port();

        if (status & KBD_STATUS_OBF) {
            UBYTE c = i8042_read_data_port();

            if ((c != KBD_REPLY_ACK) && (status & KBD_STATUS_MOUSE_OBF)) {
                buf[ret++] = c;
            }
        } else {
            i8042_delay(tmr, 1000);
        }

    } while ((--timeout) && (ret < size));

    return ret;

}

/****************************************************************************************/

static int i8042_mouse_detect_intellimouse(struct IORequest* tmr)
{
    UBYTE id = 0;

    /* Try to switch into IMPS2 mode */

    i8042_mouse_write_ack(tmr, KBD_OUTCMD_SET_RATE);
    i8042_mouse_write_ack(tmr, 200);
    i8042_mouse_write_ack(tmr, KBD_OUTCMD_SET_RATE);
    i8042_mouse_write_ack(tmr, 100);
    i8042_mouse_write_ack(tmr, KBD_OUTCMD_SET_RATE);
    i8042_mouse_write_ack(tmr, 80);
    i8042_mouse_write_ack(tmr, KBD_OUTCMD_GET_ID);
    i8042_mouse_write_noack(tmr, KBD_OUTCMD_GET_ID);

    i8042_mouse_query(tmr, &id, 1, 20);

    return ((id == 3) || (id == 4)) ? id : 0;
}

/****************************************************************************************/

static AROS_INTH1(PS2KBMResetHandler, struct i8042base *, i8042Base)
{
    AROS_INTFUNC_INIT

    DRESET(bug("[i8042:PS2Mouse] %s(0x%p)\n", __func__, i8042Base);)

    /*
     * Turn off interrupts in the simplest way possible as this is an
     * interrupt handler and the system is going down. So we want to avoid
     * calling the timer for example and we don't care about preserving any
     * state
     */
    while(i8042_read_status_port() & KBD_STATUS_IBF);
    i8042_write_command_port(KBD_CTRLCMD_WRITE_MODE);
    while(i8042_read_status_port() & KBD_STATUS_IBF);
    i8042_write_data_port(0);

    return FALSE;

    AROS_INTFUNC_EXIT
}

int i8042_mouse_reset(struct i8042base *i8042Base, struct mouse_data *data)
{
    int result, timeout = 100;

    /*
     * The commands are for the mouse and nobody else.
     */

    i8042_write_command_with_wait(data->hwdata.ioTimer, KBD_CTRLCMD_MOUSE_ENABLE);

    /*
     * Check for a mouse port.
     */
    if (!i8042_mouse_detect_aux_port(data->hwdata.ioTimer))
        return 0;

    /*
     * Unfortunately on my computer these commands cause
     * the mouse not to work at all if they are issued
     * in a different order. So please keep it that way.
     * - Stefan
     */

    /*
     * Turn interrupts off and the keyboard as well since the
     * commands are all for the mouse.
     */
    i8042Base->csd.cs_intbits &= ~KBD_MODE_MOUSE_INT;
    i8042_write_mode(data->hwdata.ioTimer, (KBD_MODE_KCC | KBD_MODE_SYS) | i8042Base->csd.cs_intbits);
    i8042_write_command_with_wait(data->hwdata.ioTimer, KBD_CTRLCMD_KBD_DISABLE);

    /* Reset mouse */
    i8042_mouse_write_ack(data->hwdata.ioTimer, KBD_OUTCMD_RESET);
    result = i8042_mouse_wait_for_input(data->hwdata.ioTimer);    /* Test result (0xAA) */
    while (result == 0xfa && --timeout) {
        /* somehow the ACK isn't always swallowed above */
        i8042_delay(data->hwdata.ioTimer, 1000);
        result = i8042_mouse_wait_for_input(data->hwdata.ioTimer);
    }
    i8042_mouse_wait_for_input(data->hwdata.ioTimer);    /* Mouse type */

    if (result != 0xaa) {
        /* No mouse. Re-enable keyboard and return failure */
        i8042_write_command_with_wait(data->hwdata.ioTimer, KBD_CTRLCMD_KBD_ENABLE);
        i8042_mouse_write_ack(data->hwdata.ioTimer, KBD_OUTCMD_ENABLE);
        return 0;
    }

    data->mouse_protocol = PS2_PROTOCOL_STANDARD;
    data->mouse_packetsize = 3;

#if INTELLIMOUSE_SUPPORT
    if (i8042_mouse_detect_intellimouse(data->hwdata.ioTimer)) {
        D(bug("[i8042:PS2Mouse] PS/2 Intellimouse detected\n"));
        data->mouse_protocol = PS2_PROTOCOL_INTELLIMOUSE;
        data->mouse_packetsize = 4;
    }
#endif

    /*
     * Now the commands themselves.
     */
    i8042_mouse_write_ack(data->hwdata.ioTimer, KBD_OUTCMD_SET_RATE);
    i8042_mouse_write_ack(data->hwdata.ioTimer, 100);
    i8042_mouse_write_ack(data->hwdata.ioTimer, KBD_OUTCMD_SET_RES);
    i8042_mouse_write_ack(data->hwdata.ioTimer, 2);
    i8042_mouse_write_ack(data->hwdata.ioTimer, KBD_OUTCMD_SET_SCALE11);

    /* Enable Aux device (and re-enable keyboard) */

    i8042_write_command_with_wait(data->hwdata.ioTimer, KBD_CTRLCMD_KBD_ENABLE);
    i8042_mouse_write_ack(data->hwdata.ioTimer, KBD_OUTCMD_ENABLE);
    i8042Base->csd.cs_intbits |= KBD_MODE_MOUSE_INT;
    i8042_write_mode(data->hwdata.ioTimer, (KBD_MODE_KCC | KBD_MODE_SYS) | i8042Base->csd.cs_intbits);

    /*
     * According to the specs there is an external
     * latch that holds the level-sensitive interrupt
     * request until the CPU reads port 0x60.
     * If this is not read then the mouse does not
     * work on my computer.- Stefan
     */

    i8042_read_data();

    D(bug("[i8042:PS2Mouse] Found and initialized PS/2 mouse!\n"));

    // Install warm-reset handler
    i8042Base->csd.cs_ResetInt.is_Node.ln_Name = i8042Base->library.lib_Node.ln_Name;
    i8042Base->csd.cs_ResetInt.is_Node.ln_Pri = -10;
    i8042Base->csd.cs_ResetInt.is_Code = (VOID_FUNC)PS2KBMResetHandler;
    i8042Base->csd.cs_ResetInt.is_Data = i8042Base;
    AddResetCallback(&i8042Base->csd.cs_ResetInt);

    return 1;
}

/****************************************************************************************/
