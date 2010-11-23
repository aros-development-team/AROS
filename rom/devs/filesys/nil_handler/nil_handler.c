/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang:
*/

#include <aros/atomic.h>
#include <aros/debug.h>
#include <exec/errors.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <libraries/expansion.h>
#include <proto/exec.h>
#include <proto/expansion.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

#include <string.h>

#include "nil_handler.h"

static int OpenDev(LIBBASETYPEPTR nilbase, struct IOFileSys *iofs);

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR nilbase)
{
    /*
     * Modules compiled with noexpunge always have seglist == NULL
     * The seglist is not kept as it is not needed because the module never
     * can be expunged
     */
    struct DeviceNode *dn;
    const char *devName = "NIL";
    const char *hndName = "nil.handler";

    /* Install NIL: handler into device list */
    if((dn = AllocMem(sizeof (struct DeviceNode) + 6 +
		      AROS_BSTR_MEMSIZE4LEN(strlen(devName)) +
		      AROS_BSTR_MEMSIZE4LEN(strlen(hndName)),
                      MEMF_CLEAR|MEMF_PUBLIC)))
    {
	BSTR str = (BSTR)MKBADDR(((IPTR)dn + sizeof(struct DeviceNode) + 3) & ~3);

        strcpy(AROS_BSTR_ADDR(str), devName);
        AROS_BSTR_setstrlen(str, strlen(devName));

	dn->dn_Name = str;
	dn->dn_Ext.dn_AROS.dn_DevName = AROS_BSTR_ADDR(str);

	str = (BSTR)MKBADDR(((IPTR)AROS_BSTR_ADDR(str) + AROS_BSTR_MEMSIZE4LEN(strlen(devName)) + 3) & ~3);

        strcpy(AROS_BSTR_ADDR(str), hndName);
        AROS_BSTR_setstrlen(str, strlen(hndName));

	dn->dn_Handler = str;

        dn->dn_Type    = DLT_DEVICE;

	/* Since dn_Device is NULL, the handler will be initialized
	   by dos.library on first access */
        if (AddDosEntry((struct DosList *)dn))
            return TRUE;

        FreeMem(dn, sizeof (struct DeviceNode));
    }
    return FALSE;
}

static int GM_UNIQUENAME(Open)
(
    LIBBASETYPEPTR nilbase,
    struct IOFileSys *iofs,
    ULONG unitnum,
    ULONG flags
)
{
    /* Mark Message as recently used. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;

    if (OpenDev(nilbase, iofs))
	return TRUE;

    iofs->IOFS.io_Error=IOERR_OPENFAIL;

    return FALSE;
}

static int OpenDev(LIBBASETYPEPTR nilbase, struct IOFileSys *iofs)
{
    struct nil_unit *dev;
    
    dev = AllocMem(sizeof(struct nil_unit), MEMF_PUBLIC);
    if(dev!=NULL)
    {
	dev->use = 0;
	D(bug("[NIL] Device name: %s\n", iofs->io_Union.io_OpenDevice.io_DosName));
	if (strcmp(iofs->io_Union.io_OpenDevice.io_DosName, "NIL"))
	    dev->chr = 0;
	else
	    dev->chr = CHAR_EOF;

        iofs->IOFS.io_Unit   = (struct Unit *)dev;
        iofs->IOFS.io_Device = (struct Device *)nilbase;
    	iofs->IOFS.io_Error = 0;

    	return TRUE;
    }
    else
    {
	iofs->io_DosError=ERROR_NO_FREE_STORE;
	return FALSE;
    }
}


static int GM_UNIQUENAME(Close)(LIBBASETYPEPTR nilbase, struct IOFileSys *iofs)
{
    struct nil_unit *dev = (struct nil_unit *)iofs->IOFS.io_Unit;

    if (dev->use)
    {
	iofs->io_DosError=ERROR_OBJECT_IN_USE;
	return FALSE;
    }

    /* Let any following attemps to use the device crash hard. */
    FreeMem(dev, sizeof(struct nil_unit));
    iofs->io_DosError=0;

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init),0)
ADD2OPENDEV(GM_UNIQUENAME(Open),0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close),0)

AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	 LIBBASETYPEPTR, nilbase, 5, Nil)
{
    AROS_LIBFUNC_INIT
    LONG error=0;
    struct nil_unit *unit;

    /*
	Do everything quick no matter what. This is possible
	because I never need to Wait().
    */
    switch(iofs->IOFS.io_Command)
    {
	case FSA_OPEN:
	case FSA_OPEN_FILE:
	    /* No names allowed on NIL: */
	    if (iofs->io_Union.io_NamedFile.io_Filename[0])
	    {
		error=ERROR_OBJECT_NOT_FOUND;
		break;
	    }

	    unit = (struct nil_unit *)iofs->IOFS.io_Unit;
	    AROS_ATOMIC_INC(unit->use);

	    break;

	case FSA_READ:
	    unit = (struct nil_unit *)iofs->IOFS.io_Unit;

	    if (unit->chr == CHAR_EOF)
		iofs->io_Union.io_READ.io_Length=0;
	    else
		memset(iofs->io_Union.io_READ.io_Buffer, unit->chr, iofs->io_Union.io_READ.io_Length);
	    
	    break;

	case FSA_WRITE:
	    break;

	case FSA_SEEK:
	    iofs->io_Union.io_SEEK.io_Offset = 0;
	    break;

	case FSA_CLOSE:
	    unit = (struct nil_unit *)iofs->IOFS.io_Unit;
	    AROS_ATOMIC_DEC(unit->use);
	    break;

	case FSA_IS_INTERACTIVE:
	    iofs->io_Union.io_IS_INTERACTIVE.io_IsInteractive = TRUE;
	    break;
	case FSA_SET_FILE_SIZE:
        case FSA_EXAMINE:
        case FSA_EXAMINE_NEXT:
        case FSA_EXAMINE_ALL:
        case FSA_CREATE_DIR:
        case FSA_CREATE_HARDLINK:
        case FSA_CREATE_SOFTLINK:
        case FSA_RENAME:
        case FSA_DELETE_OBJECT:
            error = ERROR_NOT_IMPLEMENTED;
            break;

	default:
	    error = ERROR_ACTION_NOT_KNOWN;
	    break;
    }

    /* Set error code */
    iofs->io_DosError=error;

    /* If the quick bit is not set send the message to the port */
    if(!(iofs->IOFS.io_Flags&IOF_QUICK))
	ReplyMsg(&iofs->IOFS.io_Message);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	 LIBBASETYPEPTR, nilbase, 6, Nil)
{
    AROS_LIBFUNC_INIT
    /* Everything already done. */
    return 0;
    AROS_LIBFUNC_EXIT
}
