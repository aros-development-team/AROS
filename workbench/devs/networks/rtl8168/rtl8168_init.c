/*
 * $Id: rtl8168_init.c 29806 2008-10-18 10:52:30Z verhaegs $
 */

/*
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston,
	MA 02111-1307, USA.
*/

#include "rtl8168.h"

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/lists.h>

#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <oop/oop.h>

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>

#include <utility/utility.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <hidd/pci.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <stdio.h>

#include "unit.h"
#include LC_LIBDEFS_FILE

AROS_UFH3(void, PCI_Enumerator,
	AROS_UFHA(struct Hook *,    hook,       A0),
	AROS_UFHA(OOP_Object *,     pciDevice,  A2),
	AROS_UFHA(APTR,             message,    A1))
{
    AROS_USERFUNC_INIT

    struct RTL8168Unit	*unit = NULL;
    IPTR   		RevisionID;

D(bug("[rtl8168] PCI_Enumerator(PCI Device Obj @ %p)\n", pciDevice));

    LIBBASETYPEPTR LIBBASE = (LIBBASETYPEPTR)hook->h_Data;

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_RevisionID, &RevisionID);

D(bug("[rtl8168] PCI_Enumerator: Found RTL8168 NIC Rev:%d\n", RevisionID));

    if ((LIBBASE->rtl8168b_UnitCount < MAX_UNITS) && ((unit = CreateUnit(LIBBASE, pciDevice, RevisionID)) != NULL))
    {
	AddTail(&LIBBASE->rtl8168b_Units, &unit->rtl8168u_Node);
    }
    else if (LIBBASE->rtl8168b_UnitCount < MAX_UNITS)
    {
D(bug("[rtl8168] PCI_Enumerator: Failed to create unit!\n"));
	return;
    }
    else
    {
D(bug("[rtl8168] PCI_Enumerator: Max supported units already reached\n"));
	return;
    }
RTLD(bug("[%s] PCI_Enumerator: %s NIC MMIO @ %p\n", unit->rtl8168u_name, unit->rtl8168u_rtl_chipname, unit->rtl8168u_BaseMem))

    AROS_USERFUNC_EXIT
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
D(bug("[rtl8168] Init()\n"));

    UBYTE tmpbuff[100];
    int i;

    sprintf((char *)tmpbuff, RTL8168_TASK_NAME, "rtl8168.0");

    if (FindTask(tmpbuff) != NULL)
    {
	D(bug("[rtl8168] Init: Found Task '%s'! - Device already up and running.\n", tmpbuff));
	return FALSE;
    }

    /* Load config options */
    LIBBASE->rtl8168b_MaxIntWork = 20;
    LIBBASE->rtl8168b_MulticastFilterLimit = 32;

    for (i = 0; i < MAX_UNITS; i++)
    {
	LIBBASE->speed[i] = -1;
	LIBBASE->duplex[i] = -1;
	LIBBASE->autoneg[i] = -1;
    }

    NEWLIST(&LIBBASE->rtl8168b_Units);

    LIBBASE->rtl8168b_PCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    if (LIBBASE->rtl8168b_PCIDeviceAttrBase != 0)
    {
        D(bug("[rtl8168] Init: HiddPCIDeviceAttrBase @ %p\n", LIBBASE->rtl8168b_PCIDeviceAttrBase));

        LIBBASE->rtl8168b_PCI = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

        if (LIBBASE->rtl8168b_PCI)
        {
            D(bug("[rtl8168] Init: PCI Subsystem HIDD object @ %p\n", LIBBASE->rtl8168b_PCI));

	    struct TagItem Requirements[] = {
		{tHidd_PCI_VendorID,	0x10ec},
		{tHidd_PCI_ProductID,   0x8168},
		{TAG_DONE,              0x00}
	    };

            struct Hook FindHook = {
                h_Entry:    (IPTR (*)())PCI_Enumerator,
                h_Data:     LIBBASE,
            };

	    struct pHidd_PCI_EnumDevices enummsg = {
		mID:            OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
		callback:       &FindHook,
		requirements:   (struct TagItem *)&Requirements,
	    }, *msg = &enummsg;

	    OOP_DoMethod(LIBBASE->rtl8168b_PCI, (OOP_Msg)msg);

            if (!(IsListEmpty(&LIBBASE->rtl8168b_Units)))
            {
                return TRUE;
            }
	}
    }
    return FALSE;
}

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE)
{
D(bug("[rtl8168] Expunge()\n"));

    struct RTL8168Unit *unit_current, *unit_tmp;

    if (!(IsListEmpty(&LIBBASE->rtl8168b_Units)))
    {
        ForeachNodeSafe(&LIBBASE->rtl8168b_Units, unit_current, unit_tmp)
        {
            DeleteUnit(LIBBASE, unit_current);
        }
    }

    if (LIBBASE->rtl8168b_PCIDeviceAttrBase != 0)
        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);

    LIBBASE->rtl8168b_PCIDeviceAttrBase = 0;

    if (LIBBASE->rtl8168b_PCI != NULL)
        OOP_DisposeObject(LIBBASE->rtl8168b_PCI);

    return TRUE;
}

static const ULONG rx_tags[] = {
    S2_CopyToBuff,
    S2_CopyToBuff16
};

static const ULONG tx_tags[] = {
    S2_CopyFromBuff,
    S2_CopyFromBuff16,
    S2_CopyFromBuff32
};

static int GM_UNIQUENAME(Open)
(
    LIBBASETYPEPTR LIBBASE,
    struct IOSana2Req* req,
    ULONG unitnum,
    ULONG flags
)
{
    struct TagItem *tags;
    struct RTL8168Unit *unit = NULL, *unit_current;
    struct Opener *opener;
    BYTE error=0;
    int i;

    if (!(IsListEmpty(&LIBBASE->rtl8168b_Units)))
    {
        ForeachNode(&LIBBASE->rtl8168b_Units, unit_current)
        {
            if (unit_current->rtl8168u_UnitNum == unitnum)
                unit = unit_current;
        }
    }

D(bug("[rtl8168] OpenDevice(%d)\n", unitnum));

    if (unit != NULL)
    {
RTLD(bug("[%s] OpenDevice: Unit %d @ %p\n", unit->rtl8168u_name, unitnum, unit))

	req->ios2_Req.io_Unit = NULL;
	tags = req->ios2_BufferManagement;

	req->ios2_BufferManagement = NULL;

	/* Check request size */
	if (req->ios2_Req.io_Message.mn_Length < sizeof(struct IOSana2Req))
	    error = IOERR_OPENFAIL;

	/* Get the requested unit */
	if (error == 0)
	    req->ios2_Req.io_Unit = (APTR)unit;

	/* Handle device sharing */
	if (error == 0)
	{
	    if ((unit->rtl8168u_open_count != 0) && 
	       ((unit->rtl8168u_flags & IFF_SHARED) == 0 ||
	       (flags & SANA2OPF_MINE) != 0))
		error = IOERR_UNITBUSY;
	    else
		unit->rtl8168u_open_count++;
	}

	if (error == 0)
	{
	    if ((flags & SANA2OPF_MINE) == 0)
		unit->rtl8168u_flags |= IFF_SHARED;
	    else if ((flags & SANA2OPF_PROM) != 0)
		unit->rtl8168u_flags |= IFF_PROMISC;

	    /* Set up buffer-management structure and get hooks */
	    opener = AllocVec(sizeof(struct Opener), MEMF_PUBLIC | MEMF_CLEAR);
	    req->ios2_BufferManagement = (APTR)opener;

	    if(opener == NULL)
		error = IOERR_OPENFAIL;
	}

	if (error == 0)
	{
	    NEWLIST(&opener->read_port.mp_MsgList);
	    opener->read_port.mp_Flags = PA_IGNORE;
	    NEWLIST((APTR)&opener->initial_stats);

	    for (i = 0; i < 2; i++)
		opener->rx_function = (APTR)GetTagData(rx_tags[i], (IPTR)opener->rx_function, tags);
	    for (i = 0; i < 3; i++)
		opener->tx_function = (APTR)GetTagData(tx_tags[i], (IPTR)opener->tx_function, tags);

	    opener->filter_hook = (APTR)GetTagData(S2_PacketFilter, 0, tags);

	    Disable();
	    AddTail((APTR)&unit->rtl8168u_Openers, (APTR)opener);
	    Enable();
	}

	if (error != 0)
	    CloseDevice((struct IORequest *)req);
	else
	    unit->start(unit);
    }
    else
    {
D(bug("[rtl8168] OpenDevice: Invalid Unit! (unitno = %d)\n", unitnum));
        error = IOERR_OPENFAIL;
    }

    req->ios2_Req.io_Error = error;

    return (error !=0) ? FALSE : TRUE;
}

static int GM_UNIQUENAME(Close)
(
    LIBBASETYPEPTR LIBBASE,
    struct IOSana2Req* req
)
{
    struct RTL8168Unit *unit = (struct RTL8168Unit *)req->ios2_Req.io_Unit;
    struct Opener *opener;

RTLD(bug("[rtl8168] CloseDevice(unit @ %p, unitno %d)\n", unit, unit->rtl8168u_UnitNum))

    unit->stop(unit);

    opener = (APTR)req->ios2_BufferManagement;
    if (opener != NULL)
    {
	Disable();
	Remove((struct Node *)opener);
	Enable();
	FreeVec(opener);
    }

    return TRUE;
}


ADD2INITLIB(GM_UNIQUENAME(Init),0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge),0)
ADD2OPENDEV(GM_UNIQUENAME(Open),0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close),0)

AROS_LH1(void, BeginIO,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 5, RTL8168Dev)
{
    AROS_LIBFUNC_INIT
    struct RTL8168Unit *unit;

RTLD(bug("[rtl8168] BeginIO()\n"))

    req->ios2_Req.io_Error = 0;
    unit = (APTR)req->ios2_Req.io_Unit;

    if (AttemptSemaphore(&unit->rtl8168u_unit_lock))
    {
	    handle_request(LIBBASE, req);
    }
    else
    {
	    req->ios2_Req.io_Flags &= ~IOF_QUICK;
	    PutMsg(unit->rtl8168u_input_port, (struct Message *)req);
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 6, RTL8168Dev)
{
    AROS_LIBFUNC_INIT
    struct RTL8168Unit *unit;
    unit = (APTR)req->ios2_Req.io_Unit;

RTLD(bug("[rtl8168] AbortIO()\n"))

    Disable();
    if ((req->ios2_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE) &&
       (req->ios2_Req.io_Flags & IOF_QUICK) == 0)
    {
	Remove((struct Node *)req);
	req->ios2_Req.io_Error = IOERR_ABORTED;
	req->ios2_WireError = S2WERR_GENERIC_ERROR;
	ReplyMsg((struct Message *)req);
    }
    Enable();

    return 0;

    AROS_LIBFUNC_EXIT
}
