/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stub functions for USB subsystem
    Lang: English
 */

#ifndef AROS_USE_OOP
#   define AROS_USE_OOP
#endif

#include <stdint.h>
#include <exec/libraries.h>
#include <exec/interrupts.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <oop/oop.h>

#include <hidd/hidd.h>

#include <usb/usb.h>

#include <proto/oop.h>

#undef OOPBase
#define OOPBase (OOP_OOPBASE(obj))

#ifdef AROS_CREATE_ROM
#define STATIC_MID  OOP_MethodID mid = 0
#else
#define STATIC_MID  static OOP_MethodID mid
#endif

BOOL HIDD_USB_AttachDriver(OOP_Object *obj, OOP_Object *driver)
{
    STATIC_MID;
    struct pHidd_USB_AttachDriver p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USB, moHidd_USB_AttachDriver);

    p.mID = mid;
    p.driverObject = driver;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USB_DetachDriver(OOP_Object *obj, OOP_Object *driver)
{
    STATIC_MID;
    struct pHidd_USB_DetachDriver p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USB, moHidd_USB_DetachDriver);

    p.mID = mid;
    p.driverObject = driver;

    return OOP_DoMethod(obj, &p.mID);
}

void HIDD_USB_AddClass(OOP_Object *obj, const char *className)
{
    STATIC_MID;
    struct pHidd_USB_AddClass p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USB, moHidd_USB_AddClass);

    p.mID = mid;
    p.className = className;

    OOP_DoMethod(obj, &p.mID);
}

uint8_t HIDD_USB_AllocAddress(OOP_Object *obj, OOP_Object *driver)
{
    STATIC_MID;
    struct pHidd_USB_AllocAddress p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USB, moHidd_USB_AllocAddress);

    p.mID = mid;
    p.driverObject = driver;

    return OOP_DoMethod(obj, &p.mID);
}

void HIDD_USB_FreeAddress(OOP_Object *obj, OOP_Object *driver, uint8_t address)
{
    STATIC_MID;
    struct pHidd_USB_FreeAddress p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USB, moHidd_USB_FreeAddress);

    p.mID = mid;
    p.driverObject = driver;
    p.address = address;

    OOP_DoMethod(obj, &p.mID);
}

OOP_Object *HIDD_USB_NewDevice(OOP_Object *obj, OOP_Object *hub, BOOL fast)
{
    STATIC_MID;
    struct pHidd_USB_NewDevice p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USB, moHidd_USB_NewDevice);

    p.mID = mid;
    p.hub = hub;
    p.fast = fast;

    return (OOP_Object *)OOP_DoMethod(obj, &p.mID);
}



BOOL HIDD_USBDevice_GetDescriptor(OOP_Object *obj, uint8_t type, uint8_t index, uint16_t length, APTR descriptor)
{
    STATIC_MID;
    struct pHidd_USBDevice_GetDescriptor p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDevice, moHidd_USBDevice_GetDescriptor);

    p.mID = mid;
    p.type = type;
    p.index = index;
    p.length = length;
    p.descriptor = descriptor;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBDevice_GetConfigDescriptor(OOP_Object *obj, uint8_t index, usb_config_descriptor_t *descriptor)
{
    STATIC_MID;
    struct pHidd_USBDevice_GetConfigDescriptor p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDevice, moHidd_USBDevice_GetConfigDescriptor);

    p.mID = mid;
    p.index = index;
    p.descriptor = descriptor;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBDevice_GetDeviceDescriptor(OOP_Object *obj, usb_device_descriptor_t *descriptor)
{
    STATIC_MID;
    struct pHidd_USBDevice_GetDeviceDescriptor p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDevice, moHidd_USBDevice_GetDeviceDescriptor);

    p.mID = mid;
    p.descriptor = descriptor;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBDevice_GetStatus(OOP_Object *obj, usb_status_t *status)
{
    STATIC_MID;
    struct pHidd_USBDevice_GetStatus p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDevice, moHidd_USBDevice_GetStatus);

    p.mID = mid;
    p.status = status;

    return OOP_DoMethod(obj, &p.mID);
}


BOOL HIDD_USBDevice_GetString(OOP_Object *obj, int8_t id, uint16_t language, usb_string_descriptor_t *string)
{
    STATIC_MID;
    struct pHidd_USBDevice_GetString p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDevice, moHidd_USBDevice_GetString);

    p.mID = mid;
    p.id = id;
    p.language = language;
    p.string = string;

    return OOP_DoMethod(obj, &p.mID);
}

void * HIDD_USBDevice_CreatePipe(OOP_Object *obj, enum USB_PipeType type, uint8_t endpoint, uint8_t period, uint16_t maxpacket, uint32_t timeout)
{
    STATIC_MID;
    struct pHidd_USBDevice_CreatePipe p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDevice, moHidd_USBDevice_CreatePipe);

    p.mID = mid;
    p.type = type;
    p.endpoint = endpoint;
    p.period = period;
    p.maxpacket = maxpacket;
    p.timeout = timeout;

    return (void *) OOP_DoMethod(obj, &p.mID);
}

void HIDD_USBDevice_DeletePipe(OOP_Object *obj, APTR pipe)
{
    STATIC_MID;
    struct pHidd_USBDevice_DeletePipe p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDevice, moHidd_USBDevice_DeletePipe);

    p.mID = mid;
    p.pipe = pipe;

    OOP_DoMethod(obj, &p.mID);
}

void HIDD_USBDevice_SetTimeout(OOP_Object *obj, APTR pipe, uint32_t timeout)
{
    STATIC_MID;
    struct pHidd_USBDevice_SetTimeout p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDevice, moHidd_USBDevice_SetTimeout);

    p.mID = mid;
    p.pipe = pipe;
    p.timeout = timeout;

    OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBDevice_ControlMessage(OOP_Object *obj, APTR pipe, USBDevice_Request *request, APTR buffer, uint32_t length)
{
    STATIC_MID;
    struct pHidd_USBDevice_ControlMessage p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDevice, moHidd_USBDevice_ControlMessage);

    p.mID = mid;
    p.pipe = pipe;
    p.request = request;
    p.buffer = buffer;
    p.length = length;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBDevice_BulkTransfer(OOP_Object *obj, APTR pipe, APTR buffer, uint32_t length)
{
    STATIC_MID;
    struct pHidd_USBDevice_BulkTransfer p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDevice, moHidd_USBDevice_BulkTransfer);

    p.mID = mid;
    p.pipe = pipe;
    p.buffer = buffer;
    p.length = length;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBDevice_Configure(OOP_Object *obj, uint8_t configNr)
{
    STATIC_MID;
    struct pHidd_USBDevice_Configure p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDevice, moHidd_USBDevice_Configure);

    p.mID = mid;
    p.configNr = configNr;

    return OOP_DoMethod(obj, &p.mID);
}

usb_interface_descriptor_t *HIDD_USBDevice_GetInterface(OOP_Object *obj, uint8_t interface)
{
    STATIC_MID;
    struct pHidd_USBDevice_GetInterface p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDevice, moHidd_USBDevice_GetInterface);

    p.mID = mid;
    p.interface = interface;

    return (usb_interface_descriptor_t *)OOP_DoMethod(obj, &p.mID);
}

usb_endpoint_descriptor_t *HIDD_USBDevice_GetEndpoint(OOP_Object *obj, uint8_t interface, uint8_t endpoint)
{
    STATIC_MID;
    struct pHidd_USBDevice_GetEndpoint p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDevice, moHidd_USBDevice_GetEndpoint);

    p.mID = mid;
    p.interface = interface;
    p.endpoint = endpoint;

    return (usb_endpoint_descriptor_t *)OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBHub_OnOff(OOP_Object *obj, BOOL on)
{
    STATIC_MID;
    struct pHidd_USBHub_OnOff p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHub, moHidd_USBHub_OnOff);

    p.mID = mid;
    p.on = on;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBHub_PortEnable(OOP_Object *obj, uint8_t port, BOOL enable)
{
    STATIC_MID;
    struct pHidd_USBHub_PortEnable p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHub, moHidd_USBHub_PortEnable);

    p.mID = mid;
    p.portNummer = port;
    p.enable = enable;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBHub_PortReset(OOP_Object *obj, uint8_t port)
{
    STATIC_MID;
    struct pHidd_USBHub_PortReset p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHub, moHidd_USBHub_PortReset);

    p.mID = mid;
    p.portNummer = port;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBHub_GetPortStatus(OOP_Object *obj, uint8_t port, usb_port_status_t *status)
{
    STATIC_MID;
    struct pHidd_USBHub_GetPortStatus p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHub, moHidd_USBHub_GetPortStatus);

    p.mID = mid;
    p.port = port;
    p.status = status;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBHub_GetHubStatus(OOP_Object *obj, usb_hub_status_t *status)
{
    STATIC_MID;
    struct pHidd_USBHub_GetHubStatus p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHub, moHidd_USBHub_GetHubStatus);

    p.mID = mid;
    p.status = status;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBHub_ClearHubFeature(OOP_Object *obj, int feature)
{
    STATIC_MID;
    struct pHidd_USBHub_ClearHubFeature p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHub, moHidd_USBHub_ClearHubFeature);

    p.mID = mid;
    p.feature = feature;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBHub_SetHubFeature(OOP_Object *obj, int feature)
{
    STATIC_MID;
    struct pHidd_USBHub_SetHubFeature p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHub, moHidd_USBHub_SetHubFeature);

    p.mID = mid;
    p.feature = feature;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBHub_ClearPortFeature(OOP_Object *obj, uint8_t port, int feature)
{
    STATIC_MID;
    struct pHidd_USBHub_ClearPortFeature p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHub, moHidd_USBHub_ClearPortFeature);

    p.mID = mid;
    p.port = port;
    p.feature = feature;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBHub_SetPortFeature(OOP_Object *obj, uint8_t port, int feature)
{
    STATIC_MID;
    struct pHidd_USBHub_SetPortFeature p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHub, moHidd_USBHub_SetPortFeature);

    p.mID = mid;
    p.port = port;
    p.feature = feature;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBHub_GetHubDescriptor(OOP_Object *obj, usb_hub_descriptor_t *descriptor)
{
    STATIC_MID;
    struct pHidd_USBHub_GetHubDescriptor p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHub, moHidd_USBHub_GetHubDescriptor);

    p.mID = mid;
    p.descriptor = descriptor;

    return OOP_DoMethod(obj, &p.mID);
}

OOP_Object *HIDD_USBHub_GetChild(OOP_Object *obj, uint8_t port)
{
    STATIC_MID;
    struct pHidd_USBHub_GetChild p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHub, moHidd_USBHub_GetChild);

    p.mID = mid;
    p.port = port;

    return OOP_DoMethod(obj, &p.mID);

}

APTR HIDD_USBDrv_CreatePipe(OOP_Object *obj, enum USB_PipeType	type,
        BOOL fullspeed, uint8_t address, uint8_t endpoint, uint8_t period, uint32_t maxpacket, uint32_t timeout)
{
    STATIC_MID;
    struct pHidd_USBDrv_CreatePipe p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDrv, moHidd_USBDrv_CreatePipe);

    p.mID = mid;
    p.type = type;
    p.fullspeed = fullspeed;
    p.address = address;
    p.endpoint = endpoint;
    p.period = period;
    p.maxpacket = maxpacket;
    p.timeout = timeout;

    return (APTR)OOP_DoMethod(obj, &p.mID);
}

void HIDD_USBDrv_DeletePipe(OOP_Object *obj, APTR pipe)
{
    STATIC_MID;
    struct pHidd_USBDrv_DeletePipe p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDrv, moHidd_USBDrv_DeletePipe);

    p.mID = mid;
    p.pipe = pipe;

    OOP_DoMethod(obj, &p.mID);
}

void HIDD_USBDrv_SetTimeout(OOP_Object *obj, APTR pipe, uint32_t timeout)
{
    STATIC_MID;
    struct pHidd_USBDrv_SetTimeout p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDrv, moHidd_USBDrv_SetTimeout);

    p.mID = mid;
    p.pipe = pipe;
    p.timeout = timeout;

    OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBDrv_ControlTransfer(OOP_Object *obj, APTR pipe, USBDevice_Request *request,
        APTR buffer, uint32_t length)
{
    STATIC_MID;
    struct pHidd_USBDrv_ControlTransfer p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDrv, moHidd_USBDrv_ControlTransfer);

    p.mID = mid;
    p.pipe = pipe;
    p.request = request;
    p.buffer = buffer;
    p.length = length;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBDrv_BulkTransfer(OOP_Object *obj, APTR pipe, APTR buffer, uint32_t length)
{
    STATIC_MID;
    struct pHidd_USBDrv_BulkTransfer p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDrv, moHidd_USBDrv_BulkTransfer);

    p.mID = mid;
    p.pipe = pipe;
    p.buffer = buffer;
    p.length = length;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBDrv_AddInterrupt(OOP_Object *obj, void *pipe, void *buffer, uint8_t length, struct Interrupt *interrupt)
{
    STATIC_MID;
    struct pHidd_USBDrv_AddInterrupt p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDrv, moHidd_USBDrv_AddInterrupt);

    p.mID = mid;
    p.pipe = pipe;
    p.buffer = buffer;
    p.length = length;
    p.interrupt = interrupt;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBDrv_RemInterrupt(OOP_Object *obj, void *pipe, struct Interrupt *interrupt)
{
    STATIC_MID;
    struct pHidd_USBDrv_RemInterrupt p;

    if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBDrv, moHidd_USBDrv_RemInterrupt);

    p.mID = mid;
    p.pipe = pipe;
    p.interrupt = interrupt;

    return OOP_DoMethod(obj, &p.mID);
}
