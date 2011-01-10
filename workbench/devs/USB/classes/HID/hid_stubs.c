/*
    Copyright � 2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stub functions for PCI subsystem
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
#include <oop/static_mid.h>

#include <hidd/hidd.h>
#include <usb/usb.h>
#include <usb/hid.h>

#include <proto/oop.h>

#undef OOPBase
#define OOPBase (OOP_OOPBASE(obj))

BOOL HIDD_USBHID_GetReportDescriptor(OOP_Object *obj, uint16_t length, void *buffer)
{
    STATIC_MID;
    struct pHidd_USBHID_GetReportDescriptor p;

    if (!static_mid) static_mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHID, moHidd_USBHID_GetReportDescriptor);

    p.mID = static_mid;
    p.length = length;
    p.buffer = buffer;

    return OOP_DoMethod(obj, &p.mID);
}

usb_hid_descriptor_t *HIDD_USBHID_GetHidDescriptor(OOP_Object *obj)
{
    STATIC_MID;
    struct pHidd_USBHID_GetHidDescriptor p;

    if (!static_mid) static_mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHID, moHidd_USBHID_GetHidDescriptor);

    p.mID = static_mid;

    return (usb_hid_descriptor_t *)OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBHID_SetIdle(OOP_Object *obj, uint8_t duration, uint8_t id)
{
    STATIC_MID;

    struct pHidd_USBHID_SetIdle p;

    if (!static_mid) static_mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHID, moHidd_USBHID_SetIdle);

    p.mID = static_mid;
    p.duration = duration;
    p.id = id;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBHID_SetProtocol(OOP_Object *obj, uint8_t protocol)
{
    STATIC_MID;

    struct pHidd_USBHID_SetProtocol p;

    if (!static_mid) static_mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHID, moHidd_USBHID_SetProtocol);

    p.mID = static_mid;
    p.protocol = protocol;

    return OOP_DoMethod(obj, &p.mID);
}

BOOL HIDD_USBHID_SetReport(OOP_Object *obj, uint8_t type, uint8_t id, void *report, uint16_t length)
{
    STATIC_MID;

    struct pHidd_USBHID_SetReport p;

    if (!static_mid) static_mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHID, moHidd_USBHID_SetReport);

    p.mID = static_mid;
    p.type = type;
    p.id = id;
    p.report = report;
    p.length = length;

    return OOP_DoMethod(obj, &p.mID);
}

void HIDD_USBHID_ParseReport(OOP_Object *obj, uint8_t id, void *report, uint32_t report_length)
{
    STATIC_MID;
    struct pHidd_USBHID_ParseReport p;

    if (!static_mid) static_mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBHID, moHidd_USBHID_ParseReport);

    p.mID = static_mid;
    p.id = id;
    p.report = report;
    p.report_length = report_length;

    OOP_DoMethod(obj, &p.mID);
}

