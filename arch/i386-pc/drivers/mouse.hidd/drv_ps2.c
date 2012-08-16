/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PS/2 mouse driver.
    Lang: English.
*/

/****************************************************************************************/

#include <asm/speaker.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>
#include <devices/inputevent.h>

#include <SDI/SDI_interrupt.h>

#include "mouse.h"

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

#ifdef HiddMouseAB
#undef HiddMouseAB
#endif
#define HiddMouseAB	(MSD(cl)->hiddMouseAB)

/* defines for buttonstate */

#define LEFT_BUTTON 	1
#define RIGHT_BUTTON 	2
#define MIDDLE_BUTTON	4

/****************************************************************************************/

#define TIMER_RPROK 3599597124UL

int mouse_wait_for_input(void);

/****************************************************************************************/

int mouse_ps2reset(struct mouse_data *);

/****************************************************************************************/

static ULONG usec2tick(ULONG usec)
{
    ULONG ret;
    ULONG prok = TIMER_RPROK;
    asm volatile("movl $0,%%eax; divl %2":"=a"(ret):"d"(usec),"m"(prok));
    return ret;
}

static void mouse_usleep(LONG usec)
{
    int oldtick, tick;
    usec = usec2tick(usec);

    outb(0x80, 0x43);
    oldtick = inb(0x42);
    oldtick += inb(0x42) << 8;

    while (usec > 0)
    {
        outb(0x80, 0x43);
        tick = inb(0x42);
        tick += inb(0x42) << 8;

        usec -= (oldtick - tick);
        if (tick > oldtick) usec -= 0x10000;
        oldtick = tick;
    }
}

unsigned char handle_mouse_event(void)
{
    unsigned char status = mouse_read_status();
    unsigned int work = 10000;

    while (status & KBD_STATUS_OBF)
    {
        mouse_read_input();

        status = mouse_read_status();
	if(!work--)
        {
	    //printf(KERN_ERR "pc_keyb: controller jammed (0x%02X).\n",status);
            break;
	}
    }
    return status;
}

/*
 * Wait until we can write to a peripheral again. Any input that comes in
 * while we're waiting is discarded.
 */
void mouse_wait(void)
{
    ULONG timeout = 1000; /* 1 sec should be enough */

    do
    {
	unsigned char status = handle_mouse_event();
	if (! (status & KBD_STATUS_IBF))
	    return;
	
	mouse_usleep(1000);
	timeout--;
    } while (timeout);
}

void mouse_write_cmd(int cmd)
{
    mouse_wait();
    mouse_write_command(KBD_CTRLCMD_WRITE_MODE);
    mouse_wait();
    mouse_write_output(cmd);
}

void mouse_write_ack(int val)
{
    mouse_wait();
    mouse_write_command(KBD_CTRLCMD_WRITE_MOUSE);
    mouse_wait();
    mouse_write_output(val);
    mouse_wait_for_input();
}

void mouse_write_noack(int val)
{
    mouse_wait();
    mouse_write_command(KBD_CTRLCMD_WRITE_MOUSE);
    mouse_wait();
    mouse_write_output(val);
}

void mouse_write_output_w(int data)
{
    mouse_wait();
    mouse_write_output(data);
}

void mouse_write_command_w(int data)
{
    mouse_wait();
    mouse_write_command(data);
}

#define KBD_NO_DATA	(-1)
#define KBD_BAD_DATA	(-2)

int mouse_read_data(void)
{
    LONG	retval = KBD_NO_DATA;
    UBYTE	status;
    
    status = mouse_read_status();
    if (status & KBD_STATUS_OBF)
    {
        UBYTE	data = mouse_read_input();
	
        retval = data;
        if (status & (KBD_STATUS_GTO | KBD_STATUS_PERR))
            retval = KBD_BAD_DATA;
    }
    
    return retval;
}

int mouse_clear_input(void)
{
    int maxread = 100, code, lastcode = KBD_NO_DATA;
    UBYTE status;

    do
    {
        status = mouse_read_status();
        if ((code = mouse_read_data()) == KBD_NO_DATA)
            break;
        if (!(status & KBD_STATUS_MOUSE_OBF))
            lastcode = code;
    } while (--maxread);

    return lastcode;
}

int mouse_wait_for_input(void)
{
    ULONG timeout = 1000;

    do
    {
        int retval = mouse_read_data();
        if (retval >= 0)
            return retval;
        mouse_usleep(1000);
    } while(--timeout);
    return -1;
}

/****************************************************************************************/

AROS_INTH1(mouse_ps2int,struct mouse_data *, data)
{
    AROS_INTFUNC_INIT

    struct pHidd_Mouse_Event    *e = &data->u.ps2.event;
    UWORD   	    	    	buttonstate;
    WORD    	    	    	work = 10000;
    UBYTE                       info, mousecode, *mouse_data;

    info = mouse_read_status();

    for(; ((info = mouse_read_status()) & KBD_STATUS_OBF) && work; work--)
    {
    	if (!(info & KBD_STATUS_MOUSE_OBF))
	{
	    /*
	    ** Data from keyboard. Hopefully this gets through to keyboard interrupt
	    ** if we break out of for loop here :-\
	    */
	    break;
	}

	mousecode = mouse_read_input();

	if (info & (KBD_STATUS_GTO | KBD_STATUS_PERR))
	{
            /* Ignore errors and messages for keyboard -> eat status/error byte */
	    continue;
	}

    	/* Mouse Packet Byte */

	mouse_data = data->u.ps2.mouse_data;

        data->u.ps2.expected_mouse_acks = 0;
        mouse_data[data->u.ps2.mouse_collected_bytes] = mousecode;

	/* Packet validity check. Bit 3 of first mouse packet byte must be set */

	if ((mouse_data[0] & 8) == 0)
	{
            data->u.ps2.mouse_collected_bytes = 0;
	    continue;
	}

        data->u.ps2.mouse_collected_bytes++;

	if (data->u.ps2.mouse_collected_bytes != data->u.ps2.mouse_packetsize)
	{
	    /* Mouse Packet not yet complete */
	    continue;
	}

	/* We have a complete mouse packet :-) */

    	data->u.ps2.mouse_collected_bytes = 0;

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

    } /* for(; ((info = mouse_read_status()) & KBD_STATUS_OBF) && work; work--) */

    if (!work)
    {
        D(bug("mouse.hidd: controller jammed (0x%02X).\n", info));
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

/****************************************************************************************/

int test_mouse_ps2(OOP_Class *cl, OOP_Object *o)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);
    struct Library *kbd_hidd;
    struct Interrupt    *irq;
    int result;

    /* Test for a PS/2 controller */
    if ((kbd_hidd = OpenLibrary("kbd.hidd", 0)) == NULL)
        return 0;
    CloseLibrary(kbd_hidd);

    irq = &data->u.ps2.irq;

    irq->is_Node.ln_Type = NT_INTERRUPT;
    irq->is_Node.ln_Pri  = 127;
    irq->is_Node.ln_Name = "PS/2 mouse class irq";
    irq->is_Code         = (VOID_FUNC)mouse_ps2int;
    irq->is_Data         = (APTR)data;

    AddIntServer(INTB_KERNEL + 12, irq);

    Disable();
    result = mouse_ps2reset(data);
    Enable();

    /* If mouse_ps2reset() returned non-zero value, there is vaild PS/2 mouse */
    if (result)
    {
        return 1;
    }
    /* Either no PS/2 mouse or problems with it */
    /* Remove mouse interrupt */
    RemIntServer(INTB_KERNEL + 12, irq);

    /* Report no PS/2 mouse */
    return 0;
}

void dispose_mouse_ps2(OOP_Class *cl, OOP_Object *o) {
struct mouse_data *data = OOP_INST_DATA(cl, o);

   RemIntServer(INTB_KERNEL + 12, &data->u.ps2.irq);
}

/****************************************************************************************/

void getps2State(OOP_Class *cl, OOP_Object *o, struct pHidd_Mouse_Event *event)
{
#if 0
struct mouse_data *data = OOP_INST_DATA(cl, o);
UBYTE ack;

/* The following doesn't seem to do anything useful */
	mouse_write(KBD_OUTCMD_DISABLE);
	/* switch to remote mode */
	mouse_write(KBD_OUTCMD_SET_REMOTE_MODE);
	/* we want data */
	ack = data->u.ps2.expected_mouse_acks+1;
	mouse_write(KBD_OUTCMD_READ_DATA);
	while (data->u.ps2.expected_mouse_acks>=ack)
		mouse_usleep(1000);
	/* switch back to stream mode */
	mouse_write(KBD_OUTCMD_SET_STREAM_MODE);
	mouse_write(KBD_OUTCMD_ENABLE);
#endif
}

/****************************************************************************************/

static int detect_aux()
{
    int loops = 10;
    int retval = 0;

    mouse_wait();

    mouse_write_command(KBD_CTRLCMD_WRITE_AUX_OBUF);

    mouse_wait();
    mouse_write_output(0x5a);

    do
    {
	unsigned char status = mouse_read_status();

	if (status & KBD_STATUS_OBF)
	{
	    (void) mouse_read_input();
	    if (status & KBD_STATUS_MOUSE_OBF)
	    {
		retval = 1;
	    }
	    break;
	}

	mouse_usleep(1000);

    } while (--loops);

    D(bug("PS/2 Auxilliary port %sdetected\n", retval ? "" : "not "));
    return retval;
}

/****************************************************************************************/

static int query_mouse(UBYTE *buf, int size, int timeout)
{
    int ret = 0;

    do
    {
	UBYTE status = mouse_read_status();

	if (status & KBD_STATUS_OBF)
	{
	    UBYTE c = mouse_read_input();

	    if ((c != KBD_REPLY_ACK) && (status & KBD_STATUS_MOUSE_OBF))
	    {
		buf[ret++] = c;
	    }
	}
    	else
	{
	    mouse_usleep(1000);
	}

    } while ((--timeout) && (ret < size));

    return ret;

}

/****************************************************************************************/

static int detect_intellimouse(void)
{
    UBYTE id = 0;

    /* Try to switch into IMPS2 mode */

    mouse_write_ack(KBD_OUTCMD_SET_RATE);
    mouse_write_ack(200);
    mouse_write_ack(KBD_OUTCMD_SET_RATE);
    mouse_write_ack(100);
    mouse_write_ack(KBD_OUTCMD_SET_RATE);
    mouse_write_ack(80);
    mouse_write_ack(KBD_OUTCMD_GET_ID);
    mouse_write_noack(KBD_OUTCMD_GET_ID);

    query_mouse(&id, 1, 20);

    return ((id == 3) || (id == 4)) ? id : 0;
}

/****************************************************************************************/

#define AUX_INTS_OFF (KBD_MODE_KCC | KBD_MODE_DISABLE_MOUSE | KBD_MODE_SYS | KBD_MODE_KBD_INT)
#define AUX_INTS_ON  (KBD_MODE_KCC | KBD_MODE_SYS | KBD_MODE_MOUSE_INT | KBD_MODE_KBD_INT)

/****************************************************************************************/

int mouse_ps2reset(struct mouse_data *data)
{
    /*
     * The commands are for the mouse and nobody else.
     */

    mouse_write_command_w(KBD_CTRLCMD_MOUSE_ENABLE);

    /*
     * Check for a mouse port.
     */
    if (!detect_aux())
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
    mouse_write_cmd(AUX_INTS_OFF);
    mouse_write_command_w(KBD_CTRLCMD_KBD_DISABLE);

    /* Reset mouse */
    mouse_write_ack(KBD_OUTCMD_RESET);
    mouse_wait_for_input();    /* Test result (0xAA) */
    mouse_wait_for_input();    /* Mouse type */

    data->u.ps2.mouse_protocol = PS2_PROTOCOL_STANDARD;
    data->u.ps2.mouse_packetsize = 3;

#if INTELLIMOUSE_SUPPORT
    if (detect_intellimouse())
    {
    	data->u.ps2.mouse_protocol = PS2_PROTOCOL_INTELLIMOUSE;
    	data->u.ps2.mouse_packetsize = 4;
    }
#endif

    /*
     * Now the commands themselves.
     */
    mouse_write_ack(KBD_OUTCMD_SET_RATE);
    mouse_write_ack(100);
    mouse_write_ack(KBD_OUTCMD_SET_RES);
    mouse_write_ack(2);
    mouse_write_ack(KBD_OUTCMD_SET_SCALE11);

    /* Enable Aux device (and re-enable keyboard) */

    mouse_write_command_w(KBD_CTRLCMD_KBD_ENABLE);
    mouse_write_ack(KBD_OUTCMD_ENABLE);
    mouse_write_cmd(AUX_INTS_ON);

    /*
     * According to the specs there is an external
     * latch that holds the level-sensitive interrupt
     * request until the CPU reads port 0x60.
     * If this is not read then the mouse does not
     * work on my computer.- Stefan
     */

    mouse_read_data();

    D(bug("[Mouse] Found and initialized PS/2 mouse!\n"));

    return 1;
}

/****************************************************************************************/
