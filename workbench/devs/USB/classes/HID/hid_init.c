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
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <hidd/hidd.h>
#include <usb/usb.h>

#define DEBUG 0

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include "hid.h"
#include LC_LIBDEFS_FILE

#undef SD
#define SD(x) (&LIBBASE->sd)

static int HID_Init(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] = {
            { (STRPTR)IID_Hidd,             &HiddAttrBase },
            { (STRPTR)IID_Hidd_USB,         &HiddUSBAttrBase },
            { (STRPTR)IID_Hidd_USBDevice,   &HiddUSBDeviceAttrBase },
            { (STRPTR)IID_Hidd_USBHub,      &HiddUSBHubAttrBase },
            { (STRPTR)IID_Hidd_USBHID,      &HiddUSBHIDAttrBase },
            { (STRPTR)IID_Hidd_USBDrv,      &HiddUSBDrvAttrBase },
            { NULL, NULL }
    };

    D(bug("[HID] USB HID Init.\n"));
    
    InitSemaphore(&LIBBASE->sd.Lock);
    
    if (OOP_ObtainAttrBases(attrbases))
    {
        if ((LIBBASE->sd.MemPool = CreatePool(MEMF_PUBLIC|MEMF_CLEAR|MEMF_SEM_PROTECTED, 8192, 4096)) != NULL)
        {
            D(bug("[HID] Init done.\n"));

            OOP_Object *usb = OOP_NewObject(NULL, CLID_Hidd_USB, NULL);
            if (usb)
                HIDD_USB_AddClass(usb, MOD_NAME_STRING);
            OOP_DisposeObject(usb);
            
            return TRUE;
        }
        
        OOP_ReleaseAttrBases(attrbases);
    }

    return FALSE;
}

static int HID_Expunge(LIBBASETYPEPTR LIBBASE)
{ 
    struct OOP_ABDescr attrbases[] = {
            { (STRPTR)IID_Hidd,             &HiddAttrBase },
            { (STRPTR)IID_Hidd_USB,         &HiddUSBAttrBase },
            { (STRPTR)IID_Hidd_USBDevice,   &HiddUSBDeviceAttrBase },
            { (STRPTR)IID_Hidd_USBHub,      &HiddUSBHubAttrBase },
            { (STRPTR)IID_Hidd_USBHID,      &HiddUSBHIDAttrBase },
            { (STRPTR)IID_Hidd_USBDrv,      &HiddUSBDrvAttrBase },
            { NULL, NULL }
    };

    D(bug("[HID] HID Expunge\n"));

    OOP_ReleaseAttrBases(attrbases);

    DeletePool(LIBBASE->sd.MemPool);

    return TRUE;
}

ADD2INITLIB(HID_Init, 0)
ADD2EXPUNGELIB(HID_Expunge, 0)

