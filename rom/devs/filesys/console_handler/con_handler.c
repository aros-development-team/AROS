/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Filesystem that uses console device for input/output.
    Lang: english
*/

/****************************************************************************************/

/* 
   Please always update the version-string below, if you modify the code!
*/

/* AROS includes */


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
#include <intuition/intuition.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <devices/conunit.h>
#include <aros/asmcall.h>

#include <stddef.h>
#include <string.h>

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "con_handler_intern.h"
#include "support.h"

/****************************************************************************************/

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

/****************************************************************************************/

int con_handler_entry(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

/****************************************************************************************/

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

static const char version[]="$VER: con-handler 41.2 (03.3.2002)\n";

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

/****************************************************************************************/

#define ioReq(x) ((struct IORequest *)x)

/****************************************************************************************/

AROS_UFH3(struct conbase *, AROS_SLIB_ENTRY(init,con_handler),
    AROS_UFHA(struct conbase *,	    conbase, D0),
    AROS_UFHA(BPTR,		    segList, A0),
    AROS_UFHA(struct ExecBase *,    sysBase, A6))
{
    AROS_USERFUNC_INIT
 
    static const char *devnames[2] = { "CON", "RAW" };
    struct DeviceNode *dn;
    int     	      i;


    /* Store arguments */
    conbase->sysbase = sysBase;
    conbase->seglist = segList;
    conbase->device.dd_Library.lib_OpenCnt = 1;

    conbase->dosbase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
    if (conbase->dosbase)
    {
    	conbase->intuibase = (IntuiBase *)OpenLibrary("intuition.library", 37);
	if (conbase->intuibase)
	{
	    /* Really bad hack, but con_handler is in ROM, intuition.library is
	       open, if intuition.library is open, then Input.Device must be
	       open, too, ... and I don't like to OpenDevice just for Peek-
	       Qualifier */

	    #warning InputDevice open hack. Hope this is not a problem since it is only used for PeekQualifier
	    Forbid();
	    conbase->inputbase = (struct Device *)FindName(&conbase->sysbase->DeviceList, "input.device");
	    Permit();

	    /* Install CON: and RAW: handlers into device list
	     *
	     * KLUDGE: con-handler should create only one device node, depending on
	     * the startup packet it gets. The mountlists for CON:/RAW: should be into dos.library bootstrap
	     * routines.
	     */
	    for(i = 0; i < 2; i++)
	    {
		if((dn = AllocMem(sizeof (struct DeviceNode) + 4 + 3 + 2, MEMF_CLEAR|MEMF_PUBLIC)))
		{
		    STRPTR s = (STRPTR)(((IPTR)dn + sizeof(struct DeviceNode) + 4) & ~3);
		    WORD   a;

		    for(a = 0; a < 3; a++)
		    {
		    	AROS_BSTR_putchar(s, a, devnames[i][a]);
		    }
		    AROS_BSTR_setstrlen(s, 3);

		    dn->dn_Type		= DLT_DEVICE;

		    /* 
		       i equals 1 when dn_NewName is "RAW", and 0 otherwise. This
		       tells con_task that it has to start in RAW mode
		    */   
		    dn->dn_Unit		= (struct Unit *)i;

		    dn->dn_Device	= &conbase->device;
		    dn->dn_Handler	= NULL;
		    dn->dn_Startup	= NULL;
		    dn->dn_OldName	= MKBADDR(s);
		    dn->dn_NewName	= AROS_BSTR_ADDR(dn->dn_OldName);

		    if (AddDosEntry((struct DosList *)dn))
		    {
			if (i == 0)
			    continue;

			return conbase;
		    }

	   	    FreeMem(dn, sizeof (struct DeviceNode));
	    	}
	    }

	    CloseLibrary((struct Library *)conbase->intuibase);

    	} /* if (intuition opened) */

	CloseLibrary((struct Library *)conbase->dosbase);
   	/* Alert(AT_DeadEnd|AG_NoMemory|AN_Unknown); */

    } /* if (dos opened) */

    return NULL;

    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

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

/*
    if(conbase->dosbase == NULL)
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

    /*
       Check whether the user mounted us as "RAW", in which case abuse of the
       io_Unit field in the iofs structure to tell the con task that it has
       to start in RAW mode
    */
    if (strncasecmp("RAW", iofs->io_Union.io_OpenDevice.io_DosName, 4))
        iofs->IOFS.io_Unit = (struct Unit *)1;

    /* Set returncode */
    iofs->IOFS.io_Error=0;

    /* Mark Message as recently used. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

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

/****************************************************************************************/

AROS_LH0(BPTR, expunge, struct conbase *, conbase, 3, con_handler)
{
    AROS_LIBFUNC_INIT

    /* Do not expunge the device. Set the delayed expunge flag and return. */
    conbase->device.dd_Library.lib_Flags|=LIBF_DELEXP;
    return 0;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0I(int, null, struct conbase *, conbase, 4, con_handler)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

static LONG open_con(struct conbase *conbase, struct IOFileSys *iofs)
{
    struct filehandle 	    *fh = (struct filehandle *)iofs->IOFS.io_Unit;
#if DEBUG
    STRPTR  	    	    filename = iofs->io_Union.io_OPEN.io_Filename;
    ULONG   	    	    mode = iofs->io_Union.io_OPEN.io_FileMode;
#endif
    struct conTaskParams    params;
    struct Task     	    *contask;
    LONG    	    	    err = 0;

    EnterFunc(bug("open_conh(filename=%s, mode=%d)\n",
    	filename, mode));

    if (fh != NULL && fh != (struct filehandle *)1)
    {
        /* DupLock */
	fh->usecount++;
    }
    else
    {
    	UBYTE sig = AllocSignal(-1);
	
	if (sig == -1)
	{
	    iofs->io_DosError = ERROR_NO_FREE_STORE; /* Any other error code better suited here? */
	}
	else
	{
	    params.conbase = conbase;
	    params.iofs = iofs;
	    params.parentTask = FindTask(NULL);
	    params.initSignal = 1L << sig;

	    contask = createConTask(&params, conbase);
	    if (contask)
	    {
		Wait(params.initSignal);
		if (iofs->io_DosError)
		{
	    	    RemTask(contask);
		}
	    }
	    
	    FreeSignal(sig);
	}	
	err = iofs->io_DosError;
    }

    ReturnInt("open_conh", LONG, err);
}

/****************************************************************************************/

AROS_LH1(void, beginio,
    AROS_LHA(struct IOFileSys *, iofs, A1),
    struct conbase *, conbase, 5, con_handler)
{
    AROS_LIBFUNC_INIT

    LONG error = 0;
    BOOL request_queued = FALSE;

    EnterFunc(bug("conhandler_BeginIO(iofs=%p)\n", iofs));

    /* WaitIO will look into this */
    iofs->IOFS.io_Message.mn_Node.ln_Type = NT_MESSAGE;

    /*
	Do everything quick no matter what. This is possible
	because I never need to Wait().
    */
    
    D(bug("Doing command %d\n", iofs->IOFS.io_Command));
    
    switch(iofs->IOFS.io_Command)
    {
	case FSA_OPEN_FILE:
	case FSA_OPEN:
	    error = open_con(conbase, iofs);
	    break;

	case FSA_CLOSE:
        case FSA_READ:
	case FSA_WRITE:
	case FSA_CONSOLE_MODE:
	case FSA_CHANGE_SIGNAL:
	    iofs->IOFS.io_Flags	&= ~IOF_QUICK;
	    request_queued = TRUE;

	    PutMsg(((struct filehandle *)iofs->IOFS.io_Unit)->contaskmp,
	           (struct Message *)iofs);
	    break;

	case FSA_IS_INTERACTIVE:
	    iofs->io_Union.io_IS_INTERACTIVE.io_IsInteractive = TRUE;
	    error = 0;
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

	case FSA_EXAMINE:
        {
            struct ExAllData  *ead        = iofs->io_Union.io_EXAMINE.io_ead;
            const ULONG        type       = iofs->io_Union.io_EXAMINE.io_Mode;
            const ULONG        size       = iofs->io_Union.io_EXAMINE.io_Size;
            STRPTR             next, end;

            static const ULONG sizes[]=
            {
                0,
                offsetof(struct ExAllData,ed_Type),
                offsetof(struct ExAllData,ed_Size),
                offsetof(struct ExAllData,ed_Prot),
                offsetof(struct ExAllData,ed_Days),
                offsetof(struct ExAllData,ed_Comment),
                offsetof(struct ExAllData,ed_OwnerUID),
                sizeof(struct ExAllData)
             };

	     next = (STRPTR)ead + sizes[type];
             end  = (STRPTR)ead + size;

             if (type > ED_OWNER)
             {
                 error = ERROR_BAD_NUMBER;
                 break;
             }

             switch(type)
             {
                 case ED_OWNER:
                     ead->ed_OwnerUID = 0;
                     ead->ed_OwnerGID = 0;

                 /* Fall through */
                 case ED_COMMENT:
                     ead->ed_Comment = NULL;

                 /* Fall through */
                 case ED_DATE:
                     ead->ed_Days  = 0;
                     ead->ed_Mins  = 0;
                     ead->ed_Ticks = 0;

		 /* Fall through */
                 case ED_PROTECTION:
                     ead->ed_Prot = 0;

                 /* Fall through */
                 case ED_SIZE:
                     ead->ed_Size = 0;

                 /* Fall through */
                 case ED_TYPE:
                     ead->ed_Type = ST_PIPEFILE;

		 /* Fall through */
                 case ED_NAME:
                     if (next >= end)
		     {
		         error = ERROR_BUFFER_OVERFLOW;
                         break;
		     }

                     ead->ed_Name = next;
		     *next = '\0';
	    }
        }
	break;


        case FSA_SAME_LOCK:
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
    if(!(iofs->IOFS.io_Flags&IOF_QUICK) && !request_queued)
	ReplyMsg(&iofs->IOFS.io_Message);

    ReturnVoid("conhandler_beginio");

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(LONG, abortio,
    AROS_LHA(struct IOFileSys *, iofs, A1),
    struct conbase *, conbase, 6, con_handler)
{
    AROS_LIBFUNC_INIT
    
    /* Everything already done. */
    return 0;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

static const char end = 0;

/****************************************************************************************/
