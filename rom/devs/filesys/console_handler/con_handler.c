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
#include <aros/symbolsets.h>

#include <stddef.h>
#include <string.h>

#undef SDEBUG
#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "con_handler_intern.h"
#include "support.h"

#include LC_LIBDEFS_FILE

/****************************************************************************************/

#define ioReq(x) ((struct IORequest *)x)

/****************************************************************************************/

AROS_SET_LIBFUNC(GM_UNIQUENAME(Init), LIBBASETYPE, conbase)
{
    AROS_SET_LIBFUNC_INIT
 
    static const char *devnames[2] = { "CON", "RAW" };
    struct DeviceNode *dn;
    int     	      i;


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

		return TRUE;
	    }

	    FreeMem(dn, sizeof (struct DeviceNode));
	}
    }

    return FALSE;

    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_OPENDEVFUNC(GM_UNIQUENAME(Open),
		     LIBBASETYPE, conbase,
		     struct IOFileSys, iofs,
		     unitnum,
		     flags
)
{
    AROS_SET_DEVFUNC_INIT

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
    
    return TRUE;
    
    AROS_SET_DEVFUNC_EXIT
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init),0)
ADD2OPENDEV(GM_UNIQUENAME(Open),0)

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
	
	if (sig == (UBYTE)-1)
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
    struct conbase *, conbase, 5, Con)
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
    struct conbase *, conbase, 6, Con)
{
    AROS_LIBFUNC_INIT
    
    /* Everything already done. */
    return 0;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/
