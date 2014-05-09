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

#include "rtl8139.h"

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

	char    *CardName,
			*CardChipName;
	IPTR    DeviceID, 
			VendorID, 
			RevisionID, 
			CardCapabilities;
	BOOL    FoundCompatNIC = FALSE;

D(bug("[rtl8139] PCI_Enumerator(PCI Device Obj @ %p)\n", pciDevice));

	LIBBASETYPEPTR LIBBASE = (LIBBASETYPEPTR)hook->h_Data;

	OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &VendorID);
	OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &DeviceID);
	OOP_GetAttr(pciDevice, aHidd_PCIDevice_RevisionID, &RevisionID);
	
	if ((VendorID == 0x10ec) && (DeviceID == 0x8129))
	{
		FoundCompatNIC = TRUE;
		CardCapabilities = RTLc_HAS_MII_XCVR;
		CardName = "RealTek RTL8129";
		CardChipName = "RTL8129";
	}
	else if ((VendorID == 0x10ec) && ((DeviceID == 0x8138) || (DeviceID == 0x8139)))
	{
		FoundCompatNIC = TRUE;
		if ((DeviceID == 0x8139) && (RevisionID >= 0x20))
		{
			CardCapabilities =   RTLc_HAS_CHIP_XCVR | RTLc_HAS_LNK_CHNG | RTLc_HAS_DESC;
			CardName = "RealTek RTL8139D";
			CardChipName = "RTL8139D";
		}
		else
		{
			CardCapabilities =   RTLc_HAS_CHIP_XCVR | RTLc_HAS_LNK_CHNG;
			CardName = "RealTek RTL8139C";
			CardChipName = "RTL8139C";
		}
	}
	else if ((VendorID == 0x1113) && (DeviceID == 0x1211))
	{
		FoundCompatNIC = TRUE;
		CardCapabilities = RTLc_HAS_CHIP_XCVR | RTLc_HAS_LNK_CHNG;
		CardName = "Accton EN-1207D Fast Ethernet";
		CardChipName = "RTL8139";
	}
	else if ((VendorID == 0x1186) && (DeviceID == 0x1300))
	{
		FoundCompatNIC = TRUE;
		CardCapabilities = RTLc_HAS_CHIP_XCVR | RTLc_HAS_LNK_CHNG;
		CardName = "D-Link DFE-538TX";
		CardChipName = "RTL8139";
	}
	else if ((VendorID == 0x018a) && (DeviceID == 0x0106))
	{
		FoundCompatNIC = TRUE;
		CardCapabilities = RTLc_HAS_CHIP_XCVR | RTLc_HAS_LNK_CHNG;
		CardName = "LevelOne FPC-0106Tx";
		CardChipName = "RTL8139";
	}
	else if ((VendorID == 0x021b) && (DeviceID == 0x8139))
	{
		FoundCompatNIC = TRUE;
		CardCapabilities = RTLc_HAS_CHIP_XCVR | RTLc_HAS_LNK_CHNG;
		CardName = "Compaq HNE-300";
		CardChipName = "RTL8139";
	}

	if (FoundCompatNIC)
	{
D(bug("[rtl8139] PCI_Enumerator: Found %s NIC [%s], PCI_ID %04x:%04x Rev:%d\n", CardName, CardChipName, VendorID, DeviceID, RevisionID));

		struct RTL8139Unit *unit = NULL;
		
		if ((unit = CreateUnit(LIBBASE, pciDevice, CardCapabilities, CardName, CardChipName)) != NULL)
		{
            AddTail(&LIBBASE->rtl8139b_Units, (struct Node *)&unit->rtl8139u_Node);
        }
        else
        {
D(bug("[rtl8139] PCI_Enumerator: Failed to create unit!\n"));
            return;
        }
RTLD(bug("[%s] PCI_Enumerator: %s NIC I/O MEM @ %08x\n", unit->rtl8139u_name, unit->rtl8139u_rtl_chipname, unit->rtl8139u_BaseMem))

	}
	
	AROS_USERFUNC_EXIT
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
D(bug("[rtl8139] Init()\n"));

    UBYTE tmpbuff[100];
    sprintf((char *)tmpbuff, RTL8139_TASK_NAME, "rtl8139.0");

    if (FindTask(tmpbuff) != NULL)
	{
		D(bug("[rtl8139] Init: Found Task '%s'! - Device already up and running.\n", tmpbuff));
		return FALSE;
	}

    NEWLIST(&LIBBASE->rtl8139b_Units);

    LIBBASE->rtl8139b_PCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    if (LIBBASE->rtl8139b_PCIDeviceAttrBase != 0)
    {
        D(bug("[rtl8139] Init: HiddPCIDeviceAttrBase @ %p\n", LIBBASE->rtl8139b_PCIDeviceAttrBase));

        LIBBASE->rtl8139b_PCI = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

        if (LIBBASE->rtl8139b_PCI)
        {
            D(bug("[rtl8139] Init: PCI Subsystem HIDD object @ %p\n", LIBBASE->rtl8139b_PCI));

            struct Hook FindHook =
            {
                h_Entry:    (IPTR (*)())PCI_Enumerator,
                h_Data:     LIBBASE,
            };

            HIDD_PCI_EnumDevices(   LIBBASE->rtl8139b_PCI,
                                    &FindHook,
                                    NULL
            );

            if (!(IsListEmpty(&LIBBASE->rtl8139b_Units)))
            {
                return TRUE;
            }
		}
    }
    return FALSE;
}

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE)
{
D(bug("[rtl8139] Expunge()\n"));

    struct RTL8139Unit *unit_current, *unit_tmp;

    if (!(IsListEmpty(&LIBBASE->rtl8139b_Units)))
    {
        ForeachNodeSafe(&LIBBASE->rtl8139b_Units, unit_current, unit_tmp)
        {
            DeleteUnit(LIBBASE, unit_current);
        }
    }

    if (LIBBASE->rtl8139b_PCIDeviceAttrBase != 0)
    {
        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
	}

    LIBBASE->rtl8139b_PCIDeviceAttrBase = 0;

    if (LIBBASE->rtl8139b_PCI != NULL)
    {
        OOP_DisposeObject(LIBBASE->rtl8139b_PCI);
	}

    return TRUE;
}

static const ULONG rx_tags[] =
{
	S2_CopyToBuff,
	S2_CopyToBuff16
};

static const ULONG tx_tags[] =
{
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
    struct RTL8139Unit *unit = NULL, *unit_current;
	struct Opener *opener;
	BYTE error=0;
	int i;

    if (!(IsListEmpty(&LIBBASE->rtl8139b_Units)))
    {
        ForeachNode(&LIBBASE->rtl8139b_Units, unit_current)
        {
            if (unit_current->rtl8139u_UnitNum == unitnum)
            {
                unit = unit_current;
			}
        }
    }

D(bug("[rtl8139] OpenDevice(%d)\n", unitnum));

    if (unit != NULL)
    {
RTLD(bug("[rtl8139] OpenDevice: Unit %d @ %p\n", unitnum, unit));
		req->ios2_Req.io_Unit = NULL;
		tags = req->ios2_BufferManagement;

		req->ios2_BufferManagement = NULL;

		/* Check request size */
		if (req->ios2_Req.io_Message.mn_Length < sizeof(struct IOSana2Req))
		{
			error = IOERR_OPENFAIL;
		}

		/* Get the requested unit */
		if (error == 0)
		{
			req->ios2_Req.io_Unit = (APTR)unit;
		}

		/* Handle device sharing */
		if (error == 0)
		{
			if (unit->rtl8139u_open_count != 0 && ((unit->rtl8139u_flags & IFF_SHARED) == 0 ||
				(flags & SANA2OPF_MINE) != 0))
			{
				error = IOERR_UNITBUSY;
			}
			else
			{
				unit->rtl8139u_open_count++;
			}
		}

		if (error == 0)
		{
			if ((flags & SANA2OPF_MINE) == 0)
			{
				unit->rtl8139u_flags |= IFF_SHARED;
			}
			else if ((flags & SANA2OPF_PROM) != 0)
			{
				unit->rtl8139u_flags |= IFF_PROMISC;
			}

			/* Set up buffer-management structure and get hooks */
			opener = AllocVec(sizeof(struct Opener), MEMF_PUBLIC | MEMF_CLEAR);
			req->ios2_BufferManagement = (APTR)opener;

			if(opener == NULL)
			{
				error = IOERR_OPENFAIL;
			}
		}

		if (error == 0)
		{
			NEWLIST(&opener->read_port.mp_MsgList);
			opener->read_port.mp_Flags = PA_IGNORE;
			NEWLIST((APTR)&opener->initial_stats);

			for (i = 0; i < 2; i++)
			{
				opener->rx_function = (APTR)GetTagData(rx_tags[i], (IPTR)opener->rx_function, tags);
			}
			for (i = 0; i < 3; i++)
			{
				opener->tx_function = (APTR)GetTagData(tx_tags[i], (IPTR)opener->tx_function, tags);
			}

			opener->filter_hook = (APTR)GetTagData(S2_PacketFilter, 0, tags);

			Disable();
			AddTail((APTR)&unit->rtl8139u_Openers, (APTR)opener);
			Enable();
		}

		if (error != 0)
			CloseDevice((struct IORequest *)req);
		else if (unit->rtl8139u_open_count == 1)
			unit->start(unit);
	}
    else
    {
D(bug("[rtl8139] OpenDevice: Invalid Unit! (unitno = %d)\n", unitnum));
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
	struct RTL8139Unit *unit = (struct RTL8139Unit *)req->ios2_Req.io_Unit;
	struct Opener *opener;

RTLD(bug("[rtl8139] CloseDevice(unit @ %p, unitno %d)\n", unit, unit->rtl8139u_UnitNum));

	opener = (APTR)req->ios2_BufferManagement;
	if (opener != NULL)
	{
		Disable();
		Remove((struct Node *)opener);
		Enable();
		FreeVec(opener);
	}

	if (--unit->rtl8139u_open_count == 0)
		unit->stop(unit);

	/* Without this, DHCP doesn't work the second time the device is used */
	((struct Library *)LIBBASE)->lib_Flags |= LIBF_DELEXP;

	return TRUE;
}


ADD2INITLIB(GM_UNIQUENAME(Init),0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge),0)
ADD2OPENDEV(GM_UNIQUENAME(Open),0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close),0)

AROS_LH1(void, BeginIO,
	AROS_LHA(struct IOSana2Req *, req, A1),
	LIBBASETYPEPTR, LIBBASE, 5, RTL8139Dev)
{
	AROS_LIBFUNC_INIT
	struct RTL8139Unit *unit;

	req->ios2_Req.io_Error = 0;
	unit = (APTR)req->ios2_Req.io_Unit;

RTLD(bug("[rtl8139] BeginIO()\n"));

	if (AttemptSemaphore(&unit->rtl8139u_unit_lock))
	{
		handle_request(LIBBASE, req);
	}
	else
	{
		req->ios2_Req.io_Flags &= ~IOF_QUICK;
		PutMsg(unit->rtl8139u_input_port, (struct Message *)req);
	}

	AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
	AROS_LHA(struct IOSana2Req *, req, A1),
	LIBBASETYPEPTR, LIBBASE, 6, RTL8139Dev)
{
	AROS_LIBFUNC_INIT
	struct RTL8139Unit *unit;
	unit = (APTR)req->ios2_Req.io_Unit;

RTLD(bug("[rtl8139] AbortIO()\n"))

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
