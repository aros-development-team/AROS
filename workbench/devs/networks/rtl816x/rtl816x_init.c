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

#include "rtl816x.h"

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

    struct RTL816XUnit *unit;

    LIBBASETYPEPTR LIBBASE = (LIBBASETYPEPTR)hook->h_Data;

    D(bug("[rtl816x] PCI_Enumerator(PCI Device Obj @ %p)\n", pciDevice));

    if (LIBBASE->rtl816xb_UnitCount >= MAX_UNITS)
    {
        D(bug("[rtl816x] PCI_Enumerator: Max supported units already reached\n"));
        return;
    }

    /* CreateUnit rejects devices that are not in the supported id table */
    if ((unit = CreateUnit(LIBBASE, pciDevice)) != NULL)
    {
        AddTail(&LIBBASE->rtl816xb_Units, (struct Node *)&unit->rtl816xu_Node);
        RTLD(bug("[%s] PCI_Enumerator: %s MMIO @ %p\n", unit->rtl816xu_name,
                 unit->rtl816xu_rtl_chipname, unit->rtl816xu_BaseMem))
    }

    AROS_USERFUNC_EXIT
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    UBYTE tmpbuff[100];
    int i;

    D(bug("[rtl816x] Init()\n"));

    sprintf((char *)tmpbuff, RTL816X_TASK_NAME, "rtl816x.0");

    if (FindTask(tmpbuff) != NULL)
    {
        D(bug("[rtl816x] Init: Found Task '%s'! - Device already up and running.\n", tmpbuff));
        return FALSE;
    }

    /* Load config options */
    LIBBASE->rtl816xb_MaxIntWork = 20;
    LIBBASE->rtl816xb_MulticastFilterLimit = 32;

    for (i = 0; i < MAX_UNITS; i++)
    {
        LIBBASE->speed[i] = -1;
        LIBBASE->duplex[i] = -1;
        LIBBASE->autoneg[i] = -1;
    }

    NEWLIST(&LIBBASE->rtl816xb_Units);

    LIBBASE->rtl816xb_PCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    if (LIBBASE->rtl816xb_PCIDeviceAttrBase != 0)
    {
        LIBBASE->rtl816xb_PCI = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

        if (LIBBASE->rtl816xb_PCI)
        {
            struct TagItem Requirements[] =
            {
                { tHidd_PCI_VendorID, 0 },
                { TAG_DONE,           0 }
            };

            struct Hook FindHook =
            {
                .h_Entry = (IPTR (*)())PCI_Enumerator,
                .h_Data  = LIBBASE,
            };

            struct pHidd_PCI_EnumDevices enummsg =
            {
                .mID =          OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
                .callback =     &FindHook,
                .requirements = (struct TagItem *)&Requirements,
            }, *msg = &enummsg;
            IPTR lastvendor = 0;

            /* Enumerate each distinct vendor from the id table; the
               hook/CreateUnit reject non-matching products */
            for (i = 0; cards[i].vendorID != 0; i++)
            {
                if (cards[i].vendorID == lastvendor)
                    continue;
                lastvendor = Requirements[0].ti_Data = cards[i].vendorID;
                OOP_DoMethod(LIBBASE->rtl816xb_PCI, (OOP_Msg)msg);
            }

            if (!(IsListEmpty(&LIBBASE->rtl816xb_Units)))
            {
                return TRUE;
            }

            OOP_DisposeObject(LIBBASE->rtl816xb_PCI);
            LIBBASE->rtl816xb_PCI = NULL;
        }

        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
        LIBBASE->rtl816xb_PCIDeviceAttrBase = 0;
    }
    return FALSE;
}

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE)
{
    struct RTL816XUnit *unit_current, *unit_tmp;

    D(bug("[rtl816x] Expunge()\n"));

    if (!(IsListEmpty(&LIBBASE->rtl816xb_Units)))
    {
        ForeachNodeSafe(&LIBBASE->rtl816xb_Units, unit_current, unit_tmp)
        {
            Remove((struct Node *)&unit_current->rtl816xu_Node);
            DeleteUnit(LIBBASE, unit_current);
        }
    }

    if (LIBBASE->rtl816xb_PCIDeviceAttrBase != 0)
        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);

    LIBBASE->rtl816xb_PCIDeviceAttrBase = 0;

    if (LIBBASE->rtl816xb_PCI != NULL)
        OOP_DisposeObject(LIBBASE->rtl816xb_PCI);

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
    struct IOSana2Req *req,
    ULONG unitnum,
    ULONG flags
)
{
    struct TagItem *tags;
    struct RTL816XUnit *unit = NULL, *unit_current;
    struct Opener *opener = NULL;
    BOOL counted = FALSE, added = FALSE;
    BYTE error = 0;
    int i;

    if (!(IsListEmpty(&LIBBASE->rtl816xb_Units)))
    {
        ForeachNode(&LIBBASE->rtl816xb_Units, unit_current)
        {
            if (unit_current->rtl816xu_UnitNum == unitnum)
                unit = unit_current;
        }
    }

    D(bug("[rtl816x] OpenDevice(%d)\n", unitnum));

    if (unit != NULL)
    {
        RTLD(bug("[%s] OpenDevice: Unit %d @ %p\n", unit->rtl816xu_name, unitnum, unit))

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
            if ((unit->rtl816xu_open_count != 0) &&
                ((unit->rtl816xu_flags & IFF_SHARED) == 0 ||
                (flags & SANA2OPF_MINE) != 0))
            {
                error = IOERR_UNITBUSY;
            }
            else
            {
                unit->rtl816xu_open_count++;
                counted = TRUE;
            }
        }

        if (error == 0)
        {
            if ((flags & SANA2OPF_MINE) == 0)
                unit->rtl816xu_flags |= IFF_SHARED;
            else if ((flags & SANA2OPF_PROM) != 0)
                unit->rtl816xu_flags |= IFF_PROMISC;

            /* Set up buffer-management structure and get hooks */
            opener = AllocVec(sizeof(struct Opener), MEMF_PUBLIC | MEMF_CLEAR);
            req->ios2_BufferManagement = (APTR)opener;

            if (opener == NULL)
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
            AddTail((APTR)&unit->rtl816xu_Openers, (APTR)opener);
            Enable();
            added = TRUE;
        }

        /* The hardware is started by the first opener and stopped again
           by the last closer */
        if ((error == 0) && (unit->rtl816xu_open_count == 1))
        {
            if (unit->start(unit) != 0)
                error = IOERR_OPENFAIL;
        }
        else if ((error == 0) && (unit->rtl816xu_flags & IFF_UP))
        {
            /* Apply any promiscuity change from this opener */
            unit->set_multicast(unit);
        }

        if (error != 0)
        {
            if (added)
            {
                Disable();
                Remove((struct Node *)opener);
                Enable();
            }
            if (opener != NULL)
            {
                FreeVec(opener);
                req->ios2_BufferManagement = NULL;
            }
            if (counted)
                unit->rtl816xu_open_count--;
            req->ios2_Req.io_Unit = NULL;
        }
    }
    else
    {
        D(bug("[rtl816x] OpenDevice: Invalid Unit! (unitno = %d)\n", unitnum));
        error = IOERR_OPENFAIL;
    }

    req->ios2_Req.io_Error = error;

    return (error != 0) ? FALSE : TRUE;
}

static int GM_UNIQUENAME(Close)
(
    LIBBASETYPEPTR LIBBASE,
    struct IOSana2Req *req
)
{
    struct RTL816XUnit *unit;
    struct Opener *opener;

    if ((unit = (struct RTL816XUnit *)req->ios2_Req.io_Unit) != NULL)
    {
        RTLD(bug("[rtl816x] CloseDevice(unit @ %p, unitno %d)\n", unit, unit->rtl816xu_UnitNum))

        opener = (APTR)req->ios2_BufferManagement;
        if (opener != NULL)
        {
            Disable();
            Remove((struct Node *)opener);
            Enable();
            FreeVec(opener);
        }

        if ((unit->rtl816xu_open_count > 0) && (--unit->rtl816xu_open_count == 0))
        {
            if ((unit->rtl816xu_flags & IFF_UP) != 0)
                unit->stop(unit);

            unit->rtl816xu_flags &= ~(IFF_SHARED | IFF_PROMISC | IFF_CONFIGURED);
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
    LIBBASETYPEPTR, LIBBASE, 5, RTL816XDev)
{
    AROS_LIBFUNC_INIT
    struct RTL816XUnit *unit;

    D(bug("[rtl816x] BeginIO()\n"));

    req->ios2_Req.io_Error = 0;
    if ((unit = (struct RTL816XUnit *)req->ios2_Req.io_Unit) != NULL)
    {
        if (AttemptSemaphore(&unit->rtl816xu_unit_lock))
        {
            handle_request(LIBBASE, req);
        }
        else
        {
            req->ios2_Req.io_Flags &= ~IOF_QUICK;
            PutMsg(unit->rtl816xu_input_port, (struct Message *)req);
        }
    }
    else
    {
        D(bug("[rtl816x] BeginIO: Called with unit == NULL\n"));
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 6, RTL816XDev)
{
    AROS_LIBFUNC_INIT
    struct RTL816XUnit *unit;

    D(bug("[rtl816x] AbortIO()\n"));

    if ((unit = (struct RTL816XUnit *)req->ios2_Req.io_Unit) != NULL)
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
