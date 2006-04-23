/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 hidd handling keypresses.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <dos/dos.h>

#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#define timeval sys_timeval
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <stdio.h>
#undef timeval

#include <hidd/hidd.h>
#include <hidd/keyboard.h>

#include <devices/inputevent.h>

#include <aros/symbolsets.h>

//#define DEBUG 1
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "x11.h"

/****************************************************************************************/

static UBYTE keycode2rawkey[256];
static BOOL  havetable;

long xkey2hidd (XKeyEvent *xk, struct x11_staticdata *xsd);

static OOP_AttrBase HiddKbdAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_Kbd  , &HiddKbdAB    },
    { NULL  	    , NULL  	    }
};

/****************************************************************************************/

static struct _keytable
{
    KeySym keysym;
    WORD   hiddcode;
}
keytable[] =
{
    {XK_Return, 	0x44 },
    {XK_Right,		0x4e },
    {XK_Up,		0x4c },
    {XK_Left,		0x4f },
    {XK_Down,		0x4d },
    {XK_Help,		0x5f },

    {XK_BackSpace,	0x41 },
    {XK_Delete,		0x46 },
    {XK_space,		0x40 },
    {XK_Shift_L,	0x60 },
    {XK_Shift_R,	0x61 },
    {XK_Alt_L,		0x64 },
    {XK_Alt_R,		0x65 },
    {XK_Escape,		0x45 },
    {XK_Tab,		0x42 },

    {XK_F1,		0x50 },
    {XK_F2,		0x51 },
    {XK_F3,		0x52 },
    {XK_F4,		0x53 },
    {XK_F5,		0x54 },
    {XK_F6,		0x55 },
    {XK_F7,		0x56 },
    {XK_F8,		0x57 },
    {XK_F9,		0x58 },
    {XK_F10,		0x59 },

    {XK_F11,		0x4B },	
    {XK_F12,		0x5f },	/* HELP, F12 would be 0x6F */
    {XK_Home,		0x70 },	
    {XK_End,		0x71 },
    {XK_Insert,		0x47 },
    {XK_Prior,		0x48 }, /* PageUP */
    {XK_Next,		0x49 }, /* PageDown */
    {XK_Pause,		0x6e },
    
    {XK_KP_Enter,	0x43 },
    {XK_KP_Subtract,	0x4a },
    {XK_KP_Decimal,	0x3c },
    {XK_KP_Separator,	0x3c },
    {XK_KP_Delete,	0x3c },
    {XK_KP_Add,		0x5e },
    {XK_KP_Subtract,	0x4a },
    {XK_KP_Multiply,	0x5d },
    {XK_KP_Divide,	0x5c },

    {XK_KP_0,		0x0f },
    {XK_KP_Insert,	0x0f },    
    {XK_KP_1,		0x1d },
    {XK_KP_End,		0x1d },   
    {XK_KP_2,		0x1e },
    {XK_KP_Down,	0x1e },
    {XK_KP_3,		0x1f },
    {XK_KP_Page_Down,	0x1f },
    {XK_KP_4,		0x2d },
    {XK_KP_Left,	0x2d },
    {XK_KP_5,		0x2e },
    {XK_KP_Begin,	0x2e },
    {XK_KP_6,		0x2f },
    {XK_KP_Right,	0x2f },
    {XK_KP_7,		0x3d },
    {XK_KP_Home,	0x3d },
    {XK_KP_8,		0x3e },
    {XK_KP_Up,		0x3e },
    {XK_KP_9,		0x3f },
    {XK_KP_Page_Up,	0x3f },
       
    {XK_E,		0x12 },
    {XK_e,		0x12 },
    {XK_R,		0x13 },
    {XK_r,		0x13 },
    {XK_T,		0x14 },
    {XK_t,		0x14 },
    {XK_U,		0x16 },
    {XK_u,		0x16 },
    {XK_I,		0x17 },
    {XK_i,		0x17 },
    {XK_O,		0x18 },
    {XK_o,		0x18 },
    {XK_P,		0x19 },
    {XK_p,		0x19 },

    {XK_S,		0x21 },
    {XK_s,		0x21 },
    {XK_D,		0x22 },
    {XK_d,		0x22 },
    {XK_F,		0x23 },
    {XK_f,		0x23 },
    {XK_G,		0x24 },
    {XK_g,		0x24 },
    {XK_H,		0x25 },
    {XK_h,		0x25 },
    {XK_J,		0x26 },
    {XK_j,		0x26 },
    {XK_K,		0x27 },
    {XK_k,		0x27 },
    {XK_L,		0x28 },
    {XK_l,		0x28 },

    {XK_X,		0x32 },
    {XK_x,		0x32 },
    {XK_c,		0x33 },
    {XK_C,		0x33 },
    {XK_V,		0x34 },
    {XK_v,		0x34 },
    {XK_B,		0x35 },
    {XK_b,		0x35 },
    {XK_N,		0x36 },    
    {XK_n,		0x36 },
    
    {XK_1,		0x01 },
    {XK_2,		0x02 },
    {XK_3,		0x03 },
    {XK_4,		0x04 },    
    {XK_5,		0x05 },
    {XK_6,		0x06 },
    {XK_7,		0x07 },
    {XK_8,		0x08 },
    {XK_9,		0x09 },
    {XK_0,		0x0A },
    {0, - 1 }
};

/****************************************************************************************/

/* English keyboard */
static struct _keytable english_keytable[] =
{    
    {XK_Control_L,	0x63 }, /* left control = control */	
    {XK_Multi_key,	0x63 }, /* right control = control */
    {XK_Super_L,	0x66 },	/* left win = LAMIGA */
    {XK_Super_R,	0x67 },	/* right win = RAMIGA */
    {XK_Meta_L,		0x64 }, /* left Alt = LALT */
    {XK_Mode_switch,	0x65 }, /* right Alt = RALT */
    
    /* Key left of S */
    {XK_A,		0x20 },
    {XK_a,		0x20 },
    
    /* Key right of N */
    {XK_M,		0x37 },
    {XK_m,		0x37 },
    
    /* Key right of TAB */
    {XK_Q,		0x10 },
    {XK_q,		0x10 },
    
    /* Key between T and U */
    {XK_y,		0x15 },
    {XK_Y,		0x15 },
    
    /* Key left of E */
    {XK_W,		0x11 },
    {XK_w,		0x11 },
    
    /* Key left of X */
    {XK_z,		0x31 },
    {XK_Z,		0x31 },

    
    /* Key left of 1 */
    {XK_grave,		0x00 },
    
    /* Keys right of 0 */
    {XK_minus,		0x0B },
    {XK_equal,		0x0C },
    
    /* Keys right of P */
    {XK_bracketleft,	0x1A },
    {XK_bracketright,	0x1B },
    
    /* Keys right of L */
    {XK_semicolon,	0x29 },
    {XK_apostrophe,	0x2A }, 
    {XK_backslash,	0x2B }, /* Third key right of L might not be present */
    
    /* Key right of shift and 2nd left of X (might not be present) */       
    {XK_less,		0x30 }, 
    
    /* Keys 2nd right of N (= usually right of M) */    
    {XK_comma,		0x38 }, 
    {XK_period,		0x39 }, 
    {XK_slash,		0x3A },
        
    {0, -1 }
};

/****************************************************************************************/
                        
/* German keyboard */
static struct _keytable german_keytable[] =
{
    {XK_Control_L,	0x63 }, /* linke STRG = control */	
    {XK_Multi_key,	0x63 }, /* rechte STRG = control */
    {XK_Super_L,	0x66 },	/* Linke Win = LAMIGA */
    {XK_Super_R,	0x67 },	/* Rechte Win = RAMIGA */
    {XK_Meta_L,		0x64 }, /* Linke Alt = LALT */
    {XK_Mode_switch,	0x65 }, /* Alt Gr = RALT */
    
    /* Key left of S */
    {XK_A,		0x20 },
    {XK_a,		0x20 },
    
    /* Key right of N */
    {XK_M,		0x37 },
    {XK_m,		0x37 },
    
    /* Key right of TAB */
    {XK_Q,		0x10 },
    {XK_q,		0x10 },
    
    /* Key between T and U */
    {XK_Z,		0x15 },
    {XK_z,		0x15 },
    
    /* Key left of E */
    {XK_W,		0x11 },
    {XK_w,		0x11 },
    
    /* Key left of X */
    {XK_y,		0x31 },
    {XK_Y,		0x31 },
     
     /* Key left of 1 */
    {XK_asciicircum,	0x00 }, /* Akzent links neben 1 Taste */

    /* Keys right of 0 */
    {XK_equal,		0x0A }, /* = */
    {XK_ssharp,		0x0B }, /* scharfes s */
    {XK_acute,		0x0C }, /* Akzent rechts von scharfem s */
    
    /* Keys right of P */
    {XK_udiaeresis,	0x1A }, /* Umlaut u */
    {XK_Udiaeresis,	0x1A },
    {XK_plus,		0x1B }, /* + */

    /* Keys right of L */    
    {XK_odiaeresis,	0x29 }, /* Umlaut o */
    {XK_Odiaeresis,	0x29 },
    {XK_adiaeresis,	0x2A }, /* Umlaut a */
    {XK_Adiaeresis,	0x2A },
    {XK_numbersign,	0x2B }, /* # */

    /* Key right of shift and 2nd left of X (might not be present) */        
    {XK_less,		0x30 }, /* < */

    /* Keys 2nd right of N (= usually right of M) */       
    {XK_comma,		0x38 }, /* Komma */
    {XK_period,		0x39 }, /* Punkt */
    {XK_minus,		0x3A }, /* Minus */
        
    {0, -1 }
};

/* Itialian keyboard */
static struct _keytable italian_keytable[] =
{
    {XK_Control_L,	0x63 }, /* left CTRL = control */	
    {XK_Multi_key,	0x63 }, /* right CTRL = control */
    {XK_Super_L,	0x66 },	/* left win = LAMIGA */
    {XK_Super_R,	0x67 },	/* right win = RAMIGA */
    {XK_Meta_L,		0x64 }, /* left alt = LALT */
    {XK_Mode_switch,	0x65 }, /* right alt = RALT */


    /* Key left of S */
    {XK_A,		0x20 },
    {XK_a, 		0x20 },
    
    /* Key right of N */
    {XK_M,		0x37 },
    {XK_m,		0x37 },
    
    /* Key right of TAB */
    {XK_Q,		0x10 },
    {XK_q,		0x10 },
    
    /* Key between T and U */
    {XK_y,		0x15 },
    {XK_Y,		0x15 },
    
    /* Key left of E */
    {XK_W,		0x11 },
    {XK_w,		0x11 },
    
    /* Key left of X */
    {XK_z,		0x31 },
    {XK_Z,		0x31 },
    
    
    /* Key left of 1 */
    {XK_backslash,	0x00 }, 

    /* Keys right of 0 */
    {XK_apostrophe,	0x0B },
    {XK_Igrave,		0x0C }, 
    {XK_igrave,		0x0C },

    /* Keys right of P */        
    {XK_Egrave,		0x1A },
    {XK_egrave,		0x1A },
    {XK_plus,		0x1B }, /* + */

    /* Keys right of L */    
    {XK_Ograve,		0x29 },
    {XK_ograve,		0x29 },
    {XK_Agrave,		0x2A },
    {XK_agrave,		0x2A },
    {XK_Ugrave,		0x2B }, /* Third key right of L might not be present */
    {XK_ugrave,		0x2B },

    /* Key right of shift and 2nd left of X (might not be present) */        
    {XK_less,		0x30 }, /* < */

    /* Keys 2nd right of N (= usually right of M) */    
    {XK_comma,		0x38 },
    {XK_period,		0x39 }, 
    {XK_minus,		0x3A }, 
        
    {0, -1 }
};

/****************************************************************************************/

#if 0

/* Use this template to create a keytable for your language:

   Do not touch the right values (rawkey numbers). Only change
   the XK's at the left side. To find out the XK_ names (keysym)
   start "xev" and press the key the comment describes (for
   example "Key left of S" in the xev window. In the Shell
   window you will see output like this:
   
   KeyPress event, serial 30, synthetic NO, window 0x5000001,
    root 0x47, subw 0x5000002, time 3410089115, (24,45), root:(28,69),
    state 0x0, keycode 50 (keysym 0xffe1, Shift_L), same_screen YES,
    XLookupString gives 0 characters:  ""  |
                                           |
   This is the keysym name _______________/
  
   So in this case you would have to write  "XK_Shift_L"

   Check all keys, not just the ones with "XK_????"!!!
*/

static struct _keytable template_keytable[] =
{    
    {XK_Control_L,	0x63 }, /* left control = control */	
    {XK_Multi_key,	0x63 }, /* right control = control */
    {XK_Super_L,	0x66 },	/* left win = LAMIGA */
    {XK_Super_R,	0x67 },	/* right win = RAMIGA */
    {XK_Meta_L,		0x64 }, /* left Alt = LALT */
    {XK_Mode_switch,	0x65 }, /* right Alt = RALT */
    
    /* Key left of S */
    {XK_A,		0x20 },
    {XK_a,		0x20 },
    
    /* Key right of N */
    {XK_M,		0x37 },
    {XK_m,		0x37 },
    
    /* Key right of TAB */
    {XK_Q,		0x10 },
    {XK_q,		0x10 },
    
    /* Key between T and U */
    {XK_????,		0x15 },
    {XK_????,		0x15 },
    
    /* Key left of E */
    {XK_W,		0x11 },
    {XK_w,		0x11 },
    
    /* Key left of X */
    {XK_????,		0x31 },
    {XK_????,		0x31 },

    
    /* Key left of 1 */
    {XK_????,		0x00 },
    
    /* Keys right of 0 */
    {XK_????,		0x0B },
    {XK_????,		0x0C },
    
    /* Keys right of P */
    {XK_????,		0x1A },
    {XK_????,		0x1B },
    
    /* Keys right of L */
    {XK_????,		0x29 },
    {XK_????,		0x2A }, 
    {XK_????,		0x2B }, /* Third key right of L might not be present */
    
    /* Key right of shift and 2nd left of X (might not be present) */       
    {XK_less,		0x30 }, 
    
    /* Keys 2nd right of N (= usually right of M) */
    {XK_comma,		0x38 }, 
    {XK_period,		0x39 }, 
    {XK_slash,		0x3A },
        
    {0, -1 }
};

#endif

/****************************************************************************************/

OOP_Object * X11Kbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL    	    has_kbd_hidd = FALSE;
    struct TagItem *tag, *tstate;
    APTR    	    callback = NULL;
    APTR    	    callbackdata = NULL;
    
    EnterFunc(bug("X11Kbd::New()\n"));
 
    ObtainSemaphoreShared( &XSD(cl)->sema);
 
    if (XSD(cl)->kbdhidd)
    	has_kbd_hidd = TRUE;

    ReleaseSemaphore( &XSD(cl)->sema);
 
    if (has_kbd_hidd) /* Cannot open twice */
    	ReturnPtr("X11Kbd::New", OOP_Object *, NULL); /* Should have some error code here */

    tstate = msg->attrList;
    D(bug("tstate: %p, tag=%x\n", tstate, tstate->ti_Tag));	
    
    while ((tag = NextTagItem((const struct TagItem **)&tstate)))
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
    	ReturnPtr("X11Kbd::New", OOP_Object *, NULL); /* Should have some error code here */

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
	struct x11kbd_data *data = OOP_INST_DATA(cl, o);
	
	data->kbd_callback = (VOID (*)(APTR, UWORD))callback;
	data->callbackdata = callbackdata;
	data->prev_keycode = 0xFFFF;
	
	ObtainSemaphore( &XSD(cl)->sema);
	XSD(cl)->kbdhidd = o;
	ReleaseSemaphore( &XSD(cl)->sema);
    }
    
    ReturnPtr("X11Kbd::New", OOP_Object *, o);
}

/****************************************************************************************/

VOID X11Kbd__Hidd_X11Kbd__HandleEvent(OOP_Class *cl, OOP_Object *o, struct pHidd_X11Kbd_HandleEvent *msg)
{
    struct x11kbd_data  *data;    
    XKeyEvent 	    	*xk;
    UWORD   	    	 keycode;
    
    EnterFunc(bug("x11kbd_handleevent()\n"));
    
    data = OOP_INST_DATA(cl, o);
    xk = &(msg->event->xkey);
    
    keycode = (UWORD)xkey2hidd(xk, XSD(cl));
    
    if (msg->event->type == KeyRelease)
    {
	keycode |= IECODE_UP_PREFIX;    	
    }
    
    if (keycode != data->prev_keycode)
    {
    	data->kbd_callback(data->callbackdata, keycode);
	data->prev_keycode = keycode;
    }

    ReturnVoid("X11Kbd::HandleEvent");
}

/****************************************************************************************/

#undef XSD
#define XSD(cl) xsd

/****************************************************************************************/

WORD lookup_keytable(KeySym *ks, struct _keytable *keytable)
{
    short t;
    WORD  result = -1;
    
    for (t = 0; keytable[t].hiddcode != -1; t++)
    {
	if (*ks == keytable[t].keysym)
	{
	    D(bug("xktac: found in key table\n"));
	    result = keytable[t].hiddcode;
	    break;
	}
    }
    
    return result;
}

/****************************************************************************************/

long xkey2hidd (XKeyEvent *xk, struct x11_staticdata *xsd)
{
    char    buffer[10];
    KeySym  ks;
    int     count;
    long    result;

    D(bug("xkey2hidd\n"));
   
    if (havetable)
    {
        result = -1;
	if ((xk->keycode >= 0) && (xk->keycode < 256))
	{
	    result = keycode2rawkey[xk->keycode];
	    if (result == 255) result = -1;
	}
	
	return result;
    }
    
    LOCK_X11
    xk->state = 0;
    count = XLookupString (xk, buffer, 10, &ks, NULL);
    UNLOCK_X11
    
    D(bug("xk2h: Code %d (0x%x). Event was decoded into %d chars: %d (0x%x)\n",xk->keycode, xk->keycode, count,ks,ks));
    
    result = lookup_keytable(&ks, keytable);
    if (result == -1) result = lookup_keytable(&ks, english_keytable);
    
    if (result != -1)
    {
	ReturnInt ("xk2h", long, result);
    }
    
    D(bug("xk2h: Passing X keycode\n", xk->keycode & 0xffff));

    result = xk->keycode & 0xffff;

    ReturnInt ("xk2h", long, result);
    
} /* XKeyToAmigaCode */

/****************************************************************************************/

#if X11_LOAD_KEYMAPTABLE

/****************************************************************************************/

static void LoadKeyCode2RawKeyTable(struct x11_staticdata *xsd)
{
    char *filename = "DEVS:Keymaps/X11/keycode2rawkey.table";
    BPTR  fh;
    struct DosLibrary *DOSBase;
    
    DOSBase = (struct DosLibrary *)OpenLibrary(DOSNAME, 37);
    if (DOSBase == NULL)
    {
	bug("LoadKeyCode2RawKeyTable: Opening %s failed\n", DOSNAME);
	return;
    }
	
    if ((fh = Open(filename, MODE_OLDFILE)))
    {
	if ((256 == Read(fh, keycode2rawkey, 256)))
	{
		bug("LoadKeyCode2RawKeyTable: keycode2rawkey.table successfully loaded!\n");
		havetable = TRUE;
	}
	else
	{
		bug("LoadKeyCode2RawKeyTable: Reading from \"%s\" failed!\n", filename);
	}
	Close(fh);
    }
    else
    {
	bug("\nLoadKeyCode2RawKeyTable: Loading \"%s\" failed!\n"
	    "\n"
	    "This means that many/most/all keys on your keyboard won't work as you\n"
	    "would expect in AROS. Therefore you should create this table by either\n"
	    "using the default table:\n"
	    "\n"
	    "    mmake .default-x11keymaptable\n"
	    "\n"
	    "or generating your own one:\n"
	    "\n"
	    "    mmake .change-x11keymaptable\n"
	    "\n"
	    "The default keymaptable probably works with most PCs having a 105 key\n"
	    "keyboard if you are using XFree86 as X Server (might also work with\n"
	    "others). So try that one first!\n"
	    "\n"
	    "Since the keymap table will be deleted when you do a \"make clean\" you\n"
	    "might want to make a backup of it. Then you will be able to restor it later:\n"
	    "\n"
	    "    mmake .backup-x11keymaptable\n"
	    "    mmake .restore-x11keymaptable\n"
	    "\n"
	    "The keymap table will be backuped in your HOME directory.\n"
	    "\n"
	    "Note that the keymaptable only makes sure that your keyboard looks as\n"
	    "much as possible like an Amiga keyboard to AROS. So with the keymaptable\n"
	    "alone the keyboard will behave like an Amiga keyboard with American layout\n."
	    "For other layouts you must activate the correct keymap just like in AmigaOS.\n", 
            filename);
    }
    
    CloseLibrary((struct Library *)DOSBase);
}

/****************************************************************************************/

#endif

/****************************************************************************************/

#undef XSD
#define XSD(cl) (&LIBBASE->xsd)

/****************************************************************************************/

AROS_SET_LIBFUNC(kbd_init, LIBBASETYPE, LIBBASE) 
{
    AROS_SET_LIBFUNC_INIT

#if X11_LOAD_KEYMAPTABLE
    LoadKeyCode2RawKeyTable(&LIBBASE->xsd);
#endif

    return OOP_ObtainAttrBases(attrbases);
    
    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_LIBFUNC(kbd_expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    OOP_ReleaseAttrBases(attrbases);

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

ADD2INITLIB(kbd_init, 0);
ADD2EXPUNGELIB(kbd_expunge, 0);

/****************************************************************************************/
