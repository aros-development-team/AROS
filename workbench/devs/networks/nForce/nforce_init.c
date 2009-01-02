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

#define DEBUG 0

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/lists.h>

#include <aros/libcall.h>
#include <aros/debug.h>
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

#include "nforce.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

AROS_UFH3(void, Enumerator,
    AROS_UFHA(struct Hook *,    hook,       A0),
    AROS_UFHA(OOP_Object *,     pciDevice,  A2),
    AROS_UFHA(APTR,             message,    A1))
{
    AROS_USERFUNC_INIT

    LIBBASETYPEPTR LIBBASE = (LIBBASETYPEPTR)hook->h_Data;

    IPTR DeviceID;
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &DeviceID);

    if ((DeviceID == NFORCE_MCPNET1_ID)    ||
        (DeviceID == NFORCE_MCPNET2_ID)    ||
        (DeviceID == NFORCE_MCPNET3_ID)    ||
        (DeviceID == NFORCE_MCPNET4_ID)    ||
        (DeviceID == NFORCE_MCPNET5_ID)    ||
        (DeviceID == NFORCE_MCPNET6_ID)    ||
        (DeviceID == NFORCE_MCPNET7_ID)    ||
        (DeviceID == NFORCE_MCPNET8_ID)    ||
        (DeviceID == NFORCE_MCPNET9_ID)    ||
        (DeviceID == NFORCE_MCPNET10_ID)   ||
        (DeviceID == NFORCE_MCPNET11_ID))
    {
        struct NFUnit   *unit = CreateUnit(LIBBASE, pciDevice);
        LIBBASE->nf_unit = unit;

        D(bug("[nforce] Found nForce NIC, ProductID = %04x\n", DeviceID));
        D(bug("[nforce] nForce NIC I/O @ %08x\n", unit->nu_BaseIO));
        D(bug("[nforce] nForce NIC MEM @ %08x\n", unit->nu_BaseMem)); 
    }

    AROS_USERFUNC_EXIT
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[nforce] Init()\n"));

    if (FindTask(NFORCE_TASK_NAME) != NULL)
    {
        D(bug("[nforce] device already up and running.\n"));
        return FALSE;
    }

    LIBBASE->nf_Sana2Info.HardwareType = S2WireType_Ethernet;
    LIBBASE->nf_Sana2Info.MTU = ETH_MTU;
    LIBBASE->nf_Sana2Info.AddrFieldSize = 8 * ETH_ADDRESSSIZE;

    LIBBASE->nf_pciDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    if (LIBBASE->nf_pciDeviceAttrBase != 0)
    {
        D(bug("[nforce] Got HiddPCIDeviceAttrBase\n"));
        
        LIBBASE->nf_pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

        if (LIBBASE->nf_pci)
        {
            D(bug("[nforce] Got PCI object\n"));

            struct Hook FindHook = {
                h_Entry:    (IPTR (*)())Enumerator,
                h_Data:     LIBBASE,
            };

            struct TagItem Requirements[] = {
                { tHidd_PCI_VendorID,   0x10de },
                { TAG_DONE,             0UL }
            };

            HIDD_PCI_EnumDevices(   LIBBASE->nf_pci,
                                    &FindHook,
                                    (struct TagItem *)&Requirements
            );

            if (LIBBASE->nf_unit)
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[nforce] Expunge\n"));

    if (LIBBASE->nf_unit)
    {
        DeleteUnit(LIBBASE, LIBBASE->nf_unit);
        LIBBASE->nf_unit = NULL;
    }

    if (LIBBASE->nf_pciDeviceAttrBase != 0)
        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);

    LIBBASE->nf_pciDeviceAttrBase = 0;

    if (LIBBASE->nf_pci != NULL)
        OOP_DisposeObject(LIBBASE->nf_pci);

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
 * Open device handles currently only one nforce unit.
 * 
 * It could change in future if only multiple phy support works in forcedeth.c
 * linux driver.
 */
static int GM_UNIQUENAME(Open)
(
    LIBBASETYPEPTR LIBBASE,
    struct IOSana2Req *req,
    ULONG unitnum,
    ULONG flags
)
{
    struct TagItem *tags;
    struct NFUnit *unit = LIBBASE->nf_unit;
    struct Opener *opener;
    BYTE error=0;
    int i;

    D(bug("[nforce] OpenDevice\n"));

    LIBBASE->nf_Device.dd_Library.lib_OpenCnt++;
    LIBBASE->nf_Device.dd_Library.lib_Flags &= ~LIBF_DELEXP;

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
        if(unit->open_count != 0 && ((unit->flags & IFF_SHARED) == 0 ||
            (flags & SANA2OPF_MINE) != 0))
            error = IOERR_UNITBUSY;
        unit->open_count++;
    }

    if(error == 0)
    {
        if((flags & SANA2OPF_MINE) == 0)
        unit->flags |= IFF_SHARED;
        else if((flags & SANA2OPF_PROM) != 0)
        unit->flags |= IFF_PROMISC;

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
        AddTail((APTR)&unit->nu_Openers, (APTR)opener);
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
    struct IOSana2Req *req
)
{
    struct NFUnit *unit = LIBBASE->nf_unit;
    struct Opener *opener;

    D(bug("[nforce] CloseDevice\n"));

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

AROS_LH1(void, beginio,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 5, Nforce)
{
    AROS_LIBFUNC_INIT
    struct NFUnit *dev;

    D(bug("[nforce] BeginIO\n"));

    req->ios2_Req.io_Error = 0;
    dev = (APTR)req->ios2_Req.io_Unit;

    if (AttemptSemaphore(&dev->unit_lock))
    {
        handle_request(LIBBASE, req);
    }
    else
    {
        req->ios2_Req.io_Flags &= ~IOF_QUICK;
        PutMsg(dev->nu_input_port, (struct Message *)req);
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 5, Nforce)
{
    AROS_LIBFUNC_INIT
    struct NFUnit *dev;
    dev = (APTR)req->ios2_Req.io_Unit;

    D(bug("[nforce] AbortIO\n"));

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
