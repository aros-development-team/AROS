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

#include "rtl8169.h"

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

    struct RTL8169Unit *unit = NULL;
    IPTR RevisionID;

    D(bug("[rtl8169] PCI_Enumerator(PCI Device Obj @ %p)\n", pciDevice));

    LIBBASETYPEPTR LIBBASE = (LIBBASETYPEPTR) hook->h_Data;

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_RevisionID, &RevisionID);

    D(bug("[rtl8169] PCI_Enumerator: Found RTL8169 Rev:%d\n", RevisionID));

    if ((LIBBASE->rtl8169b_UnitCount < MAX_UNITS) && ((unit = CreateUnit(LIBBASE, pciDevice, RevisionID)) != NULL))
    {
            AddTail(&LIBBASE->rtl8169b_Units, (struct Node *)&unit->rtl8169u_Node);
    }
    else if (LIBBASE->rtl8169b_UnitCount < MAX_UNITS)
    {
        D(bug("[rtl8169] PCI_Enumerator: Failed to create unit!\n"));
            return;
    }
    else
    {
        D(bug("[rtl8169] PCI_Enumerator: Max supported units already reached\n"));
            return;
    }
    RTLD(bug("[%s] PCI_Enumerator: %s MMIO @ %p\n", unit->rtl8169u_name, unit->rtl8169u_rtl_chipname, unit->rtl8169u_BaseMem))

    AROS_USERFUNC_EXIT
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[rtl8169] Init()\n"));

    UBYTE tmpbuff[100];
    int i;

    sprintf((char *) tmpbuff, RTL8169_TASK_NAME, "rtl8169.0");

    if (FindTask(tmpbuff) != NULL)
    {
            D(bug("[rtl8169] Init: Found Task '%s'! - Device already up and running.\n", tmpbuff));
            return FALSE;
    }

    /* Load config options */
    LIBBASE->rtl8169b_MaxIntWork = 20;
    LIBBASE->rtl8169b_MulticastFilterLimit = 32;

    for (i = 0; i < MAX_UNITS; i++)
    {
            LIBBASE->speed[i] = -1;
            LIBBASE->duplex[i] = -1;
            LIBBASE->autoneg[i] = -1;
    }

    NEWLIST(&LIBBASE->rtl8169b_Units);

    LIBBASE->rtl8169b_PCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    if (LIBBASE->rtl8169b_PCIDeviceAttrBase != 0)
    {
        D(bug("[rtl8169] Init: HiddPCIDeviceAttrBase @ %p\n", LIBBASE->rtl8169b_PCIDeviceAttrBase));

        LIBBASE->rtl8169b_PCI = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

        if (LIBBASE->rtl8169b_PCI)
        {
            D(bug("[rtl8169] Init: PCI Subsystem HIDD object @ %p\n", LIBBASE->rtl8169b_PCI));

            struct TagItem Requirements[] =
            {
                        { tHidd_PCI_VendorID,          0 },
                        { tHidd_PCI_ProductID,         0 },
                        { tHidd_PCI_SubsystemVendorID, 0 },
                        { tHidd_PCI_SubsystemID,       0 },
                        { TAG_DONE,                    0 }
            };
    
            struct Hook FindHook =
            {
                h_Entry:    (IPTR (*)()) PCI_Enumerator,
                h_Data:     LIBBASE,
            };
    
            struct pHidd_PCI_EnumDevices enummsg =
            {
                    mID:            OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
                    callback:       &FindHook,
                    requirements:   (struct TagItem *) &Requirements,
            }, *msg = &enummsg;
    
            i = 0;
            // Browse all known cards
            while(cards[i].vendorID)
            {
                Requirements[0].ti_Data = cards[i].vendorID;
                Requirements[1].ti_Data = cards[i].productID;
                Requirements[2].ti_Data = cards[i].sub_vendorID;
                Requirements[3].ti_Data = cards[i].sub_productID;
                OOP_DoMethod(LIBBASE->rtl8169b_PCI, (OOP_Msg) msg);
                i++;
            }
    
            if (!(IsListEmpty(&LIBBASE->rtl8169b_Units)))
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[rtl8169] Expunge()\n"));

    struct RTL8169Unit *unit_current, *unit_tmp;

    if (!(IsListEmpty(&LIBBASE->rtl8169b_Units)))
    {
        ForeachNodeSafe(&LIBBASE->rtl8169b_Units, unit_current, unit_tmp)
        {
            DeleteUnit(LIBBASE, unit_current);
        }
    }

    if (LIBBASE->rtl8169b_PCIDeviceAttrBase != 0)
    {
        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    }

    LIBBASE->rtl8169b_PCIDeviceAttrBase = 0;

    if (LIBBASE->rtl8169b_PCI != NULL)
    {
        OOP_DisposeObject(LIBBASE->rtl8169b_PCI);
    }

    return TRUE;
}

static const ULONG rx_tags[] = {
    S2_CopyToBuff,
    S2_CopyToBuff16,
        S2_CopyToBuff32
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
    struct RTL8169Unit *unit = NULL, *unit_current;
    struct Opener *opener;
    BYTE error=0;
    int i;

    if (!(IsListEmpty(&LIBBASE->rtl8169b_Units)))
    {
        ForeachNode(&LIBBASE->rtl8169b_Units, unit_current)
        {
            if (unit_current->rtl8169u_UnitNum == unitnum)
            {
                unit = unit_current;
            }
        }
    }

    D(bug("[rtl8169] OpenDevice(%d)\n", unitnum));

    if (unit != NULL)
    {
        RTLD(bug("[%s] OpenDevice: Unit %d @ %p\n", unit->rtl8169u_name, unitnum, unit))

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
            if ((unit->rtl8169u_open_count != 0) && 
               ((unit->rtl8169u_flags & IFF_SHARED) == 0 ||
               (flags & SANA2OPF_MINE) != 0))
            {
                    error = IOERR_UNITBUSY;
            }
            else
            {
                    unit->rtl8169u_open_count++;
            }
        }
    
        if (error == 0)
        {
            if ((flags & SANA2OPF_MINE) == 0)
                        unit->rtl8169u_flags |= IFF_SHARED;
            else if ((flags & SANA2OPF_PROM) != 0)
                        unit->rtl8169u_flags |= IFF_PROMISC;
    
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
            {
                        opener->rx_function = (APTR) GetTagData(rx_tags[i],
                                                                (IPTR) opener->rx_function, tags);
                        }
            for (i = 0; i < 3; i++)
            {
                        opener->tx_function = (APTR) GetTagData(tx_tags[i],
                                                                (IPTR) opener->tx_function, tags);
                        }
    
            opener->filter_hook = (APTR) GetTagData(S2_PacketFilter, 0, tags);
    
            Disable();
            AddTail((APTR) &unit->rtl8169u_Openers, (APTR) opener);
            Enable();
        }
    
        if (error != 0)
        {
            CloseDevice((struct IORequest *)req);
        }
        else
        {
            unit->start(unit);
        }
    }
    else
    {
        D(bug("[rtl8169] OpenDevice: Invalid Unit! (unitno = %d)\n", unitnum));
        error = IOERR_OPENFAIL;
    }

    req->ios2_Req.io_Error = error;

    return (error != 0) ? FALSE : TRUE;
}

static int GM_UNIQUENAME(Close)
(
    LIBBASETYPEPTR LIBBASE,
    struct IOSana2Req* req
)
{
    struct RTL8169Unit *unit = (struct RTL8169Unit *) req->ios2_Req.io_Unit;
    struct Opener *opener;

    if ((unit = (struct RTL8169Unit *) req->ios2_Req.io_Unit) != NULL)
    {
        RTLD(bug("[rtl8169] CloseDevice(unit @ %p, unitno %d)\n", unit, unit->rtl8169u_UnitNum))

        unit->stop(unit);

        opener = (APTR)req->ios2_BufferManagement;
        if (opener != NULL)
        {
            Disable();
            Remove((struct Node *)opener);
            Enable();
            FreeVec(opener);
        }
    }
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

AROS_LH1(void, BeginIO,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 5, RTL8169Dev)
{
    AROS_LIBFUNC_INIT
    struct RTL8169Unit *unit;

    D(bug("[rtl8169] BeginIO()\n"));

    req->ios2_Req.io_Error = 0;
    if ((unit = (struct RTL8169Unit *) req->ios2_Req.io_Unit) != NULL)
    {
        if (AttemptSemaphore(&unit->rtl8169u_unit_lock))
        {
            handle_request(LIBBASE, req);
        }
        else
        {
            req->ios2_Req.io_Flags &= ~IOF_QUICK;
            PutMsg(unit->rtl8169u_input_port, (struct Message *)req);
        }
    }
    else
    {
        D(bug("[rtl8169] BeginIO: Called with unit == NULL\n"));
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 6, RTL8169Dev)
{
    AROS_LIBFUNC_INIT
    struct RTL8169Unit *unit;

    D(bug("[rtl8169] AbortIO()\n"));

    if ((unit = (struct RTL8169Unit *)req->ios2_Req.io_Unit) != NULL)
    {
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
    }
    return 0;

    AROS_LIBFUNC_EXIT
}
