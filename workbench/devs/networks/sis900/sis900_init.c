/*
 * $Id$
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

#include "sis900.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

AROS_UFH3(void, PCI_Enumerator,
    AROS_UFHA(struct Hook *,    hook,       A0),
    AROS_UFHA(OOP_Object *,     pciDevice,  A2),
    AROS_UFHA(APTR,             message,    A1))
{
    AROS_USERFUNC_INIT

D(bug("[SiS900] PCI_Enumerator()\n"));

    LIBBASETYPEPTR LIBBASE = (LIBBASETYPEPTR)hook->h_Data;

	BOOL FoundCompatNIC = FALSE;
    IPTR DeviceID, VendorID, RevisionID;
	char *CardName, *CardChipName;
    struct SiS900Unit   *unit;

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &VendorID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &DeviceID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_RevisionID, &RevisionID);
	
    if ((DeviceID == 0x0900))
    {
		FoundCompatNIC = TRUE;
        CardName = "SiS 900 PCI Fast Ethernet";
		CardChipName = "SiS900";
    }
    else if ((DeviceID == 0x7016))
    {
		FoundCompatNIC = TRUE;
        CardName = "SiS 7016 PCI Fast Ethernet";
		CardChipName = "SiS7016";
    }

	if (FoundCompatNIC)
	{
D(bug("[SiS900] PCI_Enumerator: Found %s NIC, PCI_ID %04x:%04x Rev:%d\n", CardName, VendorID, DeviceID, RevisionID));

        if ((unit = CreateUnit(LIBBASE, pciDevice, CardName, CardChipName)))
        {
            AddTail(&LIBBASE->sis900b_Units, &unit->sis900u_Node);
        }
        else
        {
D(bug("[SIS900] PCI_Enumerator: Failed to create unit!\n"));
            return;
        }

D(bug("[%s] PCI_Enumerator: %s NIC I/O MEM @ %08x\n", unit->sis900u_name, unit->sis900u_rtl_chipname, unit->sis900u_BaseMem));
	}
	
    AROS_USERFUNC_EXIT
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
D(bug("[SiS900] Init()\n"));

    UBYTE tmpbuff[100];
    sprintf((char *)tmpbuff, SiS900_TASK_NAME, "sis900.0");

    if (FindTask(tmpbuff) != NULL)
    {
        D(bug("[SiS900] device already up and running.\n"));
        return FALSE;
    }

    NEWLIST(&LIBBASE->sis900b_Units);

    LIBBASE->sis900b_PCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    if (LIBBASE->sis900b_PCIDeviceAttrBase != 0)
    {
        D(bug("[SiS900] HiddPCIDeviceAttrBase @ %p\n", LIBBASE->sis900b_PCIDeviceAttrBase));

        LIBBASE->sis900b_PCI = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

        if (LIBBASE->sis900b_PCI)
        {
            D(bug("[SiS900] PCI Subsystem HIDD object @ %p\n", LIBBASE->sis900b_PCI));

            struct Hook FindHook = {
                h_Entry:    (IPTR (*)())PCI_Enumerator,
                h_Data:     LIBBASE,
            };

            struct TagItem Requirements[] = {
                { tHidd_PCI_VendorID,   0x1039  },
                { TAG_DONE,             0UL }
            };

            HIDD_PCI_EnumDevices(LIBBASE->sis900b_PCI,
                                 &FindHook,
                                 (struct TagItem *)&Requirements
            );

            if (!(IsListEmpty(&LIBBASE->sis900b_Units)))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE)
{
D(bug("[SiS900] Expunge()\n"));

    struct SiS900Unit *unit_current, *unit_tmp;

    if (!(IsListEmpty(&LIBBASE->sis900b_Units)))
    {
        ForeachNodeSafe(&LIBBASE->sis900b_Units, unit_current, unit_tmp)
        {
            DeleteUnit(LIBBASE, unit_current);
        }
    }

    if (LIBBASE->sis900b_PCIDeviceAttrBase != 0)
        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);

    LIBBASE->sis900b_PCIDeviceAttrBase = 0;

    if (LIBBASE->sis900b_PCI != NULL)
        OOP_DisposeObject(LIBBASE->sis900b_PCI);

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

/*
 * Open device handles currently only one sis900 unit.
 */
static int GM_UNIQUENAME(Open)
(
    LIBBASETYPEPTR LIBBASE,
    struct IOSana2Req* req,
    ULONG unitnum,
    ULONG flags
)
{
    struct TagItem *tags;
    struct SiS900Unit *unit = NULL, *unit_current;
    struct Opener *opener;
    BYTE error=0;
    int i;

    if (!(IsListEmpty(&LIBBASE->sis900b_Units)))
    {
        ForeachNode(&LIBBASE->sis900b_Units, unit_current)
        {
            if (unit_current->sis900u_UnitNum == unitnum)
                unit = unit_current;
        }
    }
    
D(bug("[SiS900] OpenDevice(%d)\n", unitnum));

    if (unit != NULL)
    {
D(bug("[SiS900] OpenDevice: Unit %d @ %p\n", unitnum, unit));
        req->ios2_Req.io_Unit = NULL;
        tags = req->ios2_BufferManagement;

        req->ios2_BufferManagement = NULL;

        /* Check request size */
        if (req->ios2_Req.io_Message.mn_Length < sizeof(struct IOSana2Req))
            error = IOERR_OPENFAIL;

        /* Get the requested unit */
        if (error == 0)
        {
            req->ios2_Req.io_Unit = (APTR)unit;
        }

        /* Handle device sharing */
        if (error == 0)
        {
            if(unit->sis900u_open_count != 0 && ((unit->sis900u_ifflags & IFF_SHARED) == 0 ||
                (flags & SANA2OPF_MINE) != 0))
                error = IOERR_UNITBUSY;
            unit->sis900u_open_count++;
        }

        if (error == 0)
        {
            if((flags & SANA2OPF_MINE) == 0)
            unit->sis900u_ifflags |= IFF_SHARED;
            else if((flags & SANA2OPF_PROM) != 0)
            unit->sis900u_ifflags |= IFF_PROMISC;

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

            for(i = 0; i < 2; i++)
                opener->rx_function = (APTR)GetTagData(rx_tags[i], (IPTR)opener->rx_function, tags);
            for(i = 0; i < 3; i++)
                opener->tx_function = (APTR)GetTagData(tx_tags[i], (IPTR)opener->tx_function, tags);

            opener->filter_hook = (APTR)GetTagData(S2_PacketFilter, 0, tags);

            Disable();
            AddTail((APTR)&unit->sis900u_Openers, (APTR)opener);
            Enable();
        }

        if (error != 0)
            CloseDevice((struct IORequest *)req);
        else
            sis900func_open(unit);

    }
    else
    {
D(bug("[SiS900] OpenDevice: Invalid Unit! (unitno = %d)\n", unitnum));
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
    struct SiS900Unit *unit = (struct SiS900Unit *)req->ios2_Req.io_Unit;
    struct Opener *opener;

D(bug("[SiS900] CloseDevice(unit @ %p, unitno %d)\n", unit, unit->sis900u_UnitNum));

    sis900func_close(unit);

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

AROS_LH1(void, beginio,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 5, SiS900Dev)
{
    AROS_LIBFUNC_INIT
    struct SiS900Unit *unit;

D(bug("[SiS900] BeginIO()\n"));

    req->ios2_Req.io_Error = 0;
    unit = (APTR)req->ios2_Req.io_Unit;

    if (AttemptSemaphore(&unit->sis900u_unit_lock))
    {
        handle_request(LIBBASE, req);
    }
    else
    {
        req->ios2_Req.io_Flags &= ~IOF_QUICK;
        PutMsg(unit->sis900u_input_port, (struct Message *)req);
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 6, SiS900Dev)
{
    AROS_LIBFUNC_INIT
    struct SiS900Unit *unit;
    unit = (APTR)req->ios2_Req.io_Unit;

D(bug("[SiS900] AbortIO()\n"));

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
