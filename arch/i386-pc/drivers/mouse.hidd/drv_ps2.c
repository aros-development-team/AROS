/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PS/2 mouse driver.
    Lang: English.
*/

/****************************************************************************************/

#include <asm/io.h>
#include <asm/speaker.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>
#include <hidd/irq.h>
#include <devices/inputevent.h>

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

/* from Drivers/keyboard/kbd_common.c */

#warning "it is probably better to get rid of calling functions from external module"

int kbd_read_data(void);
void kb_wait(void);
void kbd_write_cmd(int cmd);
void aux_write_ack(int val);
void aux_write_noack(int val);
void kbd_write_command_w(int data);

/****************************************************************************************/

void mouse_usleep(ULONG usec);

/****************************************************************************************/

void mouse_ps2int(HIDDT_IRQ_Handler *, HIDDT_IRQ_HwInfo *);
int mouse_ps2reset(struct mouse_data *);

/****************************************************************************************/

int test_mouse_ps2(OOP_Class *cl, OOP_Object *o)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);
    
    /* Open IRQ Hidd */
    if ((data->u.ps2.irqhidd = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL)))
    {
        HIDDT_IRQ_Handler   *irq;

        data->u.ps2.irq = irq = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR | MEMF_PUBLIC);
        if (irq != NULL)
        {
            int result;
            
            irq->h_Node.ln_Pri  = 127;
            irq->h_Node.ln_Name = "PS/2 mouse class irq";
            irq->h_Code         = mouse_ps2int;
            irq->h_Data         = (APTR)data;

            HIDD_IRQ_AddHandler(data->u.ps2.irqhidd, irq, vHidd_IRQ_Mouse);

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
            HIDD_IRQ_RemHandler(data->u.ps2.irqhidd, irq);
            /* Free IRQ structure as it's not needed anymore */
            FreeMem(irq, sizeof(HIDDT_IRQ_Handler));
        }
        /* Dispose IRQ object */
        OOP_DisposeObject(data->u.ps2.irqhidd);
    }
    /* Report no PS/2 mouse */
    return 0; 
}

void dispose_mouse_ps2(OOP_Class *cl, OOP_Object *o) {
struct mouse_data *data = OOP_INST_DATA(cl, o);

   HIDD_IRQ_RemHandler(data->u.ps2.irqhidd, data->u.ps2.irq);
   FreeMem(data->u.ps2.irq, sizeof(HIDDT_IRQ_Handler));
   OOP_DisposeObject(data->u.ps2.irqhidd);
}

/****************************************************************************************/

#define AUX_RECONNECT           170
#define AUX_ACK                 0xFA 

#define aux_write(val)				\
    ({	data->u.ps2.expected_mouse_acks++;	\
        aux_write_ack(val);			\
	})

/****************************************************************************************/

void getps2Event(struct getps2data *, struct pHidd_Mouse_Event *);

void getps2State(OOP_Class *cl, OOP_Object *o, struct pHidd_Mouse_Event *event) {
struct mouse_data *data = OOP_INST_DATA(cl, o);
UBYTE ack;

	aux_write(KBD_OUTCMD_DISABLE);
	/* switch to remote mode */
	aux_write(KBD_OUTCMD_SET_REMOTE_MODE);
	/* we want data */
	ack = data->u.ps2.expected_mouse_acks+1;
	aux_write(KBD_OUTCMD_READ_DATA);
	while (data->u.ps2.expected_mouse_acks>=ack)
		mouse_usleep(1000);
	/* switch back to sream mode */
	aux_write(KBD_OUTCMD_SET_STREAM_MODE);
	aux_write(KBD_OUTCMD_ENABLE);
}

/****************************************************************************************/

int kbd_detect_aux()
{
    int loops = 10;
    int retval = 0;
    
    kb_wait();
	
    kbd_write_command(KBD_CTRLCMD_WRITE_AUX_OBUF);
    
    kb_wait();
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
	
	mouse_usleep(1000);
	
    } while (--loops);
    
    return retval;
}

/****************************************************************************************/

static int query_mouse(UBYTE *buf, int size, int timeout)
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
    
    aux_write_ack(KBD_OUTCMD_SET_RATE);
    aux_write_ack(200);
    aux_write_ack(KBD_OUTCMD_SET_RATE);
    aux_write_ack(100);
    aux_write_ack(KBD_OUTCMD_SET_RATE);
    aux_write_ack(80);     
    aux_write_ack(KBD_OUTCMD_GET_ID);
    aux_write_noack(KBD_OUTCMD_GET_ID);
    
    query_mouse(&id, 1, 20);

    return ((id == 3) || (id == 4)) ? id : 0;
}

/****************************************************************************************/

#undef SysBase
#define SysBase (hw->sysBase)


void mouse_ps2int(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    struct mouse_data           *data =(struct mouse_data *)irq->h_Data;
    struct pHidd_Mouse_Event    *e = &data->u.ps2.event;
    UWORD   	    	    	buttonstate;
    WORD    	    	    	work = 10000;
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
	
	if ((mousecode == AUX_ACK) && (data->u.ps2.expected_mouse_acks))
	{
	    D(bug("  Got a mouse ACK!\n"));
	    data->u.ps2.expected_mouse_acks--;
	    continue;
	}

    #if 1
	if (mousecode == AUX_RECONNECT)
	{
            data->u.ps2.mouse_collected_bytes = 0;
	    
	    /* Ping mouse */
    	    aux_write(KBD_OUTCMD_ENABLE); 
 	}
    #endif
    
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
         * YV XV YS X2  1  M  R  L
         * X7 .  .  .  .  .  .  X1   (X, signed)
         * Y7 .  .  .  .  .  .  Y1   (Y, signed)
         *
         * YV,XV : over flow in x/y direction
         * XS,YS : represents sign of X and Y
         * X,Y   : displacement in x and y direction.
         * X and Y are signed, XS, YS are there to double check the
         * sign and correctnes of the collected data (?).
         *
         * http://www.hut.fi/~then/mytexts/mouse.htm
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
    
    } /* for(; ((info = kbd_read_statues()) & KBD_STATUS_OBF) && work; work--) */

    if (!work)
    {
        D(bug("kbd.hidd: controller jammed (0x%02X).\n", info));
    }	

}

/****************************************************************************************/

#undef SysBase
#define SysBase (*(struct ExecBase **)4L)

#define AUX_INTS_OFF (KBD_MODE_KCC | KBD_MODE_DISABLE_MOUSE | KBD_MODE_SYS | KBD_MODE_KBD_INT)
#define AUX_INTS_ON  (KBD_MODE_KCC | KBD_MODE_SYS | KBD_MODE_MOUSE_INT | KBD_MODE_KBD_INT)

/****************************************************************************************/

int mouse_ps2reset(struct mouse_data *data)
{
    /* 
     * The commands are for the mouse and nobody else.
     */
    
    kbd_write_command_w(KBD_CTRLCMD_MOUSE_ENABLE);

    if (!kbd_detect_aux())
	return 0;

    /*
     * Unfortunatley on my computer these commands cause
     * the mouse not to work at all if they are issued
     * in a different order. So please keep it that way.
     * - Stefan
     */

    /*
     * Turn interrupts off and the keyboard as well since the
     * commands are all for the mouse.
     */
    kbd_write_cmd(AUX_INTS_OFF);
    kbd_write_command_w(KBD_CTRLCMD_KBD_DISABLE);

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
    aux_write(KBD_OUTCMD_SET_RATE);
    aux_write(100);
    aux_write(KBD_OUTCMD_SET_RES);
    aux_write(2);
    aux_write(KBD_OUTCMD_SET_SCALE11);
    
    /* Enable Aux device */
    
    kbd_write_command_w(KBD_CTRLCMD_KBD_ENABLE);
    aux_write(KBD_OUTCMD_ENABLE);
    kbd_write_cmd(AUX_INTS_ON);

    /*
     * According to the specs there is an external
     * latch that holds the level-sensitive interrupt
     * request until the CPU readsport 0x60.
     * If this is not read then the mouse does not
     * work on my computer.- Stefan
     */
     
    kbd_read_data();

    D(bug("[Mouse] Found and initialized PS/2 mouse!\n"));

    return 1;
}

/****************************************************************************************/
