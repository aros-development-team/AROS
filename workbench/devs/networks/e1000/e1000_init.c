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

#include "e1000.h"
#include "e1000_api.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

struct pci_device_ids
{
    IPTR deviceid;
    char *devicename;
};

struct pci_device_ids e1000_devices[] =
{
    {  (E1000_DEV_ID_82542),                    "82542"                       },
    {  (E1000_DEV_ID_82543GC_FIBER),            "82543GC (Fiber)"             },
    {  (E1000_DEV_ID_82543GC_COPPER),           "82543GC (Copper)"            },
    {  (E1000_DEV_ID_82544EI_COPPER),           "82544EI (Copper)"            },
    {  (E1000_DEV_ID_82544EI_FIBER),            "82544EI (Fiber)"             },
    {  (E1000_DEV_ID_82544GC_COPPER),           "82544GC (Copper)"            },
    {  (E1000_DEV_ID_82544GC_LOM),              "82544GC (LOM)"               },
    {  (E1000_DEV_ID_82540EM),                  "82540EM"                     },
    {  (E1000_DEV_ID_82545EM_COPPER),           "82545EM (Copper)"            },
    {  (E1000_DEV_ID_82546EB_COPPER),           "82546EB (Copper)"            },
    {  (E1000_DEV_ID_82545EM_FIBER),            "82545EM (Fiber)"             },
    {  (E1000_DEV_ID_82546EB_FIBER),            "82546EB (Fiber)"             },
    {  (E1000_DEV_ID_82541EI),                  "82541EI"                     },
    {  (E1000_DEV_ID_82541ER_LOM),              "82541ER (LOM)"               },
    {  (E1000_DEV_ID_82540EM_LOM),              "82540EM (LOM)"               },
    {  (E1000_DEV_ID_82540EP_LOM),              "82540EP (LOM)"               },
    {  (E1000_DEV_ID_82540EP),                  "82540EP"                     },
    {  (E1000_DEV_ID_82541EI_MOBILE),           "82541EI (Mobile)"            },
    {  (E1000_DEV_ID_82547EI),                  "82547EI"                     },
    {  (E1000_DEV_ID_82547EI_MOBILE),           "82547EI (Mobile)"            },
    {  (E1000_DEV_ID_82546EB_QUAD_COPPER),      "82546EB (Quad Copper)"       },
    {  (E1000_DEV_ID_82540EP_LP),               "82540EP (LP)"                },
    {  (E1000_DEV_ID_82545GM_COPPER),           "82545GM (Copper)"            },
    {  (E1000_DEV_ID_82545GM_FIBER),            "82545GM (Fiber)"             },
    {  (E1000_DEV_ID_82545GM_SERDES),           "82545GM (Serdes)"            },
    {  (E1000_DEV_ID_82547GI),                  "82547GI"                     },
    {  (E1000_DEV_ID_82541GI),                  "82541GI"                     },
    {  (E1000_DEV_ID_82541GI_MOBILE),           "82541GI (Mobile)"            },
    {  (E1000_DEV_ID_82541ER),                  "82541ER"                     },
    {  (E1000_DEV_ID_82546GB_COPPER),           "82546GB (Copper)"            },
    {  (E1000_DEV_ID_82546GB_FIBER),            "82546GB (Fiber)"             },
    {  (E1000_DEV_ID_82546GB_SERDES),           "82546GB (Serdes)"            },
    {  (E1000_DEV_ID_82541GI_LF),               "82541GI (LF)"                },
    {  (E1000_DEV_ID_82546GB_PCIE),             "82546GB (PCI-E)"             },
    {  (E1000_DEV_ID_82546GB_QUAD_COPPER),      "82546GB (Quad Copper)"       },
    {  (E1000_DEV_ID_82546GB_QUAD_COPPER_KSP3), "82546GB (Quad Copper KSP3)"  },
    {  (IPTR)NULL,                              NULL                          }
};

AROS_UFH3(void, PCI_Enumerator,
    AROS_UFHA(struct Hook *,    hook,       A0),
    AROS_UFHA(OOP_Object *,     pciDevice,  A2),
    AROS_UFHA(APTR,             message,    A1))
{
    AROS_USERFUNC_INIT

    struct e1000Unit   *unit;
    int devid_count = 0;
    IPTR DeviceID = 0;

D(bug("[e1000] PCI_Enumerator()\n"));

    LIBBASETYPEPTR LIBBASE = (LIBBASETYPEPTR)hook->h_Data;

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &DeviceID);

    while(e1000_devices[devid_count].deviceid != (IPTR)NULL)
    {
        if (DeviceID == e1000_devices[devid_count].deviceid)
        {
D(bug("[e1000] PCI_Enumerator: Found %s e1000 NIC, ProductID = %04x\n", e1000_devices[devid_count].devicename, DeviceID));

            if ((unit = CreateUnit(LIBBASE, pciDevice)) != NULL)
            {
                AddTail(&LIBBASE->e1kb_Units, &unit->e1ku_Node);
            }
            else
            {
D(bug("[e1000] PCI_Enumerator: Failed to create unit!\n"));
            }
            break;
        }
        devid_count++;
    }

    AROS_USERFUNC_EXIT
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
D(bug("[e1000] Init()\n"));

    UBYTE tmpbuff[100];
    sprintf((char *)tmpbuff, e1000_TASK_NAME, "e1000.0");

    if (FindTask(tmpbuff) != NULL)
    {
        D(bug("[e1000] device already up and running.\n"));
        return FALSE;
    }

    NEWLIST(&LIBBASE->e1kb_Units);

    LIBBASE->e1kb_PCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    if (LIBBASE->e1kb_PCIDeviceAttrBase != 0)
    {
        D(bug("[e1000] HiddPCIDeviceAttrBase @ %p\n", LIBBASE->e1kb_PCIDeviceAttrBase));

        LIBBASE->e1kb_PCI = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
        
        if (LIBBASE->e1kb_PCI)
        {
            D(bug("[e1000] PCI Subsystem HIDD object @ %p\n", LIBBASE->e1kb_PCI));

            struct Hook FindHook = {
                h_Entry:    (IPTR (*)())PCI_Enumerator,
                h_Data:     LIBBASE,
            };

            struct TagItem Requirements[] = {
                { tHidd_PCI_VendorID,   0x8086  },
                { TAG_DONE,             0UL }
            };

            HIDD_PCI_EnumDevices(LIBBASE->e1kb_PCI,
                                 &FindHook,
                                 (struct TagItem *)&Requirements
            );

            if (!(IsListEmpty(&LIBBASE->e1kb_Units)))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE)
{
D(bug("[e1000] Expunge()\n"));

    struct e1000Unit *unit_current, *unit_tmp;

    if (!(IsListEmpty(&LIBBASE->e1kb_Units)))
    {
        ForeachNodeSafe(&LIBBASE->e1kb_Units, unit_current, unit_tmp)
        {
            DeleteUnit(LIBBASE, unit_current);
        }
    }

    if (LIBBASE->e1kb_PCIDeviceAttrBase != 0)
        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);

    LIBBASE->e1kb_PCIDeviceAttrBase = 0;

    if (LIBBASE->e1kb_PCI != NULL)
        OOP_DisposeObject(LIBBASE->e1kb_PCI);

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
    struct e1000Unit *unit = NULL, *unit_current;
    struct Opener *opener;
    BYTE error=0;
    int i;

    if (!(IsListEmpty(&LIBBASE->e1kb_Units)))
    {
        ForeachNode(&LIBBASE->e1kb_Units, unit_current)
        {
            if (unit_current->e1ku_UnitNum == unitnum)
                unit = unit_current;
        }
    }
    
D(bug("[e1000] OpenDevice(%d)\n", unitnum));

    if (unit != NULL)
    {
D(bug("[e1000] OpenDevice: Unit %d @ %p\n", unitnum, unit));
        req->ios2_Req.io_Unit = NULL;
        tags = req->ios2_BufferManagement;

        req->ios2_BufferManagement = NULL;

        /* Check request size */
        if(req->ios2_Req.io_Message.mn_Length < sizeof(struct IOSana2Req))
            error = IOERR_OPENFAIL;

        req->ios2_Req.io_Unit = (APTR)unit;

        /* Handle device sharing */
        if(error == 0)
        {
            if(unit->e1ku_open_count != 0 && ((unit->e1ku_ifflags & IFF_SHARED) == 0 ||
                (flags & SANA2OPF_MINE) != 0))
            {
                error = IOERR_UNITBUSY;
            }
            else
            {
                unit->e1ku_open_count++;
            }
        }

        if(error == 0)
        {
            if((flags & SANA2OPF_MINE) == 0)
                unit->e1ku_ifflags |= IFF_SHARED;
            else if((flags & SANA2OPF_PROM) != 0)
                unit->e1ku_ifflags |= IFF_PROMISC;

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
            AddTail((APTR)&unit->e1ku_Openers, (APTR)opener);
            Enable();
        }

        if (error != 0)
            CloseDevice((struct IORequest *)req);
        else
        {
D(bug("[e1000] OpenDevice: Starting Unit %d\n", unitnum));
            ULONG rx_ring_count, tx_ring_count;
            enum e1000_mac_type mac_type;

            mac_type = ((struct e1000_hw *)unit->e1ku_Private00)->mac.type;

            ((struct e1000_hw *)unit->e1ku_Private00)->mac.autoneg = AUTONEG_ENABLE;
            if (((struct e1000_hw *)unit->e1ku_Private00)->phy.media_type == e1000_media_type_fiber)
                ((struct e1000_hw *)unit->e1ku_Private00)->phy.autoneg_advertised = ADVERTISED_1000baseT_Full | ADVERTISED_FIBRE | ADVERTISED_Autoneg;
            else
                ((struct e1000_hw *)unit->e1ku_Private00)->phy.autoneg_advertised = ADVERTISED_TP | ADVERTISED_Autoneg;
            ((struct e1000_hw *)unit->e1ku_Private00)->fc.requested_mode = e1000_fc_default;

            rx_ring_count = max((ULONG)E1000_DEFAULT_RXD ,(ULONG)E1000_MIN_RXD);
            rx_ring_count = min(rx_ring_count,(ULONG)(mac_type < e1000_82544 ?
                E1000_MAX_RXD : E1000_MAX_82544_RXD));
            rx_ring_count = ALIGN(rx_ring_count, REQ_RX_DESCRIPTOR_MULTIPLE);

            tx_ring_count = max((ULONG)E1000_DEFAULT_TXD,(ULONG)E1000_MIN_TXD);
            tx_ring_count = min(tx_ring_count,(ULONG)(mac_type < e1000_82544 ?
                E1000_MAX_TXD : E1000_MAX_82544_TXD));
            tx_ring_count = ALIGN(tx_ring_count, REQ_TX_DESCRIPTOR_MULTIPLE);

            /* overwrite the counts with the new values */
            for (i = 0; i < unit->e1ku_txRing_QueueSize; i++)
                unit->e1ku_txRing[i].count = tx_ring_count;

            for (i = 0; i < unit->e1ku_rxRing_QueueSize; i++)
                unit->e1ku_rxRing[i].count = rx_ring_count;
                        
            if (e1000func_setup_all_tx_resources(unit))
                error = IOERR_OPENFAIL;
 
            /* allocate receive descriptors */
            if ((error == 0) && (e1000func_setup_all_rx_resources(unit)))
                error = IOERR_OPENFAIL;

            if ((error == 0) && (((struct e1000_hw *)unit->e1ku_Private00)->phy.media_type == e1000_media_type_copper))
                e1000_power_up_phy((struct e1000_hw *)unit->e1ku_Private00);

            if (error == 0)
                e1000func_configure(unit);

            if ((error == 0) && (request_irq(unit)))
            {
                error = IOERR_OPENFAIL;
            }
            else
            {
D(bug("[%s] OpenDevice: IRQ Attached\n", unit->e1ku_name));
            }
        }
    }
    else
    {
D(bug("[e1000] OpenDevice: Invalid Unit! (unitno = %d)\n", unitnum));
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
    struct e1000Unit *unit = (struct e1000Unit *)req->ios2_Req.io_Unit;
    struct Opener *opener;

D(bug("[e1000] CloseDevice(unit @ %p, unitno %d)\n", unit, unit->e1ku_UnitNum));

#warning "TODO: CloseDevice->stop"
//    unit->stop(unit);

    opener = (APTR)req->ios2_BufferManagement;
    if ((APTR)req->ios2_BufferManagement != NULL)
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
    LIBBASETYPEPTR, LIBBASE, 5, e1000)
{
    AROS_LIBFUNC_INIT
    struct e1000Unit *unit;

D(bug("[e1000] BeginIO()\n"));

    req->ios2_Req.io_Error = 0;
    if ((unit = (APTR)req->ios2_Req.io_Unit) != NULL)
    {
D(bug("[e1000] BeginIO: unit @ %p\n", unit));

        if (AttemptSemaphore(&unit->e1ku_unit_lock))
        {
D(bug("[e1000] BeginIO: Calling handle_request()\n"));
            handle_request(LIBBASE, req);
        }
        else
        {
D(bug("[e1000] BeginIO: Queueing request\n"));
            req->ios2_Req.io_Flags &= ~IOF_QUICK;
            PutMsg(unit->e1ku_input_port, (struct Message *)req);
        }
    }
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 6, e1000)
{
    AROS_LIBFUNC_INIT
    struct e1000Unit *unit;

D(bug("[e1000] AbortIO()\n"));

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
