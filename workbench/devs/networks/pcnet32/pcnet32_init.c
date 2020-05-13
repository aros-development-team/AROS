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

#include "pcnet32.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

AROS_UFH3(void, Enumerator,
    AROS_UFHA(struct Hook *,    hook,       A0),
    AROS_UFHA(OOP_Object *,     pciDevice,  A2),
    AROS_UFHA(APTR,             message,    A1))
{
    AROS_USERFUNC_INIT

D(bug("[pcnet32] init.PCI_Enumerator()\n"));

    LIBBASETYPEPTR LIBBASE = (LIBBASETYPEPTR)hook->h_Data;

    IPTR DeviceID;
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &DeviceID);

    if ((DeviceID == 0x2000)    ||
        (DeviceID == 0x2001)    ||
        (DeviceID == 0x2625))
    {
D(bug("[pcnet32] Found PCNet32 NIC, ProductID = %04x\n", DeviceID));

        struct PCN32Unit   *unit = CreateUnit(LIBBASE, pciDevice);
        LIBBASE->pcnb_unit = unit;

D(bug("[pcnet32] PCNet32 NIC I/O MEM @ %08x\n", unit->pcnu_BaseMem)); 
    }

    AROS_USERFUNC_EXIT
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
D(bug("[pcnet32] init.Init()\n"));

    if (FindTask(PCNET32_TASK_NAME) != NULL)
    {
        D(bug("[pcnet32] device already up and running.\n"));
        return FALSE;
    }

    LIBBASE->pcnb_Sana2Info.HardwareType = S2WireType_Ethernet;
    LIBBASE->pcnb_Sana2Info.MTU = ETH_MTU;
    LIBBASE->pcnb_Sana2Info.AddrFieldSize = 8 * ETH_ADDRESSSIZE;

    LIBBASE->pcnb_pciDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    if (LIBBASE->pcnb_pciDeviceAttrBase != 0)
    {
        D(bug("[pcnet32] Got HiddPCIDeviceAttrBase\n"));

        LIBBASE->pcnb_pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

        if (LIBBASE->pcnb_pci)
        {
            D(bug("[pcnet32] Got PCI object\n"));

            struct Hook FindHook = {
                h_Entry:    (IPTR (*)())Enumerator,
                h_Data:     LIBBASE,
            };

            struct TagItem Requirements[] = {
                { tHidd_PCI_VendorID,   0x1022 },
                { TAG_DONE,             0UL }
            };

            HIDD_PCI_EnumDevices(   LIBBASE->pcnb_pci,
                                    &FindHook,
                                    (struct TagItem *)&Requirements
            );

            if (LIBBASE->pcnb_unit)
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE)
{
D(bug("[pcnet32] init.Expunge\n"));

    if (LIBBASE->pcnb_unit)
    {
        DeleteUnit(LIBBASE, LIBBASE->pcnb_unit);
        LIBBASE->pcnb_unit = NULL;
    }

    if (LIBBASE->pcnb_pciDeviceAttrBase != 0)
        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);

    LIBBASE->pcnb_pciDeviceAttrBase = 0;

    if (LIBBASE->pcnb_pci != NULL)
        OOP_DisposeObject(LIBBASE->pcnb_pci);

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
 * Open device handles currently only one pcnet32 unit.
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
    struct PCN32Unit *unit = LIBBASE->pcnb_unit;
    struct Opener *opener;
    BYTE error=0;
    int i;

D(bug("[pcnet32] init.OpenDevice\n"));

    req->ios2_Req.io_Unit = NULL;
    tags = req->ios2_BufferManagement;

    req->ios2_BufferManagement = NULL;

    /* Check request size */

    if(req->ios2_Req.io_Message.mn_Length < sizeof(struct IOSana2Req))
        error = IOERR_OPENFAIL;

    /* Get the requested unit */
    if((error == 0) && (unitnum == 0))
    {
        req->ios2_Req.io_Unit = (APTR)unit;
    }
    else error = IOERR_OPENFAIL;

    /* Handle device sharing */

    if(error == 0)
    {
        if(unit->pcnu_open_count != 0 && ((unit->pcnu_flags & IFF_SHARED) == 0 ||
            (flags & SANA2OPF_MINE) != 0))
            error = IOERR_UNITBUSY;
        unit->pcnu_open_count++;
    }

    if(error == 0)
    {
        if((flags & SANA2OPF_MINE) == 0)
        unit->pcnu_flags |= IFF_SHARED;
        else if((flags & SANA2OPF_PROM) != 0)
        unit->pcnu_flags |= IFF_PROMISC;

        /* Set up buffer-management structure and get hooks */
        opener = AllocVec(sizeof(struct Opener), MEMF_PUBLIC | MEMF_CLEAR);
        req->ios2_BufferManagement = (APTR)opener;

        if(opener == NULL)
        error = IOERR_OPENFAIL;
    }

    if(error == 0)
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
        AddTail((APTR)&unit->pcnu_Openers, (APTR)opener);
        Enable();
    }

    if (error != 0)
        CloseDevice((struct IORequest *)req);
    else
        unit->start(unit);

    req->ios2_Req.io_Error = error;

    return (error !=0) ? FALSE : TRUE;
}

static int GM_UNIQUENAME(Close)
(
    LIBBASETYPEPTR LIBBASE,
    struct IOSana2Req* req
)
{
    struct PCN32Unit *unit = LIBBASE->pcnb_unit;
    struct Opener *opener;

D(bug("[pcnet32] init.CloseDevice\n"));

    if((unit->pcnu_flags & IFF_UP) != 0)
        unit->stop(unit);

    opener = (APTR)req->ios2_BufferManagement;
    if (opener != NULL)
    {
        Disable();
        Remove((struct Node *)opener);
        Enable();
        FreeVec(opener);
    }

    /* Without this, DHCP doesn't work the second time the device is used */
    ((struct Library *)LIBBASE)->lib_Flags |= LIBF_DELEXP;

    return TRUE;
}


ADD2INITLIB(GM_UNIQUENAME(Init),0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge),0)
ADD2OPENDEV(GM_UNIQUENAME(Open),0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close),0)

AROS_LH1(void, beginio,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 5, PCNet32)
{
    AROS_LIBFUNC_INIT
    struct PCN32Unit *dev;

D(bug("[pcnet32] init.BeginIO\n"));

    req->ios2_Req.io_Error = 0;
    dev = (APTR)req->ios2_Req.io_Unit;

    if (AttemptSemaphore(&dev->pcnu_unit_lock))
    {
        handle_request(LIBBASE, req);
    }
    else
    {
        req->ios2_Req.io_Flags &= ~IOF_QUICK;
        PutMsg(dev->pcnu_input_port, (struct Message *)req);
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 6, PCNet32)
{
    AROS_LIBFUNC_INIT

D(bug("[pcnet32] init.AbortIO\n"));

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
