/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PS/2 mouse driver.
    Lang: English.
*/

#include <aros/debug.h>

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

int ps2mouse_reset(struct IORequest* tmr, struct mouse_data *);

/****************************************************************************************
 * PS/2 Mouse Interrupt handler
 * NB: Do NOT use any functions that take the timer IORequest as input
 * in this code or from any functions it may call!.
 ****************************************************************************************/
static void PS2Mouse_IntHandler(struct mouse_data *data, void *unused)
{
    struct pHidd_Mouse_Event    *e = &data->event;
    UWORD                       buttonstate;
    WORD                        work = 10000;
    UBYTE                       info, mousecode, *mouse_data;

    info = kbd_read_status();

    for(; ((info = kbd_read_status()) & KBD_STATUS_OBF) && work; work--)
    {
        if (!(info & KBD_STATUS_MOUSE_OBF))
        {
            /*
            ** Data from keyboard. Hopefully this gets through to keyboard interrupt
            ** if we break out of for loop here :-\
            */
            break;
        }

        mousecode = kbd_read_input();

        if (info & (KBD_STATUS_GTO | KBD_STATUS_PERR))
        {
            /* Ignore errors and messages for keyboard -> eat status/error byte */
            continue;
        }

        /* Mouse Packet Byte */

        mouse_data = data->mouse_data;

        data->expected_mouse_acks = 0;
        mouse_data[data->mouse_collected_bytes] = mousecode;

        /* Packet validity check. Bit 3 of first mouse packet byte must be set */

        if ((mouse_data[0] & 8) == 0)
        {
            data->mouse_collected_bytes = 0;
            continue;
        }

        data->mouse_collected_bytes++;

        if (data->mouse_collected_bytes != data->mouse_packetsize)
        {
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

        if (e->x || e->y)
        {
            e->button   = vHidd_Mouse_NoButton;
            e->type     = vHidd_Mouse_Motion;

            data->mouse_callback(data->callbackdata, e);
        }

        buttonstate = mouse_data[0] & 0x07;

        if ((buttonstate & LEFT_BUTTON) != (data->buttonstate & LEFT_BUTTON))
        {
            e->button = vHidd_Mouse_Button1;
            e->type   = (buttonstate & LEFT_BUTTON) ? vHidd_Mouse_Press : vHidd_Mouse_Release;

            data->mouse_callback(data->callbackdata, e);
        }

        if ((buttonstate & RIGHT_BUTTON) != (data->buttonstate & RIGHT_BUTTON))
        {
            e->button = vHidd_Mouse_Button2;
            e->type   = (buttonstate & RIGHT_BUTTON) ? vHidd_Mouse_Press : vHidd_Mouse_Release;

            data->mouse_callback(data->callbackdata, e);
        }

        if ((buttonstate & MIDDLE_BUTTON) != (data->buttonstate & MIDDLE_BUTTON))
        {
            e->button = vHidd_Mouse_Button3;
            e->type = (buttonstate & MIDDLE_BUTTON) ? vHidd_Mouse_Press : vHidd_Mouse_Release;

            data->mouse_callback(data->callbackdata, e);
        }

        data->buttonstate = buttonstate;

#if INTELLIMOUSE_SUPPORT
        /* mouse wheel */
        e->y = (mouse_data[3] & 8) ? (mouse_data[3] & 15) - 16 : (mouse_data[3] & 15);
        if (e->y)
        {
            e->x = 0;
            e->type  = vHidd_Mouse_WheelMotion;
            e->button = vHidd_Mouse_NoButton;

            data->mouse_callback(data->callbackdata, e);
        }
#endif

    } /* for(; ((info = kbd_read_status()) & KBD_STATUS_OBF) && work; work--) */

    D(if (!work) bug("mouse.hidd: controller jammed (0x%02X).\n", info);)
}

/****************************************************************************************
 * PS/2 Mouse Setup Task
 * It is safe to use functions that take the timer IORequest as input
 * from here on ....
 ****************************************************************************************/
void PS2Mouse_InitTask(OOP_Class *cl, OOP_Object *o)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);
    struct Task *thisTask = FindTask(NULL);
    struct MsgPort *p = CreateMsgPort();
    struct IORequest *tmr;
    int result;

    if (!p)
    {
        D(bug("[i8042:PS2Mouse] Failed to create Timer MsgPort..\n"));
        data->irq = 0;
        Signal(thisTask->tc_UserData, SIGF_SINGLE);
        return;
    }

    tmr = CreateIORequest(p, sizeof(struct timerequest));
    if (!tmr)
    {
        D(bug("[i8042:PS2Mouse] Failed to create Timer MsgPort..\n"));
	DeleteMsgPort(p);
        data->irq = 0;
        Signal(thisTask->tc_UserData, SIGF_SINGLE);
        return;
    }

    if (0 != OpenDevice("timer.device", UNIT_MICROHZ, tmr, 0))	
    {
        D(bug("[i8042:PS2Mouse] Failed to open timer.device, unit MICROHZ\n");)
        DeleteIORequest(tmr);
	DeleteMsgPort(p);
        data->irq = 0;
        Signal(thisTask->tc_UserData, SIGF_SINGLE);
        return;
    }

    D(bug("[i8042:PS2Mouse] registering interrupt handler for IRQ #%u\n", PS2MOUSEIRQ);)
    data->irq = KrnAddIRQHandler(PS2MOUSEIRQ, PS2Mouse_IntHandler, data, NULL);

    D(bug("[i8042:PS2Mouse] attempting reset to detect mouse ...\n");)
    Disable();
    result = ps2mouse_reset(tmr, data);
    Enable();

    /* If no valid PS/2 mouse detected, release the IRQ */
    if (!result)
    {
        D(bug("[i8042:PS2Mouse] No PS/2 Mouse detected!\n");)
        KrnRemIRQHandler(data->irq);
        data->irq = 0;
    }

    D(bug("[i8042:PS2Mouse] finished - signaling waiting task and cleaning up\n");)
    /* Signals the waiting task before we exit ... */
    Signal(thisTask->tc_UserData, SIGF_SINGLE);
    DeleteIORequest(tmr);
    DeleteMsgPort(p);

    return ;
}

/****************************************************************************************/

void ps2mouse_getstate(OOP_Class *cl, OOP_Object *o, struct pHidd_Mouse_Event *event)
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
                kbd_usleep(tmr, 1000);
        /* switch back to stream mode */
        aux_write(KBD_OUTCMD_SET_STREAM_MODE);
        aux_write(KBD_OUTCMD_ENABLE);
#endif
}

/****************************************************************************************/

static int ps2mouse_auxdetect(struct IORequest* tmr)
{
    int loops = 10;
    int retval = 0;

    kb_wait(tmr, 1000);

    kbd_write_command(KBD_CTRLCMD_WRITE_AUX_OBUF);

    kb_wait(tmr, 1000);
    kbd_write_output(0x5a);

    do
    {
        unsigned char status = kbd_read_status();

        if (status & KBD_STATUS_OBF)
        {
            (void) kbd_read_input();
            if (status & KBD_STATUS_MOUSE_OBF)
            {
                retval = 1;
            }
            break;
        }

        kbd_usleep(tmr, 1000);

    } while (--loops);

    D(bug("PS/2 Auxilliary port %sdetected\n", retval ? "" : "not "));
    return retval;
}

/****************************************************************************************/

static int ps2mouse_query(struct IORequest* tmr, UBYTE *buf, int size, int timeout)
{
    int ret = 0;

    do
    {
        UBYTE status = kbd_read_status();

        if (status & KBD_STATUS_OBF)
        {
            UBYTE c = kbd_read_input();

            if ((c != KBD_REPLY_ACK) && (status & KBD_STATUS_MOUSE_OBF))
            {
                buf[ret++] = c;
            }
        }
        else
        {
            kbd_usleep(tmr, 1000);
        }

    } while ((--timeout) && (ret < size));

    return ret;

}

/****************************************************************************************/

static int ps2mouse_detectintellimouse(struct IORequest* tmr)
{
    UBYTE id = 0;

    /* Try to switch into IMPS2 mode */

    aux_write_ack(tmr, KBD_OUTCMD_SET_RATE);
    aux_write_ack(tmr, 200);
    aux_write_ack(tmr, KBD_OUTCMD_SET_RATE);
    aux_write_ack(tmr, 100);
    aux_write_ack(tmr, KBD_OUTCMD_SET_RATE);
    aux_write_ack(tmr, 80);
    aux_write_ack(tmr, KBD_OUTCMD_GET_ID);
    aux_write_noack(tmr, KBD_OUTCMD_GET_ID);

    ps2mouse_query(tmr, &id, 1, 20);

    return ((id == 3) || (id == 4)) ? id : 0;
}

/****************************************************************************************/

#define AUX_INTS_OFF (KBD_MODE_KCC | KBD_MODE_DISABLE_MOUSE | KBD_MODE_SYS | KBD_MODE_KBD_INT)
#define AUX_INTS_ON  (KBD_MODE_KCC | KBD_MODE_SYS | KBD_MODE_MOUSE_INT | KBD_MODE_KBD_INT)

/****************************************************************************************/

int ps2mouse_reset(struct IORequest* tmr, struct mouse_data *data)
{
    int result, timeout = 100;

    /*
     * The commands are for the mouse and nobody else.
     */

    kbd_write_command_w(tmr, KBD_CTRLCMD_MOUSE_ENABLE);

    /*
     * Check for a mouse port.
     */
    if (!ps2mouse_auxdetect(tmr))
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
    kbd_write_cmd(tmr, AUX_INTS_OFF);
    kbd_write_command_w(tmr, KBD_CTRLCMD_KBD_DISABLE);

    /* Reset mouse */
    aux_write_ack(tmr, KBD_OUTCMD_RESET);
    result = kbd_wait_for_input(tmr);    /* Test result (0xAA) */
    while (result == 0xfa && --timeout)
    {
        /* somehow the ACK isn't always swallowed above */
        kbd_usleep(tmr, 1000);
        result = kbd_wait_for_input(tmr);
    }
    kbd_wait_for_input(tmr);    /* Mouse type */

    if (result != 0xaa)
    {
        /* No mouse. Re-enable keyboard and return failure */
        kbd_write_command_w(tmr, KBD_CTRLCMD_KBD_ENABLE);
        aux_write_ack(tmr, KBD_OUTCMD_ENABLE);
        return 0;
    }

    data->mouse_protocol = PS2_PROTOCOL_STANDARD;
    data->mouse_packetsize = 3;

#if INTELLIMOUSE_SUPPORT
    if (ps2mouse_detectintellimouse(tmr))
    {
        D(bug("[i8042:PS2Mouse] PS/2 Intellimouse detected\n"));
        data->mouse_protocol = PS2_PROTOCOL_INTELLIMOUSE;
        data->mouse_packetsize = 4;
    }
#endif

    /*
     * Now the commands themselves.
     */
    aux_write_ack(tmr, KBD_OUTCMD_SET_RATE);
    aux_write_ack(tmr, 100);
    aux_write_ack(tmr, KBD_OUTCMD_SET_RES);
    aux_write_ack(tmr, 2);
    aux_write_ack(tmr, KBD_OUTCMD_SET_SCALE11);

    /* Enable Aux device (and re-enable keyboard) */

    kbd_write_command_w(tmr, KBD_CTRLCMD_KBD_ENABLE);
    aux_write_ack(tmr, KBD_OUTCMD_ENABLE);
    kbd_write_cmd(tmr, AUX_INTS_ON);

    /*
     * According to the specs there is an external
     * latch that holds the level-sensitive interrupt
     * request until the CPU reads port 0x60.
     * If this is not read then the mouse does not
     * work on my computer.- Stefan
     */

    kbd_read_data();

    D(bug("[i8042:PS2Mouse] Found and initialized PS/2 mouse!\n"));

    return 1;
}

/****************************************************************************************/
