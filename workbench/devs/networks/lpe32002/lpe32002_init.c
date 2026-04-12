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

/*
 * Emulex LPe32002 SANA2 Device Driver
 *
 * Main device entry point: PCI enumeration, device init/expunge,
 * open/close, and BeginIO/AbortIO for the SANA2 interface.
 *
 * Supports:
 *   - LPe32002-M2   (PCI 10DF:E300) - Dual Port 32Gb FC
 *   - LPe32000-M2   (PCI 10DF:E301) - Single Port 32Gb FC
 *   - LPe32002-M2-D (PCI 10DF:E320) - Dual Port 32Gb FC (diag)
 *   - LPe35002-M2   (PCI 10DF:E330) - Dual Port 32Gb FC
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

#include "lpe32002.h"
#include "lpe32002_hw.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

/* ===== Supported PCI Device Table ===== */

struct pci_device_ids
{
    IPTR deviceid;
    char *devicename;
};

static struct pci_device_ids lpe_devices[] =
{
    { PCI_DEVICE_ID_LPE32002_M2,    "LPe32002-M2 (Dual 32Gb FC)"    },
    { PCI_DEVICE_ID_LPE32000_M2,    "LPe32000-M2 (Single 32Gb FC)"  },
    { PCI_DEVICE_ID_LPE32002_M2_D,  "LPe32002-M2-D (Dual 32Gb FC)"  },
    { PCI_DEVICE_ID_LPE35002_M2,    "LPe35002-M2 (Dual 32Gb FC)"    },
    { (IPTR)NULL,                    NULL                             }
};

/* ===== OOP Attribute/Method Base Setup ===== */

#if defined(__OOP_NOATTRBASES__)
static CONST_STRPTR const GM_UNIQUENAME(AttrBaseIDs)[] =
{
    IID_Hidd_PCIDevice,
    NULL
};
#endif

#if defined(__OOP_NOMETHODBASES__)
static CONST_STRPTR const GM_UNIQUENAME(MethBaseIDs)[] =
{
    IID_Hidd_PCI,
    IID_Hidd_PCIDevice,
    IID_Hidd_PCIDriver,
    NULL
};
#endif

/* ===== PCI Enumerator Callback ===== */

AROS_UFH3(void, GM_UNIQUENAME(PCI_Enumerator),
    AROS_UFHA(struct Hook *,    hook,       A0),
    AROS_UFHA(OOP_Object *,     pciDevice,  A2),
    AROS_UFHA(APTR,             message,    A1))
{
    AROS_USERFUNC_INIT

    struct LPe32002Unit *unit;
    int devid_count = 0;
    IPTR DeviceID = 0;

    D(bug("[lpe32002] %s()\n", __func__));

    LIBBASETYPEPTR LIBBASE = (LIBBASETYPEPTR)hook->h_Data;

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &DeviceID);

    while(lpe_devices[devid_count].deviceid != (IPTR)NULL)
    {
        if (DeviceID == lpe_devices[devid_count].deviceid)
        {
            D(bug("[lpe32002] %s: Found %s, ProductID = %04lx\n",
                __func__, lpe_devices[devid_count].devicename, DeviceID));

            if ((unit = CreateUnit(LIBBASE, pciDevice)) != NULL)
            {
                AddTail(&LIBBASE->lpeb_Units, &unit->lpeu_Node);
            }
            else
            {
                D(bug("[lpe32002] %s: Failed to create unit!\n", __func__));
            }
            break;
        }
        devid_count++;
    }

    AROS_USERFUNC_EXIT
}

/* ===== Device Init ===== */

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    UBYTE tmpbuff[100];

    D(bug("[lpe32002] %s()\n", __func__));

    sprintf((char *)tmpbuff, LPE_TASK_NAME, "lpe32002.0");
    if (FindTask(tmpbuff) != NULL)
    {
        D(bug("[lpe32002] %s: Device already running\n", __func__));
        return FALSE;
    }

    NEWLIST(&LIBBASE->lpeb_Units);

#if defined(__OOP_NOLIBBASE__)
    if ((LIBBASE->lpeb_OOPBase = OpenLibrary("oop.library", 0)) == NULL)
        return FALSE;
#endif

#if defined(__OOP_NOATTRBASES__)
    if (OOP_ObtainAttrBasesArray(&LIBBASE->lpeb_PCIDeviceAttrBase, GM_UNIQUENAME(AttrBaseIDs)))
    {
        bug("[lpe32002] %s: Failed to obtain AttrBases!\n", __func__);
        return FALSE;
    }
#endif

#if defined(__OOP_NOMETHODBASES__)
    if (OOP_ObtainMethodBasesArray(&LIBBASE->lpeb_HiddPCIBase, GM_UNIQUENAME(MethBaseIDs)))
    {
#if defined(__OOP_NOATTRBASES__)
        OOP_ReleaseAttrBasesArray(&LIBBASE->lpeb_PCIDeviceAttrBase, GM_UNIQUENAME(AttrBaseIDs));
#endif
        return FALSE;
    }
#endif

    LIBBASE->lpeb_PCI = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (LIBBASE->lpeb_PCI)
    {
        D(bug("[lpe32002] %s: PCI HIDD @ %p\n", __func__, LIBBASE->lpeb_PCI));

        struct Hook FindHook = {
            .h_Entry = (IPTR (*)())GM_UNIQUENAME(PCI_Enumerator),
            .h_Data  = LIBBASE,
        };

        struct TagItem Requirements[] = {
            { tHidd_PCI_VendorID,   PCI_VENDOR_ID_EMULEX },
            { TAG_DONE,             0UL }
        };

        HIDD_PCI_EnumDevices(LIBBASE->lpeb_PCI,
                             &FindHook,
                             (struct TagItem *)&Requirements);

        if (!(IsListEmpty(&LIBBASE->lpeb_Units)))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/* ===== Device Expunge ===== */

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE)
{
    struct LPe32002Unit *unit_current, *unit_tmp;

    D(bug("[lpe32002] %s()\n", __func__));

    if (!(IsListEmpty(&LIBBASE->lpeb_Units)))
    {
        ForeachNodeSafe(&LIBBASE->lpeb_Units, unit_current, unit_tmp)
        {
            DeleteUnit(LIBBASE, unit_current);
        }
    }

#if defined(__OOP_NOATTRBASES__)
    D(bug("[lpe32002] %s: Releasing attribute bases\n", __func__));
    OOP_ReleaseAttrBasesArray(&LIBBASE->lpeb_PCIDeviceAttrBase, GM_UNIQUENAME(AttrBaseIDs));
#endif

    if (LIBBASE->lpeb_PCI != NULL)
        OOP_DisposeObject(LIBBASE->lpeb_PCI);

    return TRUE;
}

/* ===== Device Open ===== */

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
    struct IOSana2Req *req,
    ULONG unitnum,
    ULONG flags
)
{
    struct TagItem *tags;
    struct LPe32002Unit *unit = NULL, *unit_current;
    struct Opener *opener;
    BYTE error = 0;
    int i;

    D(bug("[lpe32002] %s(%ld)\n", __func__, unitnum));

    if (!(IsListEmpty(&LIBBASE->lpeb_Units)))
    {
        ForeachNode(&LIBBASE->lpeb_Units, unit_current)
        {
            if (unit_current->lpeu_UnitNum == unitnum)
                unit = unit_current;
        }
    }

    if (unit != NULL)
    {
        D(bug("[lpe32002] %s: Unit %ld @ %p\n", __func__, unitnum, unit));

        req->ios2_Req.io_Unit = NULL;
        tags = req->ios2_BufferManagement;
        req->ios2_BufferManagement = NULL;

        if (req->ios2_Req.io_Message.mn_Length < sizeof(struct IOSana2Req))
            error = IOERR_OPENFAIL;

        req->ios2_Req.io_Unit = (APTR)unit;

        if (error == 0)
        {
            if (unit->lpeu_open_count != 0 &&
                ((unit->lpeu_ifflags & IFF_SHARED) == 0 ||
                 (flags & SANA2OPF_MINE) != 0))
            {
                error = IOERR_UNITBUSY;
            }
            else
            {
                unit->lpeu_open_count++;
            }
        }

        if (error == 0)
        {
            if ((flags & SANA2OPF_MINE) == 0)
                unit->lpeu_ifflags |= IFF_SHARED;
            else if ((flags & SANA2OPF_PROM) != 0)
                unit->lpeu_ifflags |= IFF_PROMISC;

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
            AddTail((APTR)&unit->lpeu_Openers, (APTR)opener);
            Enable();
        }

        if (error != 0)
            CloseDevice((struct IORequest *)req);
        else
        {
            D(bug("[lpe32002] %s: Starting Unit %ld\n", __func__, unitnum));

            if (lpe_request_irq(unit))
            {
                error = IOERR_OPENFAIL;
            }
            else
            {
                D(bug("[%s] %s: IRQ attached\n", unit->lpeu_name, __func__));
            }
        }
    }
    else
    {
        D(bug("[lpe32002] %s: Invalid unit %ld\n", __func__, unitnum));
        error = IOERR_OPENFAIL;
    }

    req->ios2_Req.io_Error = error;
    return (error != 0) ? FALSE : TRUE;
}

/* ===== Device Close ===== */

static int GM_UNIQUENAME(Close)
(
    LIBBASETYPEPTR LIBBASE,
    struct IOSana2Req *req
)
{
    struct LPe32002Unit *unit = (struct LPe32002Unit *)req->ios2_Req.io_Unit;
    struct Opener *opener;

    D(bug("[lpe32002] %s(unit %ld)\n", __func__, unit ? unit->lpeu_UnitNum : -1));

    if (unit)
        unit->lpeu_open_count--;

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

/* ===== Auto-init Hooks ===== */

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

/* ===== BeginIO ===== */

AROS_LH1(void, BeginIO,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 5, LPe32002Dev)
{
    AROS_LIBFUNC_INIT
    struct LPe32002Unit *unit;

    D(bug("[lpe32002] %s()\n", __func__));

    req->ios2_Req.io_Error = 0;
    if ((unit = (APTR)req->ios2_Req.io_Unit) != NULL)
    {
        if (AttemptSemaphore(&unit->lpeu_unit_lock))
        {
            handle_request(LIBBASE, req);
        }
        else
        {
            req->ios2_Req.io_Flags &= ~IOF_QUICK;
            PutMsg(unit->lpeu_input_port, (struct Message *)req);
        }
    }

    AROS_LIBFUNC_EXIT
}

/* ===== AbortIO ===== */

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 6, LPe32002Dev)
{
    AROS_LIBFUNC_INIT
    struct LPe32002Unit *unit;

    D(bug("[lpe32002] %s()\n", __func__));

    if ((unit = (APTR)req->ios2_Req.io_Unit) != NULL)
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
