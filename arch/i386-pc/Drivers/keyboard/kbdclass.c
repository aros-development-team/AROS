/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The main keyboard class.
    Lang: English.
*/

/****************************************************************************************/

#define AROS_ALMOST_COMPATIBLE
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/irq.h>
#include <hidd/keyboard.h>

#include <aros/system.h>
#include <aros/symbolsets.h>

#include <hardware/custom.h>

#include <devices/inputevent.h>
#include <devices/rawkeycodes.h>

#include "kbd.h"
#include "keys.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

/* Predefinitions */

void kbd_keyint(HIDDT_IRQ_Handler *, HIDDT_IRQ_HwInfo *);

void kbd_updateleds();
int  kbd_reset(void);

void kb_wait(void);
void kbd_write_cmd(int cmd);
void aux_write_ack(int val);
void kbd_write_output_w(int data);
void kbd_write_command_w(int data);
void mouse_usleep(ULONG);
void kbd_clear_input(void);
int  kbd_wait_for_input(void);
int  kbd_read_data(void);

/****************************************************************************************/

#undef HiddKbdAB
#define HiddKbdAB   (XSD(cl)->hiddKbdAB)

/****************************************************************************************/

#define NOKEY -1

/****************************************************************************************/

#include "stdkeytable.h"

/****************************************************************************************/

#include "e0keytable.h"

/****************************************************************************************/

OOP_Object * PCKbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem *tag, *tstate;
    APTR    	    callback = NULL;
    APTR    	    callbackdata = NULL;
    BOOL    	    has_kbd_hidd = FALSE;
    
    EnterFunc(bug("Kbd::New()\n"));

#if 1 /* FIXME: REMOVEME: just a debugging thing for the weird s-key problem */
    SysBase->ex_Reserved2[1] = (ULONG)std_keytable;
#endif
     
    ObtainSemaphoreShared( &XSD(cl)->sema);

    if (XSD(cl)->kbdhidd)
    	has_kbd_hidd = TRUE;

    ReleaseSemaphore( &XSD(cl)->sema);
 
    if (has_kbd_hidd) /* Cannot open twice */
    	ReturnPtr("Kbd::New", OOP_Object *, NULL); /* Should have some error code here */

    tstate = msg->attrList;
    D(bug("Kbd: tstate: %p, tag=%x\n", tstate, tstate->ti_Tag));
    
    while ((tag = NextTagItem((const struct TagItem **)&tstate)))
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

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct kbd_data *data = OOP_INST_DATA(cl, o);
        
        data->kbd_callback   = (VOID (*)(APTR, UWORD))callback;
        data->callbackdata   = callbackdata;
    	data->prev_amigacode = -2;
	data->prev_keycode   = 0;
	
        /* Get irq.hidd */

        if ((XSD(cl)->irqhidd = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL)))
        {
            /* Install keyboard interrupt */

            HIDDT_IRQ_Handler   *irq;

            XSD(cl)->irq = irq = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR|MEMF_PUBLIC);

            if (!irq)
            {
                kprintf("ERROR: Cannot install Keyboard\n");
                Alert( AT_DeadEnd | AN_IntrMem );
            }
	        
            irq->h_Node.ln_Pri  = 127;		/* Set the highest pri */
            irq->h_Node.ln_Name = "Keyboard class irq";
            irq->h_Code         = kbd_keyint;
            irq->h_Data         = (APTR)data;
            Disable();
	    kbd_clear_input();
	    kbd_reset();		/* Reset the keyboard */
            kbd_updateleds(0);
            Enable();

            HIDD_IRQ_AddHandler(XSD(cl)->irqhidd, irq, vHidd_IRQ_Keyboard);
            ObtainSemaphore(&XSD(cl)->sema);
            XSD(cl)->kbdhidd = o;
            ReleaseSemaphore(&XSD(cl)->sema);

        } /* if ((XSD(cl)->irqhidd = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL))) */
	
    } /* if (o) */
    
    ReturnPtr("Kbd::New", OOP_Object *, o);
}

VOID PCKbd__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    ObtainSemaphore(&XSD(cl)->sema);
    XSD(cl)->kbdhidd = NULL;
    ReleaseSemaphore(&XSD(cl)->sema);
    HIDD_IRQ_RemHandler(XSD(cl)->irqhidd, XSD(cl)->irq);
    FreeMem(XSD(cl)->irq, sizeof(HIDDT_IRQ_Handler));
    OOP_DisposeObject(XSD(cl)->irqhidd);
    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

VOID PCKbd__Hidd_Kbd__HandleEvent(OOP_Class *cl, OOP_Object *o, struct pHidd_Kbd_HandleEvent *msg)
{
    struct kbd_data * data;

    EnterFunc(bug("kbd_handleevent()\n"));

    data = OOP_INST_DATA(cl, o);
    
    ReturnVoid("Kbd::HandleEvent");
}


/****************************************************************************************/

AROS_SET_LIBFUNC(PCKbd_InitAttrs, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd_Kbd	, &LIBBASE->ksd.hiddKbdAB   },
        {NULL	    	, NULL      	    }
    };
    
    ReturnInt("PCKbd_InitAttrs", ULONG, OOP_ObtainAttrBases(attrbases));
    
    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_LIBFUNC(PCKbd_ExpungeAttrs, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd_Kbd	, &LIBBASE->ksd.hiddKbdAB   },
        {NULL	    	, NULL      	    }
    };
    
    EnterFunc(bug("PCKbd_ExpungeAttrs\n"));

    OOP_ReleaseAttrBases(attrbases);
    
    ReturnInt("PCKbd_ExpungeAttrs", ULONG, TRUE);
    
    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

ADD2INITLIB(PCKbd_InitAttrs, 0)
ADD2EXPUNGELIB(PCKbd_ExpungeAttrs, 0)

/****************************************************************************************/

#define WaitForInput        		\
    ({ int i = 0,dummy,timeout=1000;	\
       do                   		\
       {                    		\
        info = kbd_read_status();     	\
        if (!--timeout)			\
          break;			\
       } while((info & KBD_STATUS_OBF));\
       while (i < 1000000)     		\
       {                \
         dummy = i*i;   \
         i++;           \
       }})

/****************************************************************************************/

#define FLAG_LCTRL	0x00000008
#define FLAG_RCTRL	0x00000010
#define FLAG_LALT	0x00000020
#define FLAG_RALT	0x00000040
#define	FLAG_LSHIFT	0x00000080
#define FLAG_RSHIFT	0x00000100
#define	FLAG_LMETA	0x00000200
#define FLAG_RMETA	0x00000400
#define FLAG_DEL    	0x00000800

#warning Old place of kbd_reset

/****************************************************************************************/

void kbd_updateleds(ULONG kbd_keystate)
{
    UBYTE key,info;
    kbd_write_output_w(KBD_OUTCMD_SET_LEDS);
    WaitForInput;
    key=kbd_read_input();
    kbd_write_output_w(kbd_keystate & 0x07);
    WaitForInput;
    key=kbd_read_input();
}

/****************************************************************************************/

#undef SysBase
#define SysBase (hw->sysBase)

/****************************************************************************************/

void kbd_keyint(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    struct kbd_data *data = (struct kbd_data *)irq->h_Data;
    ULONG   	    kbd_keystate = data->kbd_keystate;
    UBYTE   	    keycode;        /* Recent Keycode get */
    UBYTE   	    downkeycode;
    UBYTE   	    releaseflag;
    UBYTE   	    info = 0;       /* Data from info reg */
    UWORD   	    event;          /* Event sent to handleevent method */
    WORD    	    amigacode;
    WORD    	    work = 10000;

    D(bug("ki: {\n")); 
    for(; ((info = kbd_read_status()) & KBD_STATUS_OBF) && work; work--)
    {
    	/* data from information port */
    	if (info & KBD_STATUS_MOUSE_OBF)
	{
	    /*
	    ** Data from PS/2 mouse. Hopefully this gets through to mouse interrupt
	    ** if we break out of while loop here :-\
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

    	if ((keycode == KBD_REPLY_ACK) || (keycode == KBD_REPLY_RESEND))
	{
	    /* Ignore these */
	    continue;
	}
	
	if ((keycode == 0xE0) || (keycode == 0xE1))
	{
	    /* Extended keycodes: E0 gets followed by one code, E1 by two */
	    data->prev_keycode = keycode;
	    continue;
	}
	
	if ((keycode == 0x00) || (keycode == 0xFF))
	{
	    /* 00 is error. FF is sent by some keyboards -> ignore it. */
	    data->prev_keycode = 0;
	    continue;
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
		    /* lets remember, that we still need third key */
		    data->prev_keycode = 0x1234;
		    continue;
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
		    continue;
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
                kbd_keystate ^= 0x02;	/* Turn Numlock bit on */
                kbd_updateleds(kbd_keystate);
                break;
 
            case K_Scroll_Lock:
                kbd_keystate ^= 0x01;	/* Turn Scrolllock bit on */
                kbd_updateleds(kbd_keystate);
                break;

            case K_CapsLock:
                kbd_keystate ^= 0x04;	/* Turn Capslock bit on */
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
                kbd_keystate |= FLAG_RMETA;
                break;

            case (K_RMeta | 0x80):
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

        if ( ((kbd_keystate & (FLAG_LCTRL|FLAG_LMETA|FLAG_RMETA)) == (FLAG_LCTRL|FLAG_LMETA|FLAG_RMETA)) ||
	     ((kbd_keystate & (FLAG_LCTRL|FLAG_LALT|FLAG_DEL)) == (FLAG_LCTRL|FLAG_LALT|FLAG_DEL)) )
	{
	    amigacode = 0x78; /* Reset */
	}

    	D(bug("ki: amigacode %d (%x) last %d (%x)\n", amigacode, amigacode, data->prev_amigacode, data->prev_amigacode));

        /* Update keystate */
        data->kbd_keystate = kbd_keystate;

#if 0
        if (amigacode == 0x78)    // Reset request
            ColdReboot();
#endif

    	if (amigacode == NOKEY) continue;

    	if (amigacode == data->prev_amigacode)
	{
	    /*
	    ** Must be a repeated key. Ignore it, because we have our
	    ** own kbd repeating in input.device
	    */	    
	    continue;
	}

	data->prev_amigacode = amigacode;
	
	D(bug("ki: ********************* c %d (%x)\n", amigacode, amigacode));

        /* Pass the code to handler */
        data->kbd_callback(data->callbackdata, amigacode);

    } /* for(; ((info = kbd_read_status()) & KBD_STATUS_OBF) && work; work--) */

    if (!work)
    {
        D(bug("kbd.hidd: controller jammed (0x%02X).\n", info));
    }
    
    //return 0;	/* Enable processing other intServers */

    D(bug("ki: }\n"));

    return;
}

/****************************************************************************************/

#undef SysBase
#define SysBase (*(struct ExecBase **)4UL)

#warning This should go somewhere higher but D(bug()) is not possible there
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

    kbd_write_command_w(KBD_CTRLCMD_SELF_TEST); /* Initialize and test keyboard */

    if (kbd_wait_for_input() != 0x55)
    {
        return FALSE;
    }

    kbd_write_command_w(KBD_CTRLCMD_KBD_TEST);
    
    if (kbd_wait_for_input() != 0)
    {
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
            return FALSE;
	    
    } while(1);

    if (kbd_wait_for_input() != KBD_REPLY_POR)
        return FALSE;
   
    do
    {
        kbd_write_output_w(KBD_OUTCMD_DISABLE);
        status = kbd_wait_for_input();
	
        if (status == KBD_REPLY_ACK)
            break;
	    
        if (status != KBD_REPLY_RESEND)
            return FALSE;
	    
    } while (1);

    kbd_write_command_w(KBD_CTRLCMD_WRITE_MODE);  /* Write mode */

#if 0
    kbd_write_output_w( KBD_MODE_KCC 	| // set paramters: scan code to pc conversion, 
    		            KBD_MODE_KBD_INT 	| //                enable mouse and keyboard,
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
