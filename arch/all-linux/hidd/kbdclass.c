/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Linux hidd handling keyboard
    Lang: English.
*/


#include <dos/dos.h>

#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/dos.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/keyboard.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/signal.h>
#include <string.h>
#include <errno.h>

#include "linux_intern.h"

#include <aros/debug.h>
#include <stdio.h>

#define IID_Hidd_LinuxKbd	"hidd.kbd.linux"
#define CLID_Hidd_LinuxKbd	"hidd.kbd.linux"

static UBYTE keycode2rawkey[256];
static BOOL havetable;
void setup_sighandling(void);
void cleanup_sighandling();

struct linuxkbd_data
{
    VOID (*kbd_callback)(APTR, UWORD);
    APTR callbackdata;
};

static AttrBase HiddKbdAB = 0;

static struct ABDescr attrbases[] =
{
    { IID_Hidd_Kbd, &HiddKbdAB },
    { NULL, NULL }
};

                        
/***** Kbd::New()  ***************************************/
static Object * kbd_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    BOOL has_kbd_hidd = FALSE;
    struct TagItem *tag, *tstate;
    APTR callback = NULL;
    APTR callbackdata = NULL;
    
    EnterFunc(bug("Kbd::New()\n"));
 
 
    if (LSD(cl)->kbdhidd)
    	has_kbd_hidd = TRUE;

 
    if (has_kbd_hidd) /* Cannot open twice */
    	ReturnPtr("Kbd::New", Object *, NULL); /* Should have some error code here */

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
    	ReturnPtr("Kbd::New", Object *, NULL); /* Should have some error code here */

    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (o)
    {
	struct linuxkbd_data *data = INST_DATA(cl, o);
	data->kbd_callback = (VOID (*)(APTR, UWORD))callback;
	data->callbackdata = callbackdata;
	
	LSD(cl)->kbdhidd = o;
    }
    ReturnPtr("Kbd::New", Object *, o);
}

/***** Kbd::HandleEvent()  ***************************************/
#if 0
static VOID kbd_handleevent(Class *cl, Object *o, struct pHidd_Kbd_HandleEvent *msg)
{
    struct linuxkbd_data * data;
    
    XKeyEvent *xk;

    EnterFunc(bug("linuxkbd_handleevent()\n"));
    xk = &(msg->event->xkey);
    data = INST_DATA(cl, o);
    if (msg->event->type == KeyPress)
    {
	data->kbd_callback(data->callbackdata
		, (UWORD)xkey2hidd(xk, LSD(cl)));
		
    }
    else if (msg->event->type == KeyRelease)
    {
	data->kbd_callback(data->callbackdata
		, (UWORD)xkey2hidd(xk, LSD(cl)) | IECODE_UP_PREFIX);
    }

    
    ReturnVoid("Kbd::HandleEvent");
}

#endif


#undef LSD
#define LSD(cl) lsd

#if 0
long StateToQualifier (unsigned long state)
{
    long result;

    result = 0;

    if (state & ShiftMask)
	result |= SHIFT;

    if (state & ControlMask)
	result |= CTRL;

    if (state & LockMask)
	result |= CAPS;

    if (state & Mod2Mask) /* Right Alt */
	result |= LALT;

    if (state & 0x2000) /* Mode switch */
	result |= RALT;

    if (state & Mod1Mask) /* Left Alt */
	result |= AMIGAKEYS;

    if (state & Button1Mask)
	result |= IEQUALIFIER_LEFTBUTTON;

    if (state & Button2Mask)
	result |= IEQUALIFIER_RBUTTON;

    if (state & Button3Mask)
	result |= IEQUALIFIER_MIDBUTTON;

    return (result);
} /* StateToQualifier */


WORD lookup_keytable(KeySym *ks, struct _keytable *keytable)
{
    short t;
    WORD result = -1;
    
    for (t=0; keytable[t].hiddcode != -1; t++)
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


long xkey2hidd (XKeyEvent *xk, struct linux_staticdata *lsd)
{
    char buffer[10];
    KeySym ks;
    int count;
    long result;


 
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
LX11
    xk->state = 0;
    count = XLookupString (xk, buffer, 10, &ks, NULL);
UX11
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

/****************  LoadKeyCode2RawKeyTable()  ***************************/

static void LoadKeyCode2RawKeyTable(struct linux_staticdata *lsd)
{
    char *filename = "DEVS:Keymaps/X11/keycode2rawkey.table";
    BPTR fh;
    
    if ((fh =Open(filename, MODE_OLDFILE)))
    {
        if ((256 == Read(fh, keycode2rawkey, 256)))
	{
	    bug("LoadKeyCode2RawKeyTable: keycode2rawkey.table successfully loaded!\n");
	    havetable = TRUE;
	} else {
            bug("LoadKeyCode2RawKeyTable: Reading from \"%s\" failed!\n", filename);
	}
        Close(fh);
    } else {
	bug("\nLoadKeyCode2RawKeyTable: Loading \"%s\" failed!\n"
	    "\n"
	    "This means that many/most/all keys on your keyboard won't work as you\n"
	    "would expect in AROS. Therefore you should create this table by either\n"
	    "using the default table:\n"
	    "\n"
	    "    mmake .default-linuxkeymaptable\n"
	    "\n"
	    "or generating your own one:\n"
	    "\n"
	    "    mmake .change-linuxkeymaptable\n"
	    "\n"
	    "The default keymaptable probably works with most PCs having a 105 key\n"
	    "keyboard if you are using XFree86 as X Server (might also work with\n"
	    "others). So try that one first!\n"
	    "\n"
	    "Since the keymap table will be deleted when you do a \"make clean\" you\n"
	    "might want to make a backup of it. Then you will be able to restor it later:\n"
	    "\n"
	    "    mmake .backup-linuxkeymaptable\n"
	    "    mmake .restore-linuxkeymaptable\n"
	    "\n"
	    "The keymap table will be backuped in your HOME directory.\n"
	    "\n"
	    "Note that the keymaptable only makes sure that your keyboard looks as\n"
	    "much as possible like an Amiga keyboard to AROS. So with the keymaptable\n"
	    "alone the keyboard will behave like an Amiga keyboard with American layout\n."
	    "For other layouts you must activate the correct keymap file (which are in\n"
	    "\"DEVS:Keymaps\") just like in AmigaOS. Actually AROS has only German,\n"
	    "Italian and Swedish keymap files. You can activate them inside AROS by typing\n"
	    "this in a AROS Shell or adding it to the AROS Startup-Sequence:\n"
	    "\n"
	    "    Setmap pc105_d\n"
	    "    Setmap pc105_i\n"
	    "    Setmap pc105_s\n"
	    "\n", filename);
    }
}
#endif

/********************  init_kbdclass()  *********************************/


#define NUM_ROOT_METHODS 1
#define NUM_LINUXKBD_METHODS 1

Class *init_kbdclass (struct linux_staticdata *lsd)
{
    Class *cl = NULL;

    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = {
    	{ METHODDEF(kbd_new),		moRoot_New },
	{ NULL, 0UL }
    };
    
#if 0
    struct MethodDescr kbdhidd_descr[NUM_LINUXKBD_METHODS + 1] = 
    {
    	{ METHODDEF(kbd_handleevent),	moHidd_LinuxKbd_HandleEvent },
	{ NULL, 0UL }
    };
#endif
    
    struct InterfaceDescr ifdescr[] = {
    	{ root_descr,	 IID_Root, 		NUM_ROOT_METHODS	},
#if 0
    	{ kbdhidd_descr, IID_Hidd_LinuxKbd, 	NUM_LINUXKBD_METHODS	},
#endif
	{ NULL, NULL, 0 }
    };
    
    AttrBase MetaAttrBase = ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] = {
	{ aMeta_SuperID,		(IPTR)CLID_Hidd 			},
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr				},
	{ aMeta_InstSize,		(IPTR)sizeof (struct linuxkbd_data)	},
	{ aMeta_ID,			(IPTR)CLID_Hidd_LinuxKbd		},
	{TAG_DONE, 0UL}
    };

#if 0    
    LoadKeyCode2RawKeyTable(lsd);
#endif    
    if (MetaAttrBase) {
    
    	cl = NewObject(NULL, CLID_HiddMeta, tags);
    	if (NULL != cl) {
	    
	    if (ObtainAttrBases(attrbases)) {
		cl->UserData = (APTR)lsd;
		lsd->kbdclass = cl;
		
	    	AddClass(cl);
	    } else {
	    	free_kbdclass(lsd);
		cl = NULL;
	    }
	}
	/* Don't need this anymore */
	ReleaseAttrBase(IID_Meta);
    }
    
    return cl;
}


/*************** free_kbdclass()  **********************************/
VOID free_kbdclass(struct linux_staticdata *lsd)
{
    EnterFunc(bug("free_kbdclass(lsd=%p)\n", lsd));

    if(NULL != lsd)
    {
	
        if (NULL != lsd->kbdclass) {
        	RemoveClass(lsd->kbdclass);
	
		DisposeObject((Object *) lsd->kbdclass);
        	lsd->kbdclass = NULL;
	}
	
	ReleaseAttrBases(attrbases);

    }

    ReturnVoid("free_kbdclass");
}


int set_kbd_mode(int fd, int mode, int *oldmode)
{
    /* Get and preserve the old kbd mode */
    if (NULL != oldmode) {
	if (-1 == ioctl(fd, KDGKBMODE, oldmode)) {
	    fprintf(stderr, "Unable to get old kbd mode: %s\n", strerror(errno));
	    return 0;
	}
    }
    
    /* Set the new mode */
    if (-1 == ioctl(fd, KDSKBMODE, mode)) {
	fprintf(stderr, "Unable to set new kbd mode: %s\n", strerror(errno));
	return 0;
    }
    
    return 1;
}


static int oldkbdmode;
static int kbdfd;
static struct termios oldtio;
static struct linux_staticdata *lsdata;

BOOL  mode_done	   = FALSE
    , fd_done	   = FALSE
    , termios_done = FALSE;
    
#define KBD_DEVNAME "/dev/console"

BOOL init_kbd(struct linux_staticdata *lsd)
{
    BOOL ret = TRUE;
    
    lsdata = lsd;
kprintf("INIT_KBD\n");
    
    lsd->kbdfd = kbdfd = open(KBD_DEVNAME, O_RDONLY);
    if (-1 == kbdfd) {
	kprintf("!!! Could not open keyboard device: %s\n", strerror(errno));
	ret = FALSE;
    } else {
	/* Try to read some data from the keyboard */
	struct termios newtio;
	
	fd_done = TRUE;
	
	setup_sighandling();
kprintf("SIGNALS SETUP\n");	
	if ( (-1 == tcgetattr(kbdfd, &oldtio)) || (-1 == tcgetattr(kbdfd, &newtio))) {
	    kprintf("!!! Could not get old termios attrs: %s\n", strerror(errno));
	    ret = FALSE;
	} else {
	    /* Set some new attrs */
	    newtio.c_lflag = ~(ICANON | ECHO | ISIG);
	    newtio.c_iflag = 0;
	    newtio.c_cc[VMIN] = 1;
	    newtio.c_cc[VTIME] = 0;
	    
	    if (-1 == tcsetattr(kbdfd, TCSAFLUSH, &newtio)) {
		kprintf("!!! Could not set new termio: %s\n", strerror(errno));
		ret = FALSE;
	    } else {
	    	termios_done = TRUE;
kprintf("SET TERMIO ATTRS\n");
		if (!set_kbd_mode(kbdfd, K_MEDIUMRAW, &oldkbdmode)) {
		    kprintf("!!! Could not set kbdmode\n");
		    ret = FALSE;
		} else {
kprintf("KBD MODE SET\n");
		    mode_done = TRUE;
		    ret = TRUE;
		}

	    } /* if (termios attrs set) */
	} /*  if (got old termios attrs) */
    }
    if (!ret) {
    	cleanup_kbd(lsd);
    }
    return ret;
    
}

VOID cleanup_kbd(struct linux_staticdata *lsd)
{
    /* Reset the kbd mode */
    if (mode_done)
	set_kbd_mode(kbdfd, oldkbdmode, NULL);

    if (termios_done)
	tcsetattr(kbdfd, TCSAFLUSH, &oldtio);
   
    if (fd_done)
    	close(kbdfd);
	   
    cleanup_sighandling();
    
    return;
}

const int signals[] = {
	SIGHUP, SIGINT,	SIGQUIT, SIGILL,
	SIGTRAP, SIGBUS, SIGFPE, SIGKILL,
	/* SIGALRM, */  SIGSEGV, SIGTERM
};

void kbdsighandler(int sig)
{
    cleanup_kbd(lsdata);
}

/* Avoid that some signal kills us without resetting the keyboard */
void setup_sighandling(void)
{
    ULONG i;
    
    for (i = 0; i < sizeof (signals); i ++) {
    	signal(signals[i], kbdsighandler);
    }
    
}

void cleanup_sighandling()
{
    ULONG i;
    for (i = 0; i < sizeof (signals); i ++) {
	signal(signals[i], SIG_DFL);
    }	
}
