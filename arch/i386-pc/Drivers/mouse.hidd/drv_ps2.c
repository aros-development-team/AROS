/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: PS/2 mouse driver.
    Lang: English.
*/

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

#define DEBUG 1
#include <aros/debug.h>

#define HiddMouseAB	(MSD(cl)->hiddMouseAB)

/* defines for buttonstate */

#define LEFT_BUTTON 	1
#define RIGHT_BUTTON 	2
#define MIDDLE_BUTTON	4

void mouse_usleep(ULONG usec);

/***** Test procedure ***********************************************/

void mouse_ps2int(HIDDT_IRQ_Handler *, HIDDT_IRQ_HwInfo *);
int mouse_ps2reset(struct mouse_data *);

int test_mouse_ps2(OOP_Class *cl, OOP_Object *o)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);
    
    /* Open IRQ Hidd */
    if ((data->u.ps2.irqhidd = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL)))
    {
        HIDDT_IRQ_Handler   *irq;

        if ((irq = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR | MEMF_PUBLIC)))        
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
            else
            {
                /* Either no PS/2 mouse or problems with it */
                /* Remove mouse interrupt */
                HIDD_IRQ_RemHandler(data->u.ps2.irqhidd, irq);
                /* Dispose IRQ object */
                OOP_DisposeObject(data->u.ps2.irqhidd);
                /* Free IRQ structure as it's not needed anymore */
                FreeMem(irq, sizeof(HIDDT_IRQ_Handler));
            }
        }
    }

    return 0; /* Report no PS/2 mouse */
}

/*****  *************************************************************/

#define WaitForInput                    \
    ({ int i = 0,dummy;                 \
       do                               \
       {                                \
        info = kbd_read_status();       \
       } while((info & KBD_STATUS_OBF));\
       while (i < 100000000)              \
       {                \
         dummy = i*i;   \
         i++;           \
       }})

#define WaitForOutput                   \
    ({  do                              \
        {                               \
            info=kbd_read_status();     \
        } while(info & KBD_STATUS_IBF); \
        kbd_read_input();               \
        })

#undef SysBase
#define SysBase (hw->sysBase)

#define AUX_RECONNECT           170
#define AUX_ACK                 0xFA 

#define aux_write(val)				\
    ({	data->u.ps2.expected_mouse_acks++;	\
        aux_write_ack(val);			\
	})

unsigned char handle_kbd_event(void);
void kb_wait(void);
void kbd_write_cmd(int cmd);
void aux_write_ack(int val);
void kbd_write_output_w(int data);
void kbd_write_command_w(int data);

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

void mouse_ps2int(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    UBYTE                       info;
    struct mouse_data           *data =(struct mouse_data *)irq->h_Data;
    struct pHidd_Mouse_Event    *e = &data->u.ps2.event;

    info = kbd_read_status();

    while ((info & KBD_STATUS_OBF))             /* data from information port */
    {
        if ((info & KBD_STATUS_MOUSE_OBF))       /* If bit 5 set data from mouse. */
        {
            UBYTE *mouse_data=data->u.ps2.mouse_data;
            
            UBYTE mousecode = kbd_read_input();
//            if (0xfa == mousecode)
	    if ((0xfa == mousecode) && (data->u.ps2.expected_mouse_acks))
            /* Check whether we are excepting ACK */
//            if (data->u.ps2.expected_mouse_acks)
            {
                if (mousecode == AUX_ACK)
                {
                    D(bug("                             Got a mouse ACK!\n"));
                    data->u.ps2.expected_mouse_acks--;
                }
                else data->u.ps2.expected_mouse_acks = 0;
            }
#if 0
            else if (mousecode == AUX_RECONNECT)
            {
                data->u.ps2.mouse_collected_bytes = 0;
                aux_write(KBD_OUTCMD_ENABLE);   /* Ping mouse */
            }
#endif
            else
            {
                data->u.ps2.expected_mouse_acks = 0;
                mouse_data[data->u.ps2.mouse_collected_bytes] = mousecode;
                if (0 == (mouse_data[0] & 8))
                    data->u.ps2.mouse_collected_bytes = 0;
                else
                {
                    data->u.ps2.mouse_collected_bytes++;

                    if (data->u.ps2.mouse_collected_bytes == 3)
                    {
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
                        if ( (( (mouse_data[0] & 0x10) && (char)mouse_data[1] <  0) ||
                              (!(mouse_data[0] & 0x10) && (char)mouse_data[1] >= 0)   ) &&
                             (( (mouse_data[0] & 0x20) && (char)mouse_data[2] <  0) ||
                              (!(mouse_data[0] & 0x20) && (char)mouse_data[2] >= 0    )))
                        {
                            UWORD buttonstate;
                            
                            D(bug("Got the following: 1. byte: 0x%x, dx=%d, dy=%d\n",
                                  mouse_data[0],
                                  mouse_data[1],
                                  mouse_data[2]));
                            
                            e->x = (char)mouse_data[1];
                            e->y = -(char)mouse_data[2];	/* dy is reversed! */

                            if (e->x || e->y)
                            {
                                e->button   = vHidd_Mouse_NoButton;
                                e->type     = vHidd_Mouse_Motion;

                                data->mouse_callback(data->callbackdata, e);
                            }
                            
                            buttonstate = mouse_data[0] & 0x07;

                            if ((buttonstate & LEFT_BUTTON) 
                                    != (data->buttonstate & LEFT_BUTTON))
                            {
                                e->button = vHidd_Mouse_Button1;
                                e->type = (buttonstate & LEFT_BUTTON) ?
                                    vHidd_Mouse_Press : vHidd_Mouse_Release;
                                data->mouse_callback(data->callbackdata, e);
                            }
                            if ((buttonstate & RIGHT_BUTTON) 
                                    != (data->buttonstate & RIGHT_BUTTON))
                            {
                                e->button = vHidd_Mouse_Button2;
                                e->type = (buttonstate & RIGHT_BUTTON) ?
                                    vHidd_Mouse_Press : vHidd_Mouse_Release;
                                data->mouse_callback(data->callbackdata, e);
                            }
                            if ((buttonstate & MIDDLE_BUTTON) 
                                    != (data->buttonstate & MIDDLE_BUTTON))
                            {
                                e->button = vHidd_Mouse_Button3;
                                e->type = (buttonstate & MIDDLE_BUTTON) ?
                                    vHidd_Mouse_Press : vHidd_Mouse_Release;
                                data->mouse_callback(data->callbackdata, e);
                            }

                            data->buttonstate = buttonstate;
                        }
                    }
                }
            }                                       
        }
        info = kbd_read_status();
    } /* while data can be read */
}

#undef SysBase
#define SysBase (*(struct ExecBase **)4L)

#define AUX_INTS_OFF (KBD_MODE_KCC | KBD_MODE_DISABLE_MOUSE | KBD_MODE_SYS | KBD_MODE_KBD_INT)
#define AUX_INTS_ON  (KBD_MODE_KCC | KBD_MODE_SYS | KBD_MODE_MOUSE_INT | KBD_MODE_KBD_INT)

int mouse_ps2reset(struct mouse_data *data)
{
    if (!kbd_detect_aux())
	return 0;

    kbd_write_command_w(KBD_CTRLCMD_MOUSE_ENABLE);
    aux_write(KBD_OUTCMD_SET_RATE);
    aux_write(80);
    aux_write(KBD_OUTCMD_SET_RES);
    aux_write(3);
    aux_write(KBD_OUTCMD_SET_SCALE11);
    kbd_write_command(KBD_CTRLCMD_MOUSE_DISABLE);
    kbd_write_cmd(AUX_INTS_OFF);
    
    /* Enable Aux device */
    
    kbd_write_command_w(KBD_CTRLCMD_MOUSE_ENABLE);
    aux_write(KBD_OUTCMD_ENABLE);
    kbd_write_cmd(AUX_INTS_ON);
    
    data->u.ps2.expected_mouse_acks = 6; /* for each aux_write_ack */

    D(bug("Initialized PS/2 mouse!\n"));

    return 1;
}


