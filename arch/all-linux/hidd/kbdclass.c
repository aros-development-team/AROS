/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Linux hidd handling keyboard
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <dos/dos.h>

#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/dos.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <devices/inputevent.h>
#include <devices/rawkeycodes.h>

#include <aros/symbolsets.h>

/* hack: prevent linux include header <bits/time.h> to re-define timeval struct */
#  define _STRUCT_TIMEVAL 1

#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include "linux_intern.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>
#include <stdio.h>
#include <stdlib.h>

static UBYTE scancode2rawkey[256];
static BOOL havetable;
void setup_sighandling(void);
void cleanup_sighandling();

static OOP_AttrBase HiddKbdAB = 0;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_Kbd, &HiddKbdAB  },
    { NULL  	  , NULL    	}
};

static UBYTE scancode2rawkey[256];
static BOOL havetable = FALSE;

static UWORD scancode2hidd(UBYTE scancode, struct linux_staticdata *lsd);

/***** Kbd::New()  ***************************************/
OOP_Object * LinuxKbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL    	    has_kbd_hidd = FALSE;
    struct TagItem *tag, *tstate;
    APTR    	    callback = NULL;
    APTR    	    callbackdata = NULL;
    
    EnterFunc(bug("Kbd::New()\n"));
 
    ObtainSemaphore(&LSD(cl)->sema);
    if (LSD(cl)->kbdhidd)
    	has_kbd_hidd = TRUE;
    ReleaseSemaphore(&LSD(cl)->sema);
 
    if (has_kbd_hidd) /* Cannot open twice */
    	ReturnPtr("Kbd::New", OOP_Object *, NULL); /* Should have some error code here */

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
    	ReturnPtr("Kbd::New", OOP_Object *, NULL); /* Should have some error code here */

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
	struct linuxkbd_data *data = OOP_INST_DATA(cl, o);

	data->kbd_callback = (VOID (*)(APTR, UWORD))callback;
	data->callbackdata = callbackdata;
	
    	ObtainSemaphore(&LSD(cl)->sema);
	LSD(cl)->kbdhidd = o;
    	ReleaseSemaphore(&LSD(cl)->sema);
    }
    ReturnPtr("Kbd::New", OOP_Object *, o);
}


VOID LinuxKbd__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    ObtainSemaphore(&LSD(cl)->sema);
    LSD(cl)->kbdhidd = NULL;
    ReleaseSemaphore(&LSD(cl)->sema);
    
    OOP_DoSuperMethod(cl, o, msg);  
}

/***** LinuxKbd::HandleEvent()  ***************************************/

VOID LinuxKbd__Hidd_LinuxKbd__HandleEvent(OOP_Class *cl, OOP_Object *o, struct pHidd_LinuxKbd_HandleEvent *msg)
{
    struct linuxkbd_data  *data;
    UBYTE   	    	   scancode;
    UWORD   	    	   hiddcode;

    EnterFunc(bug("linuxkbd_handleevent()\n"));
    
    data = OOP_INST_DATA(cl, o);
    
    scancode = msg->scanCode;
    hiddcode = scancode2hidd(scancode, LSD(cl));
    
    if (hiddcode != 0xFF)
    {  
	if (scancode >= 0x80)
	    hiddcode |= IECODE_UP_PREFIX;

	data->kbd_callback(data->callbackdata, hiddcode);
    }
    
    ReturnVoid("Kbd::HandleEvent");
}


#undef LSD
#define LSD(cl) lsd

/**************** scancode2hidd() ****************/
#define DEF_TAB_SIZE 128

const UBYTE deftable[] =
{
    0xff,
    RAWKEY_ESCAPE,
    RAWKEY_1,
    RAWKEY_2,
    RAWKEY_3,
    RAWKEY_4,
    RAWKEY_5,
    RAWKEY_6,
    RAWKEY_7,
    RAWKEY_8,
    RAWKEY_9,
    RAWKEY_0,
    RAWKEY_MINUS,
    RAWKEY_EQUAL,
    RAWKEY_BACKSPACE,
    RAWKEY_TAB,
    RAWKEY_Q,
    RAWKEY_W,
    RAWKEY_E,
    RAWKEY_R,
    RAWKEY_T,
    RAWKEY_Y,
    RAWKEY_U,
    RAWKEY_I,
    RAWKEY_O,
    RAWKEY_P,
    RAWKEY_LBRACKET,
    RAWKEY_RBRACKET,
    RAWKEY_RETURN,
    RAWKEY_CONTROL,
    RAWKEY_A,
    RAWKEY_S,
    RAWKEY_D,
    RAWKEY_F,
    RAWKEY_G,
    RAWKEY_H,
    RAWKEY_J,
    RAWKEY_K,
    RAWKEY_L,
    RAWKEY_SEMICOLON,
    RAWKEY_QUOTE,
    RAWKEY_TILDE,
    RAWKEY_LSHIFT,
    RAWKEY_2B,
    RAWKEY_Z,
    RAWKEY_X,
    RAWKEY_C,
    RAWKEY_V,
    RAWKEY_B,
    RAWKEY_N,
    RAWKEY_M,
    RAWKEY_COMMA,
    RAWKEY_PERIOD,
    RAWKEY_SLASH,
    RAWKEY_RSHIFT,
    0x5C,
    RAWKEY_LALT,
    RAWKEY_SPACE,
    RAWKEY_CAPSLOCK,
    RAWKEY_F1,
    RAWKEY_F2,
    RAWKEY_F3,
    RAWKEY_F4,
    RAWKEY_F5,
    RAWKEY_F6,
    RAWKEY_F7,
    RAWKEY_F8,
    RAWKEY_F9,
    RAWKEY_F10,
    0x5A,
    0xff,
    RAWKEY_KP_7,
    RAWKEY_KP_8,
    RAWKEY_KP_9,
    0x5D,
    RAWKEY_KP_4,
    RAWKEY_KP_5,
    RAWKEY_KP_6,
    RAWKEY_KP_PLUS,
    RAWKEY_KP_1,
    RAWKEY_KP_2,
    RAWKEY_KP_3,
    RAWKEY_KP_0,
    RAWKEY_KP_DECIMAL,
    0xff,
    0xff,
    RAWKEY_LESSGREATER,
    RAWKEY_F11,
    RAWKEY_F12,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    RAWKEY_KP_ENTER,
    RAWKEY_CONTROL,
    0x5B,
    0xff,
    RAWKEY_RALT,
    RAWKEY_PAUSE,
    RAWKEY_HOME,
    RAWKEY_UP,
    RAWKEY_PAGEUP,
    RAWKEY_LEFT,
    RAWKEY_RIGHT,
    RAWKEY_END,
    RAWKEY_DOWN,
    RAWKEY_PAGEDOWN,
    RAWKEY_INSERT,
    RAWKEY_DELETE,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    RAWKEY_PAUSE,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    RAWKEY_LAMIGA,
    RAWKEY_RAMIGA,
    0xff
};
static UWORD scancode2hidd(UBYTE scancode, struct linux_staticdata *lsd)
{
    UWORD hiddcode;
    
    if ((scancode & 0x80) == 0x80)
	scancode &= ~0x80;
	
    if (havetable)
    {
	hiddcode = scancode2rawkey[scancode];
    }
    else
    {
	if (scancode >= DEF_TAB_SIZE)
	    hiddcode = 0xFF;
	else
	    hiddcode = deftable[scancode];
    }
    
    return hiddcode;
}

#if 0
/****************  LoadScanCode2RawKeyTable()  ***************************/

static void LoadScanCode2RawKeyTable(struct linux_staticdata *lsd)
{
    char *filename = "DEVS:Keymaps/X11/keycode2rawkey.table";
    BPTR  fh;
    
    if ((fh =Open(filename, MODE_OLDFILE)))
    {
        if ((256 == Read(fh, scancode2rawkey, 256)))
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


AROS_SET_LIBFUNC(Init_KbdClass, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

#if 0
    LoadScanCode2RawKeyTable(&LIBBASE->lsd);
#endif

    if (!OOP_ObtainAttrBases(attrbases))
	return FALSE;
    
    if (!init_linuxkbd(&LIBBASE->lsd))
    {
	OOP_ReleaseAttrBases(attrbases);
	return FALSE;
    }
    
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}


/*************** free_kbdclass()  **********************************/
AROS_SET_LIBFUNC(Expunge_KbdClass, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    cleanup_linuxkbd(&LIBBASE->lsd);
    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(Init_KbdClass, 0)
ADD2EXPUNGELIB(Expunge_KbdClass, 0)


int set_kbd_mode(int fd, int mode, int *oldmode)
{
    /* Get and preserve the old kbd mode */
    if (NULL != oldmode)
    {
	if (-1 == ioctl(fd, KDGKBMODE, oldmode))
	{
	    fprintf(stderr, "Unable to get old kbd mode: %s\n", strerror(errno));
	    return 0;
	}
    }
    
    /* Set the new mode */
    if (-1 == ioctl(fd, KDSKBMODE, mode))
    {
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

BOOL init_linuxkbd(struct linux_staticdata *lsd)
{
    BOOL ret = TRUE;
    lsdata = lsd;

kprintf("INIT_KBD\n");
    
    lsd->kbdfd = kbdfd = open(KBD_DEVNAME, O_RDONLY);
    if (-1 == kbdfd)
    {
	kprintf("!!! Could not open keyboard device: %s\n", strerror(errno));
	ret = FALSE;
    }
    else
    {
	/* Try to read some data from the keyboard */
	struct termios newtio;
	
	fd_done = TRUE;
	

	setup_sighandling();

kprintf("SIGNALS SETUP\n");	

	if ( (-1 == tcgetattr(kbdfd, &oldtio)) || (-1 == tcgetattr(kbdfd, &newtio)))
	{
	    kprintf("!!! Could not get old termios attrs: %s\n", strerror(errno));
	    ret = FALSE;
	}
	else
	{
	    /* Set some new attrs */
	    newtio.c_lflag = ~(ICANON | ECHO | ISIG);
	    newtio.c_iflag = 0;
	    newtio.c_cc[VMIN] = 1;
	    newtio.c_cc[VTIME] = 0;
	    
	    if (-1 == tcsetattr(kbdfd, TCSAFLUSH, &newtio))
	    {
		kprintf("!!! Could not set new termio: %s\n", strerror(errno));
		ret = FALSE;
	    }
	    else
	    {
	    	termios_done = TRUE;
kprintf("SET TERMIO ATTRS\n");

		if (!set_kbd_mode(kbdfd, K_MEDIUMRAW, &oldkbdmode))
		{
		    kprintf("!!! Could not set kbdmode\n");
		    ret = FALSE;
		}
		else
		{
kprintf("KBD MODE SET\n");
		    mode_done = TRUE;
		    ret = TRUE;
		    
		    ioctl(kbdfd, KDSETMODE, KD_GRAPHICS); /* stegerg */
		    
		}

	    } /* if (termios attrs set) */
	} /*  if (got old termios attrs) */
    }
    
    if (!ret)
    {
    	cleanup_linuxkbd(lsd);
    }

    return ret;
    
}

VOID cleanup_linuxkbd(struct linux_staticdata *lsd)
{
    /* Reset the kbd mode */
    if (mode_done)
    {
	ioctl(kbdfd, KDSETMODE, KD_TEXT); /* stegerg */

	set_kbd_mode(kbdfd, oldkbdmode, NULL);
    }
    
    if (termios_done)
	tcsetattr(kbdfd, TCSAFLUSH, &oldtio);
   
    if (fd_done)
    	close(kbdfd);
	   
    cleanup_sighandling();
    
    return;
}

const int signals[] =
{
    SIGHUP, SIGINT,	SIGQUIT, SIGILL,
    SIGTRAP, SIGBUS, SIGFPE, SIGKILL,
    /* SIGALRM, */  SIGSEGV , SIGTERM
};


void exit_sighandler(int sig)
{
    printf("PARENT EXITING VIA SIGHANDLER\n");
    cleanup_linuxkbd(lsdata);
    exit(0);
}

void kbdsighandler(int sig)
{
    cleanup_linuxkbd(lsdata);
}

/* Avoid that some signal kills us without resetting the keyboard */
void setup_sighandling(void)
{
    ULONG i;
    pid_t pid;
    
    for (i = 0; i < sizeof (signals); i ++)
    {
    	signal(signals[i], kbdsighandler);
    }
    
    signal(SIGTERM, exit_sighandler);
    
    /* Sig alrm is is taken so we have to fork() to create a new process
      that will kill us after a while */
      
    pid = fork();
    
    switch (pid)
    {
	case -1:
	    kprintf("!!!!!!!! ERROR FORKING !!!!!!!!!!!!!!\n");
	    exit(1);

	    
	case 0:
	{
	    int *status = 0;
	    /* We are the child */
	    kprintf("----- CHILD GOING TO SLEEP ....\n");
	    sleep(120);
	    kprintf("-------- CHILD EXITING ------------\n");
	    kill(getppid(), SIGTERM);
	    exit(0);
	}

	default:
	    /* We are the parent */
	    kprintf("------- PARENT: PID %d\n", getpid());
	    break;
    }
    
}

void cleanup_sighandling()
{
    ULONG i;
    
    for (i = 0; i < sizeof (signals); i ++)
    {
	signal(signals[i], SIG_DFL);
    }	
}


VOID HIDD_LinuxKbd_HandleEvent(OOP_Object *o, UBYTE scanCode)
{
    static OOP_MethodID     	    	mid;
    struct pHidd_LinuxKbd_HandleEvent 	p;
    
    if (!mid)
	mid = OOP_GetMethodID(IID_Hidd_LinuxKbd, moHidd_LinuxKbd_HandleEvent);
	
    p.mID	= mid;
    p.scanCode	= scanCode;
    
    OOP_DoMethod(o, (OOP_Msg)&p);
}
