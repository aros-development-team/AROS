/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
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
#include <devices/inputevent.h>

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

static UBYTE scancode2rawkey[256];
static BOOL havetable;
void setup_sighandling(void);
void cleanup_sighandling();

struct linuxkbd_data
{
    VOID (*kbd_callback)(APTR, UWORD);
    APTR callbackdata;
};

static OOP_AttrBase HiddKbdAB = 0;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_Kbd, &HiddKbdAB },
    { NULL, NULL }
};

static UBYTE scancode2rawkey[256];
static BOOL havetable = FALSE;

static UWORD scancode2hidd(UBYTE scancode, struct linux_staticdata *lsd);

/***** Kbd::New()  ***************************************/
static OOP_Object * kbd_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL has_kbd_hidd = FALSE;
    struct TagItem *tag, *tstate;
    APTR callback = NULL;
    APTR callbackdata = NULL;
    
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


static OOP_Object *kbd_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    ObtainSemaphore(&LSD(cl)->sema);
    LSD(cl)->kbdhidd = NULL;
    ReleaseSemaphore(&LSD(cl)->sema);
    
    OOP_DoSuperMethod(cl, o, msg);
    
}

/***** LinuxKbd::HandleEvent()  ***************************************/

static VOID kbd_handleevent(OOP_Class *cl, OOP_Object *o, struct pHidd_LinuxKbd_HandleEvent *msg)
{
    struct linuxkbd_data * data;
    UBYTE scancode;
    UWORD hiddcode;

    EnterFunc(bug("linuxkbd_handleevent()\n"));
    
    data = OOP_INST_DATA(cl, o);
    
    scancode = msg->scanCode;
    hiddcode = scancode2hidd(scancode, LSD(cl));
    
    if (hiddcode != 0xFF) {
    
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
const UBYTE deftable[] = {
	  0xff, 0x43, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
	, 0x09, 0x0a, 0x0b, 0x0c, 0x41, 0x42, 0x10, 0x11, 0x12, 0x13
	, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x44, 0x63
	, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29
	, 0x2a, 0xff, 0x60, 0xff, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36
	, 0x37, 0x38, 0x39, 0x3a, 0x61, 0xff, 0x64, 0x40, 0x62, 0x50
	, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0xff
	, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x4b, 0x6f, 0xff
	, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	, 0x65, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	, 0x47, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	, 0xff, 0xff, 0xff, 0xff, 0xff, 0x66, 0xff, 0x67
};
static UWORD scancode2hidd(UBYTE scancode, struct linux_staticdata *lsd)
{
    UWORD hiddcode;
    if ((scancode & 0x80) == 0x80)
	scancode &= ~0x80;
	
    if (havetable) {
	hiddcode = scancode2rawkey[scancode];
    } else {
	if (scancode >= DEF_TAB_SIZE)
	    hiddcode = 0xFF;
	else
	    hiddcode = deftable[scancode];
    }
    return hiddcode;
}

/****************  LoadScanCode2RawKeyTable()  ***************************/

static void LoadScanCode2RawKeyTable(struct linux_staticdata *lsd)
{
    char *filename = "DEVS:Keymaps/X11/keycode2rawkey.table";
    BPTR fh;
    
    if ((fh =Open(filename, MODE_OLDFILE)))
    {
        if ((256 == Read(fh, scancode2rawkey, 256)))
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

/********************  init_kbdclass()  *********************************/


#define NUM_ROOT_METHODS 2
#define NUM_LINUXKBD_METHODS 1

OOP_Class *init_kbdclass (struct linux_staticdata *lsd)
{
    OOP_Class *cl = NULL;

    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] = {
    	{ OOP_METHODDEF(kbd_new),		moRoot_New },
    	{ OOP_METHODDEF(kbd_dispose),	moRoot_Dispose },
	{ NULL, 0UL }
    };
    
    struct OOP_MethodDescr kbdhidd_descr[NUM_LINUXKBD_METHODS + 1] = 
    {
    	{ OOP_METHODDEF(kbd_handleevent),	moHidd_LinuxKbd_HandleEvent },
	{ NULL, 0UL }
    };
    
    struct OOP_InterfaceDescr ifdescr[] = {
    	{ root_descr,	 IID_Root, 		NUM_ROOT_METHODS	},
    	{ kbdhidd_descr, IID_Hidd_LinuxKbd, 	NUM_LINUXKBD_METHODS	},
	{ NULL, NULL, 0 }
    };
    
    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] = {
	{ aMeta_SuperID,		(IPTR)CLID_Hidd 			},
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr				},
	{ aMeta_InstSize,		(IPTR)sizeof (struct linuxkbd_data)	},
	{ aMeta_ID,			(IPTR)CLID_Hidd_LinuxKbd		},
	{TAG_DONE, 0UL}
    };

    LoadScanCode2RawKeyTable(lsd);
    if (MetaAttrBase) {
    
    	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
    	if (NULL != cl) {
	    
	    if (OOP_ObtainAttrBases(attrbases)) {
		cl->UserData = (APTR)lsd;
		lsd->kbdclass = cl;
		
	    	OOP_AddClass(cl);
	    } else {
	    	free_kbdclass(lsd);
		cl = NULL;
	    }
	}
	/* Don't need this anymore */
	OOP_ReleaseAttrBase(IID_Meta);
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
        	OOP_RemoveClass(lsd->kbdclass);
	
		OOP_DisposeObject((OOP_Object *) lsd->kbdclass);
        	lsd->kbdclass = NULL;
	}
	
	OOP_ReleaseAttrBases(attrbases);

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
	/* SIGALRM, */  SIGSEGV , SIGTERM
};


void exit_sighandler(int sig)
{
    printf("PARENT EXITING VIA SIGHANDLER\n");
    cleanup_kbd(lsdata);
    exit(0);
}

void kbdsighandler(int sig)
{
    cleanup_kbd(lsdata);
}

/* Avoid that some signal kills us without resetting the keyboard */
void setup_sighandling(void)
{
    ULONG i;
    pid_t pid;
    
    for (i = 0; i < sizeof (signals); i ++) {
    	signal(signals[i], kbdsighandler);
    }
    
    signal(SIGTERM, exit_sighandler);
    
    /* Sig alrm is is taken so we have to fork() to create a new process
      that will kill us after a while */
      
    pid = fork();
    
    switch (pid) {
	case -1:
	    kprintf("!!!!!!!! ERROR FORKING !!!!!!!!!!!!!!\n");
	    exit(1);

	    
	case 0: {
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
    for (i = 0; i < sizeof (signals); i ++) {
	signal(signals[i], SIG_DFL);
    }	
}


#undef OOPBase
#define OOPBase ((struct Library *)OOP_OCLASS(OOP_OCLASS(OOP_OCLASS(o)))->UserData)

VOID HIDD_LinuxKbd_HandleEvent(OOP_Object *o, UBYTE scanCode)
{
    static OOP_MethodID mid = 0;
    struct pHidd_LinuxKbd_HandleEvent p;
    
    if (!mid)
	mid = OOP_GetMethodID(IID_Hidd_LinuxKbd, moHidd_LinuxKbd_HandleEvent);
	
    p.mID	= mid;
    p.scanCode	= scanCode;
    
    OOP_DoMethod(o, (OOP_Msg)&p);
}
