/*
    Copyright (C) 2006 by Michal Schulz
    $Id$

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as 
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <aros/symbolsets.h>

#include <exec/types.h>
#include <oop/oop.h>

#include <asm/io.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>

#include <usb/usb.h>

#define DEBUG 0

#include <proto/exec.h>
#include <proto/oop.h>
#include <aros/debug.h>

#include "uhci.h"

#undef SD
#define SD(x) (&LIBBASE->sd)

/*
 * usb_delay() stops waits for specified amount of miliseconds. It uses the timerequest
 * of specified USB device. No pre-allocation of signals is required.
 */
static void USBDelay(struct timerequest *tr, uint32_t msec)
{
    /* Allocate a signal within this task context */
    tr->tr_node.io_Message.mn_ReplyPort->mp_SigBit = AllocSignal(-1);
    tr->tr_node.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);
    
    /* Specify the request */
    tr->tr_node.io_Command = TR_ADDREQUEST;
    tr->tr_time.tv_secs = msec / 1000;
    tr->tr_time.tv_micro = 1000 * (msec % 1000);

    /* Wait */
    DoIO((struct IORequest *)tr);
    
    /* The signal is not needed anymore */
    FreeSignal(tr->tr_node.io_Message.mn_ReplyPort->mp_SigBit);
    tr->tr_node.io_Message.mn_ReplyPort->mp_SigTask = NULL;
}


static struct timerequest *USBCreateTimer()
{
    struct timerequest *tr = NULL;
    struct MsgPort *mp = NULL;
    
    mp = CreateMsgPort();
    if (mp)
    {        
        tr = (struct timerequest *)CreateIORequest(mp, sizeof(struct timerequest));
        if (tr)
        {
            FreeSignal(mp->mp_SigBit);
            if (!OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ, (struct IORequest *)tr, 0))
                return tr;
            
            DeleteIORequest((struct IORequest *)tr);
            mp->mp_SigBit = AllocSignal(-1);
        }
        DeleteMsgPort(mp);
    }
    
    return NULL;
}

static void USBDeleteTimer(struct timerequest *tr)
{
    if (tr)
    {
        tr->tr_node.io_Message.mn_ReplyPort->mp_SigBit = AllocSignal(-1);
        CloseDevice((struct IORequest *)tr);
        DeleteMsgPort(tr->tr_node.io_Message.mn_ReplyPort);
        DeleteIORequest((struct IORequest *)tr);
    }
}

static
AROS_UFH3(void, Enumerator,
        AROS_UFHA(struct Hook *,        hook,           A0),
        AROS_UFHA(OOP_Object *,         pciDevice,      A2),
        AROS_UFHA(APTR,                 message,        A1))
{
    AROS_USERFUNC_INIT

    static int counter;

    if (counter == MAX_DEVS)
        return;

    LIBBASETYPE *LIBBASE = (LIBBASETYPE *)hook->h_Data;

    struct TagItem attrs[] = {
            { aHidd_PCIDevice_isIO,     TRUE },
            { aHidd_PCIDevice_isMaster, TRUE },
            { TAG_DONE, 0UL },
    };

    OOP_SetAttrs(pciDevice, (struct TagItem *)attrs);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base4, &LIBBASE->sd.iobase[counter]);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&LIBBASE->sd.uhciPCIDriver[counter]);
    LIBBASE->sd.uhciDevice[counter] = pciDevice;

    D(bug("[UHCI]   Device %d @ %08x with IO @ %08x\n", counter, pciDevice, LIBBASE->sd.iobase[counter]));

    D(bug("[UHCI]   Performing full reset. Hopefull legacy USB will do it'S handoff at this time.\n"));
    HIDD_PCIDevice_WriteConfigWord(pciDevice, PCI_LEGSUP, 0x8f00);
    
    outw(UHCI_CMD_HCRESET, LIBBASE->sd.iobase[counter] + UHCI_CMD);
    struct timerequest *tr = USBCreateTimer();
    USBDelay(tr, 10);
    USBDeleteTimer(tr);
    if (inw((uint16_t *)(LIBBASE->sd.iobase[counter] + UHCI_CMD)) & UHCI_CMD_HCRESET)
        D(bug("[UHCI]   Wrrr. Reset not yet completed\n"));
    
    outw(0, LIBBASE->sd.iobase[counter] + UHCI_INTR);  
    outw(0, LIBBASE->sd.iobase[counter] + UHCI_CMD);
    
    
    D(bug("[UHCI]    0xc0: %04x\n", HIDD_PCIDevice_ReadConfigWord(pciDevice, 0xc0)));

    LIBBASE->sd.num_devices = ++counter;

    AROS_USERFUNC_EXIT
}

static int UHCI_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[UHCI] UHCI_Init()\n"));

    LIBBASE->sd.usb = OOP_NewObject(NULL, (STRPTR)CLID_Hidd_USB, NULL);

    if (!LIBBASE->sd.usb)
    {
        bug("[UHCI] Cannot create the instance of base USB class\n");
        return FALSE;
    }

    if ((LIBBASE->sd.pci=OOP_NewObject(NULL, (STRPTR)IID_Hidd_PCI, NULL)))
    {
        struct TagItem tags[] = {
                { tHidd_PCI_Class,      PCI_BASE_CLASS_SERIAL },
                { tHidd_PCI_SubClass,   PCI_SUB_CLASS_USB },
                { tHidd_PCI_Interface,  PCI_INTERFACE_UHCI },
                { TAG_DONE, 0UL }
        };

        struct OOP_ABDescr attrbases[] = {
                { (STRPTR)IID_Hidd,             &HiddAttrBase },
                { (STRPTR)IID_Hidd_PCIDevice,   &HiddPCIDeviceAttrBase },
                { (STRPTR)IID_Hidd_USBDevice,   &HiddUSBDeviceAttrBase },
                { (STRPTR)IID_Hidd_USBHub,      &HiddUSBHubAttrBase },
                { (STRPTR)IID_Hidd_USBDrv,      &HiddUSBDrvAttrBase },
                { NULL, NULL }
        };

        struct Hook FindHook = {
                h_Entry:        (IPTR (*)())Enumerator,
                h_Data:         LIBBASE,
        };

        NEWLIST(&LIBBASE->sd.td_list);
        InitSemaphore(&LIBBASE->sd.global_lock);

        OOP_ObtainAttrBases(attrbases);

        D(bug("[UHCI] Searching for UHCI devices...\n"));

        HIDD_PCI_EnumDevices(LIBBASE->sd.pci, &FindHook, (struct TagItem *)&tags);

        D(bug("[UHCI] Done. UHCI devices found: %d\n", LIBBASE->sd.num_devices));

        if (LIBBASE->sd.num_devices > 0)
        {
            LIBBASE->sd.MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED, 8192, 4096);

            if (LIBBASE->sd.MemPool)
                return TRUE;
        }
    }

    return FALSE;
}

static int UHCI_Expunge(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] = {
            { (STRPTR)IID_Hidd,                 &HiddAttrBase },
            { (STRPTR)IID_Hidd_PCIDevice,       &HiddPCIDeviceAttrBase },
            { (STRPTR)IID_Hidd_USBDevice,       &HiddUSBDeviceAttrBase },
            { (STRPTR)IID_Hidd_USBHub,          &HiddUSBHubAttrBase },
            { (STRPTR)IID_Hidd_USBDrv,          &HiddUSBDrvAttrBase },
            { NULL, NULL }
    };

    OOP_ReleaseAttrBases(attrbases);

    return TRUE;	
}

ADD2INITLIB(UHCI_Init, 0)
ADD2EXPUNGELIB(UHCI_Expunge, 0)
ADD2LIBS((STRPTR)"usb.hidd", 0, static struct Library *, __usbbase)
