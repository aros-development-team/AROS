/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: The main keyboard class.
    Lang: English.
*/

/*
 * Define the following to have a combined mouse and keyboard hidd!
 * Has to come before include kbd.h!
 */
#define MOUSE_ACTIVE 0

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

#include "../../speaker.h"

#define DEBUG 1
#include <aros/debug.h>

void kbd_keyint(HIDDT_IRQ_Handler *, HIDDT_IRQ_HwInfo *);

void kbd_updateleds();
int kbd_reset(void);
int poll_data(void);


#ifdef MOUSE_ACTIVE
int mouse_reset(void);
#endif

long pckey2hidd (ULONG event);

static OOP_AttrBase HiddKbdAB;

static struct abdescr attrbases[] =
{
    { IID_Hidd_Kbd, &HiddKbdAB },
    { NULL, NULL }
};

struct kbd_data
{
    VOID (*kbd_callback)(APTR, UWORD);
    APTR callbackdata;
};

static struct _keytable
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
    D(bug("tstate: %p, tag=%x\n", tstate, tstate->ti_Tag));	
    while ((tag = NextTagItem(&tstate)))
    {
        ULONG idx;
	
        D(bug("Got tag %d, data %x\n", tag->ti_Tag, tag->ti_Data));
	    
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
            irq->h_Data         = (APTR)o;

            HIDD_IRQ_AddHandler(XSD(cl)->irqhidd, irq, vHidd_IRQ_Keyboard);

            Disable();
            kbd_reset();		/* Reset the keyboard */
            Enable();
            
            kbd_updateleds();
            ObtainSemaphore(&XSD(cl)->sema);
            XSD(cl)->kbdhidd = o;
            ReleaseSemaphore(&XSD(cl)->sema);

        }
        
#if MOUSE_ACTIVE
        if ((XSD(cl)->irqhidd_mouse = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL)))
        {
            /* install mouse irq handler */

            HIDDT_IRQ_Handler   *irq_mouse;

            irq_mouse = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR|MEMF_PUBLIC);

            if (!irq_mouse)
            {
                kprintf("ERROR: Cannot install PS/2 Mouse\n");
                Alert( AT_DeadEnd | AN_IntrMem );
            }
	        
            irq_mouse->h_Node.ln_Pri  = 127;		/* Set the highest pri */
            irq_mouse->h_Node.ln_Name = "Mouse class irq";
#warning Uses same routine as keyboard for irq handling!
            irq_mouse->h_Code         = kbd_keyint;     /* Use same routine since uses same hw! */
            irq_mouse->h_Data         = (APTR)o;

            HIDD_IRQ_AddHandler(XSD(cl)->irqhidd_mouse, irq_mouse, vHidd_IRQ_Mouse);

            D(bug("Installed mouse irq handler!\n"));

            Disable();
            mouse_reset();
            Enable();

        }
#endif

  /*
   * Please leave the following lines here.
   * It only works when they are here. I don't know why, but
   * I'll find a way to avoid them...
   */

#if 0
  Sound(400,100000000);
  D(bug("reseting keyboard!\n"));
  kbd_reset();
#endif

#warning There is an endless loop here for testing purposes.
#if 0
  while (1)
  {

  }
#endif
    }
    ReturnPtr("Kbd::New", OOP_Object *, o);
}

/***** X11Kbd::HandleEvent()  ***************************************/

static VOID kbd_handleevent(OOP_Class *cl, OOP_Object *o, struct pHidd_Kbd_HandleEvent *msg)
{
    struct kbd_data * data;
    UWORD key;

    EnterFunc(bug("kbd_handleevent()\n"));
    data = OOP_INST_DATA(cl, o);
    key = pckey2hidd(msg->event);
    D(bug("%lx ",key));
    if (key == 0x78)	// Reset request
        ColdReboot();
    if (data->kbd_callback)
    {
       data->kbd_callback(data->callbackdata, key);
    }
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
    
            if (obtainattrbases(attrbases, OOPBase))
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
    EnterFunc(bug("free_kbdclass(xsd=%p)\n", xsd));

    if(xsd)
    {
        OOP_RemoveClass(xsd->kbdclass);

        if(xsd->kbdclass) OOP_DisposeObject((OOP_Object *) xsd->kbdclass);
        xsd->kbdclass = NULL;

        releaseattrbases(attrbases, OOPBase);
    }
    ReturnVoid("free_kbdclass");
}

/************************* Keyboard Interrupt ****************************/

#define inb(port) \
    ({  char __value;   \
    __asm__ __volatile__ ("inb $" #port ",%%al":"=a"(__value)); \
    __value;    })

#define outb(val,port)      \
    ({  char __value=(val); \
    __asm__ __volatile__ ("outb %%al,$" #port::"a"(__value)); })

#define WaitForInput        \
    ({ int i = 0,dummy;     \
       do                   \
       {                    \
        info=inb(0x64);     \
       } while((info & 0x01)); \
       while (i < 1000000)     \
       {                \
         dummy = i*i;   \
         i++;           \
       }})

#define WaitForOutput   \
    ({  do              \
        {               \
            info=inb(0x64);     \
        } while(info & 0x02);   \
        inb(0x60);              \
    })


ULONG kbd_keystate=0;
#define LCTRL	0x00000008
#define RCTRL	0x00000010
#define LALT	0x00000020
#define RALT	0x00000040
#define	LSHIFT	0x00000080
#define RSHIFT	0x00000100
#define	LMETA	0x00000200
#define RMETA	0x00000400


#warning Old place of kbd_reset

void kbd_updateleds()
{
    UBYTE key,info;
    WaitForOutput;
    outb(0xed,0x60);	/* SetLeds command */
    WaitForInput;
    key=inb(0x60);
    WaitForOutput;
    outb(kbd_keystate & 0x07,0x60);
    WaitForInput;
    key=inb(0x60);
}

#undef SysBase
#define SysBase (hw->sysBase)

static UBYTE mouse_data[3];
static UBYTE mouse_collected_bytes = 0;
static UBYTE expected_mouse_acks = 0;

void kbd_keyint(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    UBYTE   keycode;        /* Recent Keycode get */
    UBYTE   info;           /* Data from info reg */
    UWORD   event;          /* Event sent to handleevent method */
    static UWORD le;        /* Last event used to prevent from some reps */    
    info = inb(0x64);

    while ((info & 0x01))               /* data from information port */
    {
      if (!(info & 0x20))               /* If bit 5 set data from mouse. Otherwise keyboard */
      {
        keycode=poll_data();
        if (keycode==0xe0)              /* Special key */
        {
            keycode=poll_data();
            if (keycode==0x2a)          /* Shift modifier - we can skip it */
            {
                keycode=poll_data();
                keycode=poll_data();
            }
            event=0x4000|keycode;   /* set event to send */
            if (event==0x40aa)      /* If you get something like this... */
            {                       /* Special Shift up.... */
                //return -1;        /* Treat it as NoKey and don't let */
                info = inb(0x64);
                continue;
            }                       /* Other interrupts see it */
        }
        else if (keycode==0xe1)     /* Pause key */
        {
            keycode=poll_data();    /* Read next 5 bytes from keyboard */
            keycode=poll_data();    /* This is hack, but I know there is */
            keycode=poll_data();    /* Only one key which starts with */
            keycode=poll_data();    /* 0xe1 code */
            keycode=poll_data();
            event=K_Pause;
        }
        else if (keycode==0xfa)
        {
            //return -1;		/* Treat it as NoKey */
            D(bug("!!! Got Keyboard ACK!!!\n"));
            info = inb(0x64);
            continue;
        }
        else if (keycode==0xfe)
        {
            /* supposed to resend the command */
            D(bug("!!! Got Resend command from Keyboard!\n"));
            info = inb(0x64);
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
                kbd_updateleds();
                break;
            case K_Scroll_Lock:
                kbd_keystate^=0x01;	/* Turn Scrolllock bit on */
                kbd_updateleds();
                break;
            case K_CapsLock:
                kbd_keystate^=0x04;	/* Turn Capslock bit on */
                kbd_updateleds();
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
        le=event;
        if ((kbd_keystate & (LCTRL|LMETA|RMETA))==(LCTRL|LMETA|RMETA))
            event=K_ResetRequest;
        if ((event & 0x7f7f)==(K_Scroll_Lock & 0x7f)) event|=0x4000;
        Hidd_Kbd_HandleEvent((OOP_Object *)irq->h_Data,(ULONG) event);
      }
#if MOUSE_ACTIVE
      else
      {
        UBYTE mousecode = inb(0x60);
        if (0xfa == mousecode) 
        {
          D(bug("                             Got a mouse ACK!\n"));
          if (expected_mouse_acks) {
            expected_mouse_acks--;
          }
        }
        else
        {
          expected_mouse_acks = 0;
          mouse_data[mouse_collected_bytes] = mousecode;
          
          if (0 == (mouse_data[0] & 8))
            mouse_collected_bytes = 0;
          else
          {
            mouse_collected_bytes++;
            if (3 == mouse_collected_bytes) 
            {
              mouse_collected_bytes = 0;
              /*
               * Let's see whether these data can be right...
               *
               *  D7 D6 D5 D4 D3 D2 D1 D0
               *  YV XV YS X2  1  M  R  L
               *  X7 .  .  .  .  .  .  X1   (X, signed)
               *  Y7 .  .  .  .  .  .  Y1   (Y, signed)
               *
               *  YV,XV : over flow in x/y direction
               *  XS,YS : represents sign of X and Y
               *  X,Y   : displacement in x and y direction.
               *  X and Y are signed, XS, YS are there to double check the
               *  sign and correctnes of the collected data (?).
               *
               *  http://www.hut.fi/~then/mytexts/mouse.htm
               */
              if ( (( (mouse_data[0] & 0x10) && (char)mouse_data[1] <  0) ||
                    (!(mouse_data[0] & 0x10) && (char)mouse_data[1] >= 0)   ) &&
                   (( (mouse_data[0] & 0x20) && (char)mouse_data[2] <  0) ||
                    (!(mouse_data[0] & 0x20) && (char)mouse_data[2] >= 0    )))
              {
                D(bug("Got the following: 1. byte: 0x%x, dx=%d, dy=%d\n",
                      mouse_data[0],
                      mouse_data[1],
                      mouse_data[2]));
                /*
                 * Pass them on to the handler!
                 */
              
#warning The mouse data *seem* right at this point and can now be treated!
              }
            }
          }
        }
      }
#endif
      info = inb(0x64);
    } /* while data can be read */

    //return 0;	/* Enable processing other intServers */
    return;
}

#ifdef SysBase
#undef SysBase
#endif /* SysBase */
#define SysBase (*(struct ExecBase **)4UL)

long pckey2hidd (ULONG event)
{
    long result;
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
            ReturnInt ("xk2h", long, result);
        }
    }
    D(bug("pk2h: Passing PC keycode\n", event & 0xffff));

    result = event & 0xffff;
    result |= KeyUpFlag;
    ReturnInt ("xk2h", long, result);
}

#warning This should go somewhere higher but D(bug()) is not possible there

int poll_data(void)
{
    int i;
    unsigned char stat;

    for(i = 100000; i; i--)
    {
        stat = inb(0x64);
        if(stat & 0x01)
        {
            unsigned char c;

            TimeDelay(0,0,8);   /* !!! I don't really know if this alib function has any effect on native-i386 yet */
            c = inb(0x60);
            return(c);
        }
    }
    return(-1);
}

/*
 * Please leave this routine as is for now.
 * It works and that is all that matters right now.
 */
int kbd_reset(void)
{
    UBYTE retval, status;
    UBYTE key,info;

    poll_data();		/* Empty keys queue */

    WaitForOutput;		/* Disable mouse interface */
    outb(0xa7, 0x64);

    WaitForOutput;
    outb(0xaa,0x64);	/* Initialize and test keyboard */

    retval = (UBYTE)poll_data();
    if (retval != 0x55)
    {
      D(bug("Error! Got reset return value %x.\n",retval));
      return FALSE;
    }


    WaitForOutput;
    outb(0xae,0x64);    /* enable keyboard */

    D(bug("Keyboard enabled!\n"));

    WaitForOutput;
    outb(0x60, 0x64);  // Write mode
    WaitForOutput;
    outb(0x47, 0x60);  // set paramters: scan code to pc conversion, 
                       //                enable mouse and keyboard,
                       //                enable IRQ 1 & 12.

    D(bug("Successfully reset keyboard!\n"));

    return TRUE;
}

#ifdef MOUSE_ACTIVE

/*
 * Please leave this routine as is for now.
 * It works and that is all that matters right now.
 */
int mouse_reset(void)
{
    UBYTE info;
    UBYTE retval;

    WaitForOutput;
    outb(0x60, 0x64);	/* write mode */
    WaitForOutput;
    outb(0x74, 0x60);   /*  */
    WaitForOutput;
    WaitForInput;
    retval = inb(0x60);
//    D(bug("Return from disabling all irqs: 0x%x\n",retval));
    

    WaitForOutput;
    outb(0xa8, 0x64);   /* enable mouse interface */


    WaitForOutput;
    outb(0xa9,0x64);	/* Initialize and test mouse interface */
    WaitForInput;
    retval = inb(0x60);
    if (retval != 0x00)
    {
      D(bug("Error! (1) Got return value %x from mouse interface test.\n",retval));
//      return FALSE;
    }

    WaitForOutput;
    outb(0xa8, 0x64);   /* enable mouse interface */


    WaitForOutput;
    outb(0xd4,0x64);	/* write to mouse */
    WaitForOutput;
    outb(0xff,0x60);	/* reset mouse */
    WaitForOutput;
    WaitForInput;
    retval = inb(0x60);
//    D(bug("Return value from reset mouse: 0x%x\n",retval));
    
    WaitForOutput;
    outb(0xd4,0x64);	/* write to mouse */
    WaitForOutput;
    outb(0xf4,0x60);	/* enable mouse device */
    WaitForOutput;
    WaitForInput;
    retval = inb(0x60);
//    D(bug("Return value from enable mouse: 0x%x\n",retval));

    do
    {
      WaitForOutput;
      outb(0xd4,0x64);	/* write to mouse */
      WaitForOutput;
      outb(0xf3,0x60);	/* set samples */
      WaitForInput;
      WaitForOutput;
      outb(0xd4,0x64);	/* write to mouse */
      WaitForOutput;
      outb(50,0x60);	/* 50 samples/s */
      WaitForOutput;
      WaitForInput;
      retval = inb(0x60);
//      D(bug("Return value from setting mouse samples: 0x%x\n",retval));
    }
    while (0xfe == retval);


    do
    {
      WaitForOutput;
      outb(0xd4,0x64);	/* write to mouse */
      WaitForOutput;
      outb(0xe7,0x60);	/* set 2:1 scaling */
      WaitForOutput;
      WaitForInput;
      retval = inb(0x60);
//      D(bug("Return value from setting mouse scaling 2:1: 0x%x\n",retval));
    }
    while (0xfe == retval);


    do
    {
      WaitForOutput;
      outb(0xd4,0x64);	/* write to mouse */
      WaitForOutput;
      outb(0xea,0x60);	/* stream mode on mouse device */
      WaitForOutput;
      WaitForInput;
      retval = inb(0x60);
//      D(bug("Return value from stream mode on mouse: 0x%x\n",retval));
    }
    while (0xfe == retval);

    do
    {
      WaitForOutput;
      outb(0xd4,0x64);	/* write to mouse */
      WaitForOutput;
      outb(0xf4,0x60);	/* enable mouse device */
      WaitForOutput;
      WaitForInput;
      retval = inb(0x60);
//      D(bug("Return value from enable mouse: 0x%x\n",retval));
    }
    while (0xfe == retval);


    WaitForOutput;
    outb(0x47, 0x64);     /* enable mouse,keyboard & irqs */
    WaitForOutput;

    expected_mouse_acks = 1;

    D(bug("Initialized PS/2 mouse!\n"));
}

#endif
