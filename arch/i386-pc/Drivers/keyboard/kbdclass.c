/*
    (C) 1999-2001 AROS - The Amiga Research OS
    $Id$

    Desc: The main keyboard class.
    Lang: English.
*/

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
#include <aros/machine.h>
#include <aros/asmcall.h>

#include <hardware/custom.h>

#include <devices/inputevent.h>

#include "kbd.h"
#include "keys.h"

#define DEBUG 1 
#include <aros/debug.h>

/* Predefinitions */

void kbd_keyint(HIDDT_IRQ_Handler *, HIDDT_IRQ_HwInfo *);

void kbd_updateleds();
int kbd_reset(void);

unsigned char handle_kbd_event(void);
void kb_wait(void);
void kbd_write_cmd(int cmd);
void aux_write_ack(int val);
void kbd_write_output_w(int data);
void kbd_write_command_w(int data);
void mouse_usleep(ULONG);
void kbd_clear_input(void);
int  kbd_wait_for_input(void);
int  kbd_read_data(void);

/* End of predefinitions */

/* !!!!!!!!!! Remove all .data from file
 */
#define HiddKbdAB   (XSD(cl)->hiddKbdAB)
/*
static OOP_AttrBase HiddKbdAB;

static struct abdescr attrbases[] =
{
    { IID_Hidd_Kbd, &HiddKbdAB },
    { NULL, NULL }
};
*/

struct kbd_data
{
    VOID (*kbd_callback)(APTR, UWORD);
    APTR callbackdata;

    ULONG kbd_keystate;     /* State of special keys */
    UWORD le;               /* Last event */
};

static const __attribute__((section(".text"))) struct _keytable
{
    ULONG  keysym;
    ULONG  hiddcode;
}
keytable[] =
{
    {K_Enter,       0x44 },
    {K_Right,       0x4e },
    {K_Up,          0x4c },
    {K_Left,        0x4f },
    {K_Down,        0x4d },
    {K_KP_Enter,    0x43 },
    {K_KP_Decimal,  0x3c },
    {K_KP_Sub,      0x4a },
    {K_KP_Add,      0x5e },
    {K_KP_Multiply, 0x5d },
    {K_KP_Divide,   0x5c },
    {K_KP_0,        0x0f },
    {K_KP_1,        0x1d },
    {K_KP_2,        0x1e },
    {K_KP_3,        0x1f },
    {K_KP_4,        0x2d },
    {K_KP_5,        0x2e },
    {K_KP_6,        0x2f },
    {K_KP_7,        0x3d },
    {K_KP_8,        0x3e },
    {K_KP_9,        0x3f },
    
    {K_F1,          0x50 },
    {K_F2,          0x51 },
    {K_F3,          0x52 },
    {K_F4,          0x53 },
    {K_F5,          0x54 },
    {K_F6,          0x55 },
    {K_F7,          0x56 },
    {K_F8,          0x57 },
    {K_F9,          0x58 },
    {K_F10,         0x59 },
    {K_F11,         0x66 },     /* LAMIGA */
    {K_F12,         0x67 },     /* RAMIGA */
    {K_Home,        0x5f },     /* HELP */
    
    {K_A,           0x20 },
    {K_B,           0x35 },
    {K_C,           0x33 },
    {K_D,           0x22 },
    {K_E,           0x12 },
    {K_F,           0x23 },
    {K_G,           0x24 },
    {K_H,           0x25 },
    {K_I,           0x17 },
    {K_J,           0x26 },
    {K_K,           0x27 },
    {K_L,           0x28 },
    {K_M,           0x37 },
    {K_N,           0x36 },
    {K_O,           0x18 },
    {K_P,           0x19 },
    {K_Q,           0x10 },
    {K_R,           0x13 },
    {K_S,           0x21 },
    {K_T,           0x14 },
    {K_U,           0x16 },
    {K_V,           0x34 },
    {K_W,           0x11 },
    {K_X,           0x32 },
    {K_Y,           0x15 },
    {K_Z,           0x31 },
    
    {K_1,           0x01 },
    {K_2,           0x02 },
    {K_3,           0x03 },
    {K_4,           0x04 },    
    {K_5,           0x05 },
    {K_6,           0x06 },
    {K_7,           0x07 },
    {K_8,           0x08 },
    {K_9,           0x09 },
    {K_0,           0x0A },
    
    {K_Period,      0x39 },
    {K_Comma,       0x38 },
                        
    {K_Backspace,   0x41 },
    {K_Del,         0x46 },
    {K_Space,       0x40 },
    {K_LShift,      0x60 },
    {K_RShift,      0x61 },
    {K_LAlt,        0x64 },
    {K_RAlt,        0x65 },
    {K_LCtrl,       0x63 },
    {K_RCtrl,       0x63 },
    {K_LMeta,       0x66 },	/* Left Win key = LAmi */
    {K_RMeta,       0x67 },	/* Right Win key = RAmi */
    {K_Escape,      0x45 },
    {K_Tab,         0x42 },
    /* New stuff */
    {K_CapsLock,    0x62 },
    {K_LBracket,    0x1a },
    {K_RBracket,    0x1b },
    {K_Semicolon,   0x29 },
    {K_Slash,       0x3a },
    {K_BackSlash,   0x0d },
    {K_Quote,       0x2a },
    {K_BackQuote,   0x00 },
    {K_Minus,       0x0b },
    {K_Equal,       0x0c },

    {K_ResetRequest,0x78 },
    {0, -1 }
};

/***** Kbd::New()  ***************************************/
static OOP_Object * kbd_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL has_kbd_hidd = FALSE;
    struct TagItem *tag, *tstate;
    APTR callback = NULL;
    APTR callbackdata = NULL;
    
    EnterFunc(bug("Kbd::New()\n"));
 
    ObtainSemaphoreShared( &XSD(cl)->sema);

    if (XSD(cl)->kbdhidd)
    	has_kbd_hidd = TRUE;

    ReleaseSemaphore( &XSD(cl)->sema);
 
    if (has_kbd_hidd) /* Cannot open twice */
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

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct kbd_data *data = OOP_INST_DATA(cl, o);
        
        data->kbd_callback = (VOID (*)(APTR, UWORD))callback;
        data->callbackdata = callbackdata;

        /* Get irq.hidd */

        if ((XSD(cl)->irqhidd = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL)))
        {
            /* Install keyboard interrupt */

            HIDDT_IRQ_Handler   *irq;

            irq = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR|MEMF_PUBLIC);

            if (!irq)
            {
                kprintf("ERROR: Cannot install Keyboard\n");
                Alert( AT_DeadEnd | AN_IntrMem );
            }
	        
            irq->h_Node.ln_Pri  = 127;		/* Set the highest pri */
            irq->h_Node.ln_Name = "Keyboard class irq";
            irq->h_Code         = kbd_keyint;
            irq->h_Data         = (APTR)data;

//kprintf("kbdclass: calling disable/kbd_clear_input/kbd_reset/enable()\n");
            Disable();
	    kbd_clear_input();
	    kbd_reset();		/* Reset the keyboard */
//kprintf("kbdclass: updateleds \n");
            kbd_updateleds(0);
            Enable();

//kprintf("kbdclass:adding irqhandler \n");
            
            HIDD_IRQ_AddHandler(XSD(cl)->irqhidd, irq, vHidd_IRQ_Keyboard);
            ObtainSemaphore(&XSD(cl)->sema);
            XSD(cl)->kbdhidd = o;
            ReleaseSemaphore(&XSD(cl)->sema);

        }
    }
//kprintf("kbdclass: exit\n");
    ReturnPtr("Kbd::New", OOP_Object *, o);
}

/***** X11Kbd::HandleEvent()  ***************************************/

static VOID kbd_handleevent(OOP_Class *cl, OOP_Object *o, struct pHidd_Kbd_HandleEvent *msg)
{
    struct kbd_data * data;

    EnterFunc(bug("kbd_handleevent()\n"));

    data = OOP_INST_DATA(cl, o);
    
    ReturnVoid("Kbd::HandleEvent");
}

#undef XSD
#define XSD(cl) xsd

/********************  init_kbdclass()  *********************************/

#define NUM_ROOT_METHODS 1
#define NUM_KBD_METHODS 1

OOP_Class *init_kbdclass (struct kbd_staticdata *xsd)
{
    OOP_Class *cl = NULL;
    
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd_Kbd,  &xsd->hiddKbdAB},
        {NULL, NULL }
    };
    
    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
        {OOP_METHODDEF(kbd_new),            moRoot_New},
        {NULL, 0UL}
    };
    
    struct OOP_MethodDescr kbdhidd_descr[NUM_KBD_METHODS + 1] = 
    {
        {OOP_METHODDEF(kbd_handleevent),    moHidd_Kbd_HandleEvent},
        {NULL, 0UL}
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {root_descr,    IID_Root,           NUM_ROOT_METHODS},
        {kbdhidd_descr, IID_Hidd_HwKbd,     NUM_KBD_METHODS},
        {NULL, NULL, 0}
    };
    
    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        { aMeta_SuperID,                (IPTR)CLID_Hidd },
        { aMeta_InterfaceDescr,         (IPTR)ifdescr},
        { aMeta_InstSize,               (IPTR)sizeof (struct kbd_data) },
        { aMeta_ID,                     (IPTR)CLID_Hidd_HwKbd },
        {TAG_DONE, 0UL}
    };

    EnterFunc(bug("KbdHiddClass init\n"));
    
    if (MetaAttrBase)
    {
        cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            cl->UserData = (APTR)xsd;
            xsd->kbdclass = cl;
    
            if (OOP_ObtainAttrBases(attrbases))
            {
                D(bug("KbdHiddClass ok\n"));

                OOP_AddClass(cl);
            }
            else
            {
                free_kbdclass(xsd);
                cl = NULL;
            }
        }
        /* Don't need this anymore */
        OOP_ReleaseAttrBase(IID_Meta);
    }
    ReturnPtr("init_kbdclass", OOP_Class *, cl);
}

/*************** free_kbdclass()  **********************************/
VOID free_kbdclass(struct kbd_staticdata *xsd)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd_Kbd,  &xsd->hiddKbdAB},
        {NULL, NULL }
    };
    
    EnterFunc(bug("free_kbdclass(xsd=%p)\n", xsd));

    if(xsd)
    {
        OOP_RemoveClass(xsd->kbdclass);

        if(xsd->kbdclass) OOP_DisposeObject((OOP_Object *) xsd->kbdclass);
        xsd->kbdclass = NULL;

        OOP_ReleaseAttrBases(attrbases);
    }
    ReturnVoid("free_kbdclass");
}

/************************* Keyboard Interrupt ****************************/

#define WaitForInput        		\
    ({ int i = 0,dummy;     		\
       do                   		\
       {                    		\
        info = kbd_read_status();     	\
       } while((info & KBD_STATUS_OBF));\
       while (i < 1000000)     		\
       {                \
         dummy = i*i;   \
         i++;           \
       }})

#define LCTRL	0x00000008
#define RCTRL	0x00000010
#define LALT	0x00000020
#define RALT	0x00000040
#define	LSHIFT	0x00000080
#define RSHIFT	0x00000100
#define	LMETA	0x00000200
#define RMETA	0x00000400

#warning Old place of kbd_reset

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

#undef SysBase
#define SysBase (hw->sysBase)

void kbd_keyint(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    UBYTE   keycode;        /* Recent Keycode get */
    UBYTE   info;           /* Data from info reg */
    UWORD   event;          /* Event sent to handleevent method */

    struct kbd_data *data = (struct kbd_data *)irq->h_Data;
    ULONG kbd_keystate = data->kbd_keystate;
    UWORD le = data->le;
    
    unsigned int work = 10000;

//kprintf("ki: {\n");    
    info = kbd_read_status();

    while ((info & KBD_STATUS_OBF))     	/* data from information port */
    {
        /* Ignore errors and messages for mouse */
        if (!(info & (KBD_STATUS_GTO | KBD_STATUS_PERR | KBD_STATUS_MOUSE_OBF)))
        {
            keycode = kbd_read_input();
	
          if (keycode==0xe0)              /* Special key */
          {
	    keycode = kbd_wait_for_input();
            if (keycode==0x2a)          /* Shift modifier - we can skip it */
            {
		keycode = kbd_wait_for_input();
		keycode = kbd_wait_for_input();
            }
            event=0x4000|keycode;   /* set event to send */
            if (event==0x40aa)      /* If you get something like this... */
            {                       /* Special Shift up.... */
                //return -1;        /* Treat it as NoKey and don't let */
                info = kbd_read_status();
                continue;
            }                       /* Other interrupts see it */
        }
        else if (keycode==0xe1)     /* Pause key */
        {
	    keycode = kbd_wait_for_input();
	    keycode = kbd_wait_for_input();
	    keycode = kbd_wait_for_input();
	    keycode = kbd_wait_for_input();
	    keycode = kbd_wait_for_input();
	    
            event=K_Pause;
        }
        else if (keycode==KBD_REPLY_ACK)
        {
            //return -1;		/* Treat it as NoKey */
            D(bug("Kbd: Gottt Keyboard ACK!!!\n"));
            info = kbd_read_status();
            continue;
        }
        else if (keycode==KBD_REPLY_RESEND)
        {
            /* supposed to resend the command */
            D(bug("Kbd: Got Resend command from Keyboard!\n"));
            info = kbd_read_status();
            continue;
        }	
        else event=keycode;
        if ((event==le) && (
            le==K_KP_Numl || le==K_Scroll_Lock || le==K_CapsLock ||
    	    le==K_LShift || le==K_RShift || le==K_LCtrl || le==K_RCtrl ||
            le==K_LAlt || le==K_RAlt || le==K_LMeta || le==K_RMeta))
        {
            //return -1;	/* Do not repeat shift pressed or something like this */
            continue;
        }		/* Just forgot about interrupt */
        switch(event)
        {
            case K_KP_Numl:
                kbd_keystate^=0x02;	/* Turn Numlock bit on */
                kbd_updateleds(kbd_keystate);
                break;
            case K_Scroll_Lock:
                kbd_keystate^=0x01;	/* Turn Scrolllock bit on */
                kbd_updateleds(kbd_keystate);
                break;
            case K_CapsLock:
                kbd_keystate^=0x04;	/* Turn Capslock bit on */
                kbd_updateleds(kbd_keystate);
                break;
            case K_LShift:
                kbd_keystate|=LSHIFT;
                break;
            case (K_LShift|0x80):
                kbd_keystate&=~LSHIFT;
                break;
            case K_RShift:
                kbd_keystate|=RSHIFT;
                break;
            case (K_RShift|0x80):
                kbd_keystate&=~RSHIFT;
                break;
            case K_LCtrl:
                kbd_keystate|=LCTRL;
                break;
            case (K_LCtrl|0x80):
                kbd_keystate&=~LCTRL;
                break;
            case K_RCtrl:
                kbd_keystate|=RCTRL;
                break;
            case (K_RCtrl|0x80):
                kbd_keystate&=~RCTRL;
                break;
            case K_LMeta:
                kbd_keystate|=LMETA;
                break;
            case (K_LMeta|0x80):
                kbd_keystate&=~LMETA;
                break;
            case K_RMeta:
                kbd_keystate|=RMETA;
                break;
            case (K_RMeta|0x80):
                kbd_keystate&=~RMETA;
                break;
            case K_LAlt:
                kbd_keystate|=LALT;
                break;
            case (K_LAlt|0x80):
                kbd_keystate&=~LALT;
                break;
            case K_RAlt:
                kbd_keystate|=RALT;
                break;
            case (K_RAlt|0x80):
                kbd_keystate&=~RALT;
                break;
        }
        data->le=event;
        if ((kbd_keystate & (LCTRL|LMETA|RMETA))==(LCTRL|LMETA|RMETA))
            event=K_ResetRequest;
        if ((event & 0x7f7f)==(K_Scroll_Lock & 0x7f)) event|=0x4000;

        /* Update keystate */
        data->kbd_keystate = kbd_keystate;
        
        /* Translate code into Amiga-like code */
        {
            long result = -1;
            short t;
            UBYTE KeyUpFlag;

            KeyUpFlag=(UBYTE)event & 0x80;
            event &= ~(0x80);

            for (t=0; keytable[t].hiddcode != -1; t++)
            {
                if (event == keytable[t].keysym)
                {
                    result = keytable[t].hiddcode;
                    result|= KeyUpFlag;
                    break;
                }
            }
            if (result == -1)
            {
                result = event & 0xffff;
                result |= KeyUpFlag;
            }

            if (result == 0x78)    // Reset request
                ColdReboot();
                      
	    //kprintf("ki: c %d (%x)\n", result, result);
	                         
            /* Pass the code to handler */
            data->kbd_callback(data->callbackdata, result);
        }
      } else break;

      info = kbd_read_status();

      /* Protect as from forever loop */
      if (!--work)
      {
        D(bug("kbd.hidd: controller jammed (0x%02X).\n", info));
        break;
      }
    } /* while data can be read */

    //return 0;	/* Enable processing other intServers */
//kprintf("ki: }\n");
    return;
}

#ifdef SysBase
#undef SysBase
#endif /* SysBase */
#define SysBase (*(struct ExecBase **)4UL)

#warning This should go somewhere higher but D(bug()) is not possible there

/*
 * Please leave this routine as is for now.
 * It works and that is all that matters right now.
 */
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
