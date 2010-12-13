#ifndef USB_USB_H
#define USB_USB_H

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

#include <exec/ports.h>
#include <exec/interrupts.h>
#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <utility/tagitem.h>
#include <usb/usb_core.h>

enum USB_PipeType {
    PIPE_Isochronous,
    PIPE_Control,
    PIPE_Bulk,
    PIPE_Interrupt
};

/*
 * Base USB class
 */

#define CLID_Hidd_USB    "Bus::USB"
#define IID_Hidd_USB     "IBus::USB"

#define HiddUSBAttrBase __IHidd_USB

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddUSBAttrBase;
#endif

enum {
    moHidd_USB_AttachDriver,
    moHidd_USB_DetachDriver,
    moHidd_USB_AddClass,
    moHidd_USB_AllocAddress,
    moHidd_USB_FreeAddress,
    moHidd_USB_NewDevice,		// Internal call used to attach new USB devices to the system

    NUM_HIDD_USB_METHODS
};

enum {
    aoHidd_USB_Bus,

    num_Hidd_USB_Attrs
};

#define aHidd_USB_Bus             (HiddUSBAttrBase + aoHidd_USB_Bus)

#define IS_USB_ATTR(attr, idx) \
    (((idx) = (attr) - HiddUSBAttrBase) < num_Hidd_USB_Attrs)

/// The Method arguments for USB::AttachDriver call
struct pHidd_USB_AttachDriver {
    OOP_MethodID    mID;
    OOP_Object		*driverObject;
};

struct pHidd_USB_DetachDriver {
    OOP_MethodID        mID;
    OOP_Object		*driverObject;
};

struct pHidd_USB_AddClass {
    OOP_MethodID        mID;
    const char          *className;
};

struct pHidd_USB_AllocAddress {
    OOP_MethodID	mID;
    OOP_Object		*driverObject;
};

struct pHidd_USB_FreeAddress {
    OOP_MethodID	mID;
    OOP_Object		*driverObject;
    uint8_t			address;
};

struct pHidd_USB_NewDevice {
    OOP_MethodID			mID;
    OOP_Object				*hub;		// New device attached to this hub
    usb_device_descriptor_t	*descriptor;
    uint8_t					fast;
};

/*
 * USB Device Class
 */

#define CLID_Hidd_USBDevice "Hidd::USBDevice"
#define IID_Hidd_USBDevice	"IHidd::USBDevice"

#define HiddUSBDeviceAttrBase __IHidd_USBDevice

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddUSBDeviceAttrBase;
#endif

enum {
    moHidd_USBDevice_GetString,
    moHidd_USBDevice_CreatePipe,
    moHidd_USBDevice_DeletePipe,
    moHidd_USBDevice_ControlMessage,
    moHidd_USBDevice_GetDescriptor,
    moHidd_USBDevice_GetConfigDescriptor,
    moHidd_USBDevice_GetDeviceDescriptor,
    moHidd_USBDevice_GetStatus,
    moHidd_USBDevice_Configure,
    moHidd_USBDevice_GetInterface,
    moHidd_USBDevice_GetEndpoint,
    moHidd_USBDevice_BulkTransfer,
    moHidd_USBDevice_SetTimeout,

    NUM_HIDD_USBDEVICE_METHODS
};

enum {
    aoHidd_USBDevice_Address,
    aoHidd_USBDevice_Bus,
    aoHidd_USBDevice_Hub,
    aoHidd_USBDevice_ProductID,
    aoHidd_USBDevice_VendorID,
    aoHidd_USBDevice_Interface,
    aoHidd_USBDevice_InterfaceNumber,

    aoHidd_USBDevice_ProductName,
    aoHidd_USBDevice_ManufacturerName,
    aoHidd_USBDevice_SerialNumber,

    aoHidd_USBDevice_Fast,
    aoHidd_USBDevice_MaxPacketSize,

    aoHidd_USBDevice_Next,

    num_Hidd_USBDevice_Attrs
};

#define aHidd_USBDevice_Address             (HiddUSBDeviceAttrBase + aoHidd_USBDevice_Address)
#define aHidd_USBDevice_Bus                 (HiddUSBDeviceAttrBase + aoHidd_USBDevice_Bus)
#define aHidd_USBDevice_Hub                 (HiddUSBDeviceAttrBase + aoHidd_USBDevice_Hub)
#define aHidd_USBDevice_ProductID           (HiddUSBDeviceAttrBase + aoHidd_USBDevice_ProductID)
#define aHidd_USBDevice_VendorID            (HiddUSBDeviceAttrBase + aoHidd_USBDevice_VendorID)
#define aHidd_USBDevice_Interface           (HiddUSBDeviceAttrBase + aoHidd_USBDevice_Interface)
#define aHidd_USBDevice_InterfaceNumber     (HiddUSBDeviceAttrBase + aoHidd_USBDevice_InterfaceNumber)
#define aHidd_USBDevice_ProductName         (HiddUSBDeviceAttrBase + aoHidd_USBDevice_ProductName)
#define aHidd_USBDevice_ManufacturerName    (HiddUSBDeviceAttrBase + aoHidd_USBDevice_ManufacturerName)
#define aHidd_USBDevice_SerialNumber        (HiddUSBDeviceAttrBase + aoHidd_USBDevice_SerialNumber)

#define aHidd_USBDevice_Fast                (HiddUSBDeviceAttrBase + aoHidd_USBDevice_Fast)
#define aHidd_USBDevice_MaxPacketSize       (HiddUSBDeviceAttrBase + aoHidd_USBDevice_MaxPacketSize)

#define aHidd_USBDevice_Next                (HiddUSBDeviceAttrBase + aoHidd_USBDevice_Next)

#define IS_USBDEVICE_ATTR(attr, idx) \
    (((idx) = (attr) - HiddUSBDeviceAttrBase) < num_Hidd_USBDevice_Attrs)

struct pHidd_USBDevice_GetDescriptor {
    OOP_MethodID    mID;
    uint8_t           type;
    uint8_t           index;
    uint16_t           length;
    APTR            descriptor;
};

struct pHidd_USBDevice_GetConfigDescriptor {
    OOP_MethodID    mID;
    uint8_t           index;
    usb_config_descriptor_t *descriptor;
};

struct pHidd_USBDevice_GetDeviceDescriptor {
    OOP_MethodID    mID;
    usb_device_descriptor_t *descriptor;
};

struct pHidd_USBDevice_GetStatus {
    OOP_MethodID    mID;
    usb_status_t *status;
};

struct pHidd_USBDevice_GetString {
    OOP_MethodID            mID;
    int8_t			        id;
    uint16_t			        language;
    usb_string_descriptor_t *string;
};

struct pHidd_USBDevice_ControlMessage {
    OOP_MethodID       mID;
    APTR               pipe;
    USBDevice_Request  *request;
    APTR               buffer;
    uint32_t              length;
};

struct pHidd_USBDevice_CreatePipe {
    OOP_MethodID        mID;
    enum USB_PipeType  type;
    uint8_t             endpoint;
    uint8_t             period;
    uint16_t			maxpacket;
    uint32_t            timeout;
};

struct pHidd_USBDevice_DeletePipe {
    OOP_MethodID        mID;
    APTR                pipe;
};

struct pHidd_USBDevice_Configure {
    OOP_MethodID        mID;
    uint8_t             configNr;
};

struct pHidd_USBDevice_GetInterface {
    OOP_MethodID        mID;
    uint8_t             interface;
};

struct pHidd_USBDevice_GetEndpoint {
    OOP_MethodID        mID;
    uint8_t             interface;
    uint8_t             endpoint;
};

struct pHidd_USBDevice_BulkTransfer {
	OOP_MethodID	mID;
	APTR			pipe;
	APTR			buffer;
	uint32_t		length;
};

struct pHidd_USBDevice_SetTimeout {
    OOP_MethodID        mID;
    APTR                pipe;
    uint32_t			timeout;
};


/*
 * USB Hub Class
 */

#define CLID_Hidd_USBHub    "Hidd::USBHub"
#define IID_Hidd_USBHub		"IHidd::USBHub"

#define HiddUSBHubAttrBase __IHidd_USBHub

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddUSBHubAttrBase;
#endif

enum {
    moHidd_USBHub_OnOff,
    moHidd_USBHub_PortEnable,
    moHidd_USBHub_PortReset,
    moHidd_USBHub_GetPortStatus,
    moHidd_USBHub_GetHubStatus,
    moHidd_USBHub_ClearHubFeature,
    moHidd_USBHub_SetHubFeature,
    moHidd_USBHub_ClearPortFeature,
    moHidd_USBHub_SetPortFeature,
    moHidd_USBHub_GetChild,
    moHidd_USBHub_GetHubDescriptor,

    NUM_HIDD_USBHUB_METHODS
};

struct pHidd_USBHub_OnOff {
    OOP_MethodID    mID;
    BOOL			on;
};

struct pHidd_USBHub_PortEnable {
    OOP_MethodID    mID;
    uint8_t			portNummer;
    BOOL			enable;
};

struct pHidd_USBHub_PortReset {
    OOP_MethodID    mID;
    uint8_t			portNummer;
};

struct pHidd_USBHub_GetPortStatus {
    OOP_MethodID        mID;
    uint8_t             port;
    usb_port_status_t   *status;
};

struct pHidd_USBHub_GetHubStatus {
    OOP_MethodID        mID;
    usb_hub_status_t    *status;
};

struct pHidd_USBHub_ClearHubFeature {
    OOP_MethodID        mID;
    int                 feature;
};

struct pHidd_USBHub_SetHubFeature {
    OOP_MethodID        mID;
    int                 feature;
};

struct pHidd_USBHub_ClearPortFeature {
    OOP_MethodID        mID;
    uint8_t             port;
    int                 feature;
};

struct pHidd_USBHub_SetPortFeature {
    OOP_MethodID        mID;
    uint8_t             port;
    int                 feature;
};

struct pHidd_USBHub_GetChild {
    OOP_MethodID    mID;
    int8_t            port;
};

struct pHidd_USBHub_GetHubDescriptor {
    OOP_MethodID    mID;
    usb_hub_descriptor_t *descriptor;
};

enum {
    aoHidd_USBHub_IsRoot,
    aoHidd_USBHub_IsCompound,
    aoHidd_USBHub_HubCurrent,
    aoHidd_USBHub_NumPorts,

    num_Hidd_USBHub_Attrs
};

#define aHidd_USBHub_IsRoot			(HiddUSBHubAttrBase + aoHidd_USBHub_IsRoot)
#define aHidd_USBHub_IsCompound		(HiddUSBHubAttrBase + aoHidd_USBHub_IsCompound)
#define aHidd_USBHub_HubCurrent		(HiddUSBHubAttrBase + aoHidd_USBHub_HubCurrent)
#define aHidd_USBHub_NumPorts		(HiddUSBHubAttrBase + aoHidd_USBHub_NumPorts)

#define IS_USBHUB_ATTR(attr, idx) \
    (((idx) = (attr) - HiddUSBHubAttrBase) < num_Hidd_USBHub_Attrs)


#define CLID_Hidd_USBDrv    "Hidd::USBDriver"
#define IID_Hidd_USBDrv		"IHidd::USBDriver"

#define HiddUSBDrvAttrBase __IHidd_USBDrv

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddUSBDrvAttrBase;
#endif

enum {
    moHidd_USBDrv_CreatePipe,
    moHidd_USBDrv_DeletePipe,
    moHidd_USBDrv_ControlTransfer,
    moHidd_USBDrv_AddInterrupt,
    moHidd_USBDrv_RemInterrupt,
    moHidd_USBDrv_BulkTransfer,
    moHidd_USBDrv_SetTimeout,

    NUM_HIDD_USBDRV_METHODS
};

enum {
    num_Hidd_USBDrv_Attrs
};

#define IS_USBDRV_ATTR(attr, idx) \
    (((idx) = (attr) - HiddUSBDrvAttrBase) < num_Hidd_USBDrv_Attrs)

struct pHidd_USBDrv_CreatePipe {
    OOP_MethodID        mID;
    enum USB_PipeType   type;
    BOOL                fullspeed;
    uint8_t               address;
    uint8_t               endpoint;
    uint8_t               period;
    uint32_t               maxpacket;
    uint32_t               timeout;
};

struct pHidd_USBDrv_DeletePipe {
    OOP_MethodID    mID;
    void            *pipe;
};

struct pHidd_USBDrv_ControlTransfer {
    OOP_MethodID		mID;
    APTR				pipe;
    USBDevice_Request	*request;
    APTR				buffer;
    uint32_t			length;
};

struct pHidd_USBDrv_BulkTransfer {
	OOP_MethodID	mID;
	APTR			pipe;
	APTR			buffer;
	uint32_t		length;
};

struct pHidd_USBDrv_AddInterrupt {
    OOP_MethodID        mID;
    void                *pipe;
    void                *buffer;
    uint8_t             length;
    struct Interrupt    *interrupt;
};

struct pHidd_USBDrv_RemInterrupt {
    OOP_MethodID        mID;
    void                *pipe;
    struct Interrupt    *interrupt;
};

struct pHidd_USBDrv_SetTimeout {
    OOP_MethodID    mID;
    void            *pipe;
    uint32_t		timeout;
};


/* USB stubs */
BOOL HIDD_USB_AttachDriver(OOP_Object *obj, OOP_Object *driver);
BOOL HIDD_USB_DetachDriver(OOP_Object *obj, OOP_Object *driver);
void HIDD_USB_AddClass(OOP_Object *obj, const char *className);
uint8_t HIDD_USB_AllocAddress(OOP_Object *obj, OOP_Object *driver);
void HIDD_USB_FreeAddress(OOP_Object *obj, OOP_Object *driver, uint8_t address);
OOP_Object *HIDD_USB_NewDevice(OOP_Object *obj, OOP_Object *hub, BOOL fast);

BOOL HIDD_USBDevice_GetDescriptor(OOP_Object *obj, uint8_t type, uint8_t index, uint16_t length, APTR descriptor);
BOOL HIDD_USBDevice_GetConfigDescriptor(OOP_Object *obj, uint8_t index, usb_config_descriptor_t *descriptor);
BOOL HIDD_USBDevice_GetDeviceDescriptor(OOP_Object *obj, usb_device_descriptor_t *descriptor);
BOOL HIDD_USBDevice_GetStatus(OOP_Object *obj, usb_status_t *status);
APTR HIDD_USBDevice_CreatePipe(OOP_Object *obj, enum USB_PipeType type, uint8_t endpoint, uint8_t period, uint16_t maxpacket, uint32_t timeout);
void HIDD_USBDevice_DeletePipe(OOP_Object *obj, APTR pipe);
BOOL HIDD_USBDevice_GetString(OOP_Object *obj, int8_t id, uint16_t language, usb_string_descriptor_t *string);
BOOL HIDD_USBDevice_ControlMessage(OOP_Object *obj, APTR pipe, USBDevice_Request *request, APTR buffer, uint32_t length);
BOOL HIDD_USBDevice_Configure(OOP_Object *obj, uint8_t configNr);
usb_interface_descriptor_t *HIDD_USBDevice_GetInterface(OOP_Object *obj, uint8_t interface);
usb_endpoint_descriptor_t *HIDD_USBDevice_GetEndpoint(OOP_Object *obj, uint8_t interface, uint8_t endpoint);
BOOL HIDD_USBDevice_BulkTransfer(OOP_Object *obj, APTR pipe, APTR buffer, uint32_t length);
void HIDD_USBDevice_SetTimeout(OOP_Object *obj, APTR pipe, uint32_t timeout);

BOOL HIDD_USBHub_OnOff(OOP_Object *obj, BOOL on);
OOP_Object *HIDD_USBHub_GetChild(OOP_Object *obj, uint8_t port);
BOOL HIDD_USBHub_PortEnable(OOP_Object *obj, uint8_t port, BOOL enable);
BOOL HIDD_USBHub_PortReset(OOP_Object *obj, uint8_t port);
BOOL HIDD_USBHub_GetPortStatus(OOP_Object *obj, uint8_t port, usb_port_status_t *status);
BOOL HIDD_USBHub_GetHubStatus(OOP_Object *obj, usb_hub_status_t *status);
BOOL HIDD_USBHub_ClearHubFeature(OOP_Object *obj, int feature);
BOOL HIDD_USBHub_SetHubFeature(OOP_Object *obj, int feature);
BOOL HIDD_USBHub_ClearPortFeature(OOP_Object *obj, uint8_t port, int feature);
BOOL HIDD_USBHub_SetPortFeature(OOP_Object *obj, uint8_t port, int feature);
BOOL HIDD_USBHub_GetHubDescriptor(OOP_Object *obj, usb_hub_descriptor_t *descriptor);

APTR HIDD_USBDrv_CreatePipe(OOP_Object *obj, enum USB_PipeType type, BOOL fullspeed, uint8_t address, uint8_t endpoint, uint8_t period, uint32_t maxpacket, uint32_t timeout);
void HIDD_USBDrv_DeletePipe(OOP_Object *obj, APTR pipe);
BOOL HIDD_USBDrv_ControlTransfer(OOP_Object *obj, APTR pipe, USBDevice_Request *request, APTR buffer, uint32_t length);
BOOL HIDD_USBDrv_AddInterrupt(OOP_Object *obj, void *pipe, void *buffer, uint8_t length, struct Interrupt *interrupt);
BOOL HIDD_USBDrv_RemInterrupt(OOP_Object *obj, void *pipe, struct Interrupt *interrupt);
BOOL HIDD_USBDrv_BulkTransfer(OOP_Object *obj, APTR pipe, APTR buffer, uint32_t length);
void HIDD_USBDrv_SetTimeout(OOP_Object *obj, APTR pipe, uint32_t timeout);

#endif /*USB_USB_H*/
