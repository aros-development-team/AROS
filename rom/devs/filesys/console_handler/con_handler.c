/*
    Copyright (C) 1995-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Filesystem that uses console device for input/output.
    Lang: english
*/

/* 
   Please always update the version-string below, if you modify the code!
*/

/* AROS includes */

#define AROS_ALMOST_COMPATIBLE 1

#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/alerts.h>
#include <utility/tagitem.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <devices/conunit.h>

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "con_handler_intern.h"



static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
static const UBYTE datatable;

struct conbase * AROS_SLIB_ENTRY(init,con_handler) ();
void AROS_SLIB_ENTRY(open,con_handler) ();
BPTR AROS_SLIB_ENTRY(close,con_handler) ();
BPTR AROS_SLIB_ENTRY(expunge,con_handler) ();
int AROS_SLIB_ENTRY(null,con_handler) ();
void AROS_SLIB_ENTRY(beginio,con_handler) ();
LONG AROS_SLIB_ENTRY(abortio,con_handler) ();

static const char end;


int con_handler_entry(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

const struct Resident con_handler_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&con_handler_resident,
    (APTR)&end,
    RTF_AUTOINIT | RTF_AFTERDOS,
    41,
    NT_DEVICE,
    -126,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="con.handler";

static const char version[]="$VER: con_handler 41.0 (02.09.1998)\r\n";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct conbase),
    (APTR)functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init,con_handler)
};

static void *const functable[]=
{
    &AROS_SLIB_ENTRY(open,con_handler),
    &AROS_SLIB_ENTRY(close,con_handler),
    &AROS_SLIB_ENTRY(expunge,con_handler),
    &AROS_SLIB_ENTRY(null,con_handler),
    &AROS_SLIB_ENTRY(beginio,con_handler),
    &AROS_SLIB_ENTRY(abortio,con_handler),
    (void *)-1
};

static const UBYTE datatable=0;

/*********************************** Support *******************************/

#define ioReq(x) ((struct IORequest *)x)

/*****************
**  open_con()  **
*****************/
static const struct TagItem win_tags[] =
{
    {WA_Width,		500},
    {WA_Height,		300},
    {WA_SmartRefresh,	TRUE},
    {WA_DragBar,	TRUE},
    {WA_IDCMP,		IDCMP_REFRESHWINDOW | IDCMP_RAWKEY | IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_NEWSIZE | IDCMP_CLOSEWINDOW },
    {WA_Title,		(IPTR)"CON:"},
    {WA_Flags,		WFLG_DEPTHGADGET | WFLG_SIZEGADGET | WFLG_DRAGBAR | WFLG_CLOSEGADGET | WFLG_SIZEBRIGHT },
    {TAG_DONE,		0UL}
};

static LONG open_con(struct conbase 	*conbase
	,struct filehandle 		**fh_ptr
	,STRPTR				filename
	,LONG				mode)
{
    LONG err = 0;
    struct filehandle *fh;
    
    EnterFunc(bug("open_conh(fhptr=%p, filename=%s, mode=%d)\n",
    	fh_ptr, filename, mode));

    fh = AllocMem(sizeof (struct filehandle), MEMF_ANY);
    if (fh)
    {
    	D(bug("fh allocated\n"));
    	/* Create msgport for console.device communication */
	fh->conmp = AllocMem(sizeof (struct MsgPort), MEMF_PUBLIC|MEMF_CLEAR);
	if (fh->conmp)
	{
	
    	    D(bug("conmp created\n"));
	    fh->conmp->mp_Flags = PA_SIGNAL;
	    fh->conmp->mp_SigBit = SIGB_DOS;
	    fh->conmp->mp_SigTask = FindTask(NULL);
	    NEWLIST(&fh->conmp->mp_MsgList);
	    
	    fh->conio = (struct IOStdReq *)CreateIORequest(fh->conmp, sizeof (struct IOStdReq));
	    if (fh->conio)
	    {
    	    	D(bug("conio created\n"));
		fh->window = OpenWindowTagList(NULL, (struct TagItem *)win_tags);
		if (fh->window)
		{
    	    	    D(bug("window opened\n"));
		    fh->conio->io_Data	 = (APTR)fh->window;
		    fh->conio->io_Length = sizeof (struct Window);
	    	    if (0 == OpenDevice("console.device", CONU_STANDARD, ioReq(fh->conio), 0))
		    {


			const UBYTE lf_on[] = {0x9B, 0x32, 0x30, 0x68 }; /* Set linefeed mode    */
			
			D(bug("device opened\n"));
			
			/* Turn the console into LF+CR mode so that both
			   linefeed and carriage return is done on 
			*/
			fh->conio->io_Command	= CMD_WRITE;
			fh->conio->io_Data	= (APTR)lf_on;
			fh->conio->io_Length	= 4;
			
			DoIO(ioReq(fh->conio));

			
		    	*fh_ptr = fh;
		    	ReturnInt("open_conh", LONG, 0);
		    
		    }
		    else
		    {
		        err = ERROR_INVALID_RESIDENT_LIBRARY;
		    }
		    CloseWindow(fh->window);
		    
		}
		else
		{
		    err = ERROR_NO_FREE_STORE;
		}
		DeleteIORequest( ioReq(fh->conio) );
	    
	    }
	    else
	    {
	    	err = ERROR_NO_FREE_STORE;
	    }
	    DeleteMsgPort(fh->conmp);
	}
	else
	{
	    err = ERROR_NO_FREE_STORE;
	}
	
	FreeMem(fh, sizeof (struct filehandle));
	
    }
    else
    	err = ERROR_NO_FREE_STORE;
	
    ReturnInt("open_conh", LONG, err);
}

static LONG close_con(struct conbase *conbase, struct filehandle *fh)
{
    EnterFunc(bug("close_con(fh=%p)\n", fh));
    /* Abort all pending requests */
    if (!CheckIO( ioReq(fh->conio) ))
    	AbortIO( ioReq(fh->conio) );
	
    /* Wait for abort */
    WaitIO( ioReq(fh->conio) );
    
    /* Clean up */
    CloseDevice((struct IORequest *)fh->conio);
    CloseWindow(fh->window);
    DeleteIORequest( ioReq(fh->conio) );
    FreeMem(fh->conmp, sizeof (struct MsgPort));
    
    FreeMem(fh, sizeof (struct filehandle));
    
    ReturnInt("close_con", LONG, 0);
}

static LONG con_read(struct conbase *conbase, struct filehandle *fh, UBYTE *buffer, LONG *length_ptr)
{
    LONG err;
    
    fh->conio->io_Command = CMD_READ;
    fh->conio->io_Data	  = buffer;
    fh->conio->io_Length  = *length_ptr;
	
    fh->conmp->mp_SigTask = FindTask(NULL);
    fh->conmp->mp_SigBit = SIGB_DOS;
    
    err = DoIO( ioReq(fh->conio) );
    *length_ptr = fh->conio->io_Actual;

    return err;
}

static LONG con_write(struct conbase *conbase, struct filehandle *fh, UBYTE *buffer, LONG *length_ptr)
{
    LONG err;
    
    EnterFunc(bug("con_write(fh=%p, buf=%s)\n", fh, buffer));

    
    fh->conio->io_Command = CMD_WRITE;
    fh->conio->io_Data	  = buffer;
    fh->conio->io_Length  = *length_ptr;

    fh->conmp->mp_SigTask = FindTask(NULL);
    fh->conmp->mp_SigBit = SIGB_DOS;
    
    err = DoIO( ioReq(fh->conio) );
    
    *length_ptr = fh->conio->io_Actual;
    
   ReturnInt("con_write", LONG, err);
}

/************************ Library entry points ************************/

/************
** init()  **
************/

AROS_LH2(struct conbase *, init,
 AROS_LHA(struct conbase *, conbase, D0),
 AROS_LHA(BPTR,              segList,   A0),
	   struct ExecBase *, sysBase, 0, con_handler)
{
    AROS_LIBFUNC_INIT
    
    struct DeviceNode *dn;

    /* Store arguments */
    conbase->sysbase=sysBase;
    conbase->seglist=segList;
    conbase->device.dd_Library.lib_OpenCnt=1;    

    
    conbase->dosbase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
    if (conbase->dosbase)
    {
    	conbase->intuibase = (IntuiBase *)OpenLibrary("intuition.library", 37);
	if (conbase->intuibase)
	{
	    /* Install CON: handler into device list */
    	    dn = AllocMem(sizeof (struct DeviceNode), MEMF_CLEAR|MEMF_PUBLIC);
    	    if (dn)
    	    {
    	    	STRPTR s;
	    	s = AllocVec(5, MEMF_PUBLIC);
    	    	if (s)
	    	{
	    	    CopyMem("CON", &s[1], 3);
	    	    s[4] = 0;
	    	    *s = 3;
	    	    dn->dn_Type		= DLT_DEVICE;
	    	    dn->dn_Unit		= NULL;
	    	    dn->dn_Device	= &conbase->device;
	    	    dn->dn_Handler	= NULL;
	    	    dn->dn_Startup	= NULL;
	    	    dn->dn_OldName	= MKBADDR(s);
	    	    dn->dn_NewName	= &s[1];
	    
	    	    if (AddDosEntry((struct DosList *)dn))
		    {

	    	    	return conbase;
	    	    }
	    	    FreeVec(s);
	    
	    	}
	
	   	FreeMem(dn, sizeof (struct DeviceNode));
	    }
	    
	    CloseLibrary((struct Library *)conbase->intuibase);
 	
    	} /* if (intuition opened) */
	
	CloseLibrary((struct Library *)conbase->dosbase);
   	/* Alert(AT_DeadEnd|AG_NoMemory|AN_Unknown); */
	
    } /* if (dos opened) */

    return NULL;

    AROS_LIBFUNC_EXIT
}

/*************
**  open()  **
*************/

AROS_LH3(void, open,
 AROS_LHA(struct IOFileSys *, iofs, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct conbase *, conbase, 1, con_handler)
{
    AROS_LIBFUNC_INIT

    /* Keep compiler happy */
    unitnum=0;
    flags=0;

/*    if(conbase->dosbase == NULL)
    {
	conbase->dosbase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
	if( conbase->dosbase == NULL )
	{
	    iofs->IOFS.io_Error = IOERR_OPENFAIL;
	    return;
	}
    }

    
*/
   /* I have one more opener. */
    conbase->device.dd_Library.lib_Flags&=~LIBF_DELEXP;

    /* Set returncode */
    iofs->IOFS.io_Error=0;

    /* Mark Message as recently used. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    AROS_LIBFUNC_EXIT
}

/**************
**  close()  **
**************/
AROS_LH1(BPTR, close,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct conbase *, conbase, 2, con_handler)
{
    AROS_LIBFUNC_INIT

    /* Let any following attemps to use the device crash hard. */
    iofs->IOFS.io_Device=(struct Device *)-1;
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************
**  expunge()  **
****************/
AROS_LH0(BPTR, expunge, struct conbase *, conbase, 3, con_handler)
{
    AROS_LIBFUNC_INIT

    /* Do not expunge the device. Set the delayed expunge flag and return. */
    conbase->device.dd_Library.lib_Flags|=LIBF_DELEXP;
    return 0;
    AROS_LIBFUNC_EXIT
}

/*************
**  null()  **
*************/
AROS_LH0I(int, null, struct conbase *, conbase, 4, con_handler)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************
**  beginio()  **
****************/
AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct conbase *, conbase, 5, con_handler)
{
    AROS_LIBFUNC_INIT
    LONG error=0;
    
    EnterFunc(bug("conhandler_BeginIO(iofs=%p)\n", iofs));

    /* WaitIO will look into this */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_MESSAGE;

    /*
	Do everything quick no matter what. This is possible
	because I never need to Wait().
    */
    
    D(bug("Doing command %d\n", iofs->IOFS.io_Command));
    
    switch(iofs->IOFS.io_Command)
    {
	case FSA_OPEN_FILE:
	case FSA_OPEN:
	    error = open_con(conbase,
			    (struct filehandle **)&iofs->IOFS.io_Unit,
			    iofs->io_Union.io_OPEN.io_Filename,
			    iofs->io_Union.io_OPEN.io_FileMode);
	    break;
	    
	case FSA_CLOSE:	
	    error = close_con(conbase, (struct filehandle *)iofs->IOFS.io_Unit);
	    break;
	    
	

	case FSA_READ:
 	    error = con_read(	conbase,
	    			(struct filehandle *)iofs->IOFS.io_Unit,
				iofs->io_Union.io_READ.io_Buffer,
				&(iofs->io_Union.io_READ.io_Length));
	    break;


	case FSA_WRITE:
	    error = con_write(	conbase,
	    			(struct filehandle *)iofs->IOFS.io_Unit,
				iofs->io_Union.io_WRITE.io_Buffer,
				&(iofs->io_Union.io_WRITE.io_Length));
	    break;
				
	case FSA_SEEK:
	case FSA_SET_FILE_SIZE:
	    error = ERROR_NOT_IMPLEMENTED;
	    break;
	    
	case FSA_WAIT_CHAR:
	    /* We could manually wait for a character to arrive, but this is
	       currently not implemented. FIXME */
	case FSA_FILE_MODE:
#warning FIXME: not supported yet
	    error=ERROR_ACTION_NOT_KNOWN;
	    break;

	case FSA_IS_INTERACTIVE:
	    iofs->io_Union.io_IS_INTERACTIVE.io_IsInteractive = TRUE;
	    error = 0;
	    break;

	case FSA_SAME_LOCK: 
	case FSA_EXAMINE:
	case FSA_EXAMINE_NEXT:
	case FSA_EXAMINE_ALL:
	case FSA_EXAMINE_ALL_END:
	case FSA_CREATE_DIR:
	case FSA_CREATE_HARDLINK:
	case FSA_CREATE_SOFTLINK:
	case FSA_RENAME:
        case FSA_READ_SOFTLINK:
	case FSA_DELETE_OBJECT:
	case FSA_PARENT_DIR:
        case FSA_PARENT_DIR_POST:
	case FSA_SET_COMMENT:
	case FSA_SET_PROTECT:
	case FSA_SET_OWNER:
	case FSA_SET_DATE:
	case FSA_IS_FILESYSTEM:
	case FSA_MORE_CACHE:
	case FSA_FORMAT:
	case FSA_MOUNT_MODE:
	    error = ERROR_ACTION_NOT_KNOWN;

	default:
	    error=ERROR_ACTION_NOT_KNOWN;
	    break;
    }

    /* Set error code */
    iofs->io_DosError=error;

    /* If the quick bit is not set send the message to the port */
    if(!(iofs->IOFS.io_Flags&IOF_QUICK))
	ReplyMsg(&iofs->IOFS.io_Message);
	
    ReturnVoid("conhandler_beginio");

    AROS_LIBFUNC_EXIT
}

/****************
**  abortio()  **
****************/
AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct conbase *, conbase, 6, con_handler)
{
    AROS_LIBFUNC_INIT
    /* Everything already done. */
    return 0;
    AROS_LIBFUNC_EXIT
}

static const char end=0;
