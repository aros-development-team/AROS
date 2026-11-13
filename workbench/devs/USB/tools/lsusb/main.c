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

#define DEBUG 1

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>

#include <usb/usb.h>
#include <usb/hid.h>
#include <usb/usb_core.h>

#include <stdio.h>

OOP_AttrBase HiddUSBAttrBase;
OOP_AttrBase HiddUSBDeviceAttrBase;
OOP_AttrBase HiddUSBHubAttrBase;

void scan_hub(OOP_Object *hub, int depth)
{
    intptr_t nports, i;
    intptr_t address = 0;
    intptr_t productid=0, vendorid=0;
    STRPTR name = NULL;

    char pad[depth+1];
    for (i=0; i < depth; i++)
        pad[i]=' ';
    pad[depth] = 0;
    
    OOP_GetAttr(hub, aHidd_USBHub_NumPorts, &nports);
    OOP_GetAttr(hub, aHidd_USBDevice_ProductName, (intptr_t *)&name);
    OOP_GetAttr(hub, aHidd_USBDevice_Address, &address);
    OOP_GetAttr(hub, aHidd_USBDevice_ProductID, &productid);
    OOP_GetAttr(hub, aHidd_USBDevice_VendorID, &vendorid);
    
    D(bug("[lsusb] hub with %d ports\n", nports));
    
    printf("%s%03d: %04x:%04x %s\n", pad, (int)address, (int)productid, (int)vendorid, name);
    
    for (i=1; i <= nports; i++)
    {
        
        OOP_Object *child = HIDD_USBHub_GetChild(hub, i);
        if (child)
        {
            intptr_t ports = 0;
            OOP_GetAttr(child, aHidd_USBHub_NumPorts, &ports);
            OOP_GetAttr(child, aHidd_USBDevice_ProductName, (intptr_t *)&name);
            OOP_GetAttr(child, aHidd_USBDevice_Address, &address);
            OOP_GetAttr(child, aHidd_USBDevice_ProductID, &productid);
            OOP_GetAttr(child, aHidd_USBDevice_VendorID, &vendorid);

            if (ports > 0)
                scan_hub(child, depth+1);
            else
                printf("%s %03d: %04x:%04x %s\n", pad, (int)address, (int)productid, (int)vendorid, name);

            D(bug("[lsusb] Child %d: %p (%s) with %d ports\n", i, child, name, nports));
        }
    }
}

int main(void)
{
    struct OOP_ABDescr attrbases[] = {
            { (STRPTR)IID_Hidd_USB,         &HiddUSBAttrBase },
            { (STRPTR)IID_Hidd_USBDevice,   &HiddUSBDeviceAttrBase },
            { (STRPTR)IID_Hidd_USBHub,      &HiddUSBHubAttrBase },
            { NULL, NULL }
    };

    OOP_ObtainAttrBases(attrbases);
    
    OOP_Object *usb = OOP_NewObject(NULL, CLID_Hidd_USB, NULL);
    OOP_Object *bus = NULL;
    
    D(bug("[lsusb] USB object @ %p\n", usb));
    
    if(usb)
    {
        PutStr("USB device tree:\n");

        do {
            OOP_GetAttr(usb, aHidd_USB_Bus, (IPTR *)&bus);
            if (bus)
            {
                D(bug("[lsusb] bus=%p\n", bus));
            
                printf("Bus %p:\n", bus);
                scan_hub(bus, 1);
            }
                
        } while(bus);
        OOP_DisposeObject(usb);
 
    }else
    {
        PutStr("Please use C:LoadResource...\n");
    }

    OOP_ReleaseAttrBases(attrbases);
    
    return RETURN_OK;
}
