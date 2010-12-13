#ifndef USB_H_
#define USB_H_

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

#include <stdint.h>

#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/ports.h>
#include <exec/interrupts.h>
#include <dos/bptr.h>
#include <dos/dosextens.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <oop/oop.h>

#include <aros/arossupportbase.h>
#include <exec/execbase.h>

#include LC_LIBDEFS_FILE

extern OOP_AttrBase HiddAttrBase;

#define BITMAP_SIZE		128
#define MAX_HUB_PORTS	256

typedef struct usb_driver {
    struct Node                 d_Node;
    struct SignalSemaphore      d_Lock;
    OOP_Object                  *d_Driver;
    uint32_t                    bitmap[BITMAP_SIZE/32];
} usb_driver_t;

struct usb_ExtClass {
    struct Node         ec_Node;
    const char *        ec_ShortName;
    struct Library      *ec_LibBase;
};

struct usb_staticdata
{
    struct SignalSemaphore      global_lock;
    void                        *MemPool;

    struct List                 driverList;
    struct SignalSemaphore      driverListLock;

    struct List                 extClassList;

    struct Process              *usbProcess;

    OOP_Object                  *usb;
    OOP_Class                   *usbClass;
    OOP_Class                   *deviceClass;
    OOP_Class                   *driverClass;
    OOP_Class                   *hubClass;
};

typedef struct {
    usb_endpoint_descriptor_t   *endpoint;
} EndpointData;

typedef struct {
    usb_interface_descriptor_t  *interface;
    EndpointData                *endpoints;
    int                         index;
    int                         altindex;
} InterfaceData;

typedef struct  {
    uint8_t                     address;
    uint8_t                     iface;
    uint8_t                     fast;
    uint8_t                     maxpacket;
    uint16_t                    langid;
    usb_device_descriptor_t     descriptor;
    usb_config_descriptor_t     *config_desc;
    InterfaceData               *interfaces;

    int                         config;

    char                        *product_name;
    char                        *manufacturer_name;
    char                        *serialnumber_name;

    void                        *default_pipe;
    struct usb_staticdata       *sd;
    OOP_Object                  *hub;
    OOP_Object                  *bus;
    struct timerequest          *tr;

    OOP_Object                  *next;
} DeviceData;

typedef struct  {
    struct Task                 *hub_task;
    struct MsgPort              *hub_port;
    struct timerequest          *tr;
    uint8_t                     sigInterrupt;

    STRPTR                      hub_name;
    STRPTR                      proc_name;
    struct usb_staticdata       *sd;
    OOP_Object                  **children;
    usb_hub_descriptor_t        descriptor;
    uint8_t                     got_descriptor;

    void                        *intr_pipe;

    uint8_t                     root;
    uint8_t                     enabled;
    uint8_t                     status[20];
    struct Interrupt            interrupt;
} HubData;

struct usbbase
{
    struct Library          LibNode;
    struct usb_staticdata   sd;
};

enum EventType {
    evt_Startup,                // Startup message
    evt_Cleanup,                // Cleanup message
    evt_Method,                 // Call method attached as ev_Event, put return value into ev_Retval, and reply
    evt_AsyncMethod,            // Call method attached as ev_Event, destroy original message
    evt_HubAttached,            // New hub (OOP_Object * in ev_RetVal) attached
    evt_OnOff,
};

struct usbEvent {
    struct Message      ev_Message;
    enum EventType      ev_Type;
    intptr_t            ev_RetVal;
    OOP_Object          *ev_Target;
    uint8_t             ev_Event[];
};

#define METHOD(base, id, name) \
    base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib)((struct usbbase*)(lib))
#define SD(cl) (&BASE(cl->UserData)->sd)

void setBitmap(uint32_t *bmp, uint8_t addr);
void freeBitmap(uint32_t *bmp, uint8_t addr);
uint8_t allocBitmap(uint32_t *bmp);

#endif /*USB_H_*/
