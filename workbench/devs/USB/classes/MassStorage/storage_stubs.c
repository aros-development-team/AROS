/*
    Copyright � 2004-2019, The AROS Development Team. All rights reserved.
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
#include <usb/mstorage.h>

#include <proto/oop.h>

#undef OOPBase
#define OOPBase (OOP_OOPBASE(o))

BOOL HIDD_USBStorage_Reset(OOP_Object *o)
{
	STATIC_MID;
	struct pHidd_USBStorage_Reset p;

	if (!static_mid) static_mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_Reset);

	p.mID = static_mid;

	return OOP_DoMethod(o, &p.mID);
}

uint8_t HIDD_USBStorage_GetMaxLUN(OOP_Object *o)
{
	STATIC_MID;
	struct pHidd_USBStorage_GetMaxLUN p;

	if (!static_mid) static_mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_GetMaxLUN);

	p.mID = static_mid;

	return OOP_DoMethod(o, &p.mID);
}

BOOL HIDD_USBStorage_TestUnitReady(OOP_Object *o, uint8_t lun)
{
	STATIC_MID;
	struct pHidd_USBStorage_TestUnitReady p;

	if (!static_mid) static_mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_TestUnitReady);

	p.mID = static_mid;
	p.lun = lun;

	return OOP_DoMethod(o, &p.mID);
}

uint32_t HIDD_USBStorage_DirectSCSI(OOP_Object *o, uint8_t lun, uint8_t *cmd, uint8_t cmdLen, void *data, uint32_t dataLen, uint8_t read)
{
	STATIC_MID;
	struct pHidd_USBStorage_DirectSCSI p;

	if (!static_mid) static_mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_DirectSCSI);

	p.mID = static_mid;
	p.lun = lun;
	p.cmd = cmd;
	p.cmdLen = cmdLen;
	p.data = data;
	p.dataLen = dataLen;
	p.read = read;

	return OOP_DoMethod(o, &p.mID);
}

BOOL HIDD_USBStorage_ReadCapacity(OOP_Object *o, uint8_t lun, uint32_t *blockTotal, uint32_t *blockSize)
{
	STATIC_MID;
	struct pHidd_USBStorage_ReadCapacity p;

	if (!static_mid) static_mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_ReadCapacity);

	p.mID = static_mid;
	p.lun = lun;
	p.blockTotal = blockTotal;
	p.blockSize = blockSize;

	return OOP_DoMethod(o, &p.mID);
}

BOOL HIDD_USBStorage_RequestSense(OOP_Object *o, uint8_t lun, void *buffer, uint32_t bufferLength)
{
	STATIC_MID;
	struct pHidd_USBStorage_RequestSense p;

	if (!static_mid) static_mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_RequestSense);

	p.mID = static_mid;
	p.lun = lun;
	p.buffer = buffer;
	p.bufferLength = bufferLength;

	return OOP_DoMethod(o, &p.mID);
}

BOOL HIDD_USBStorage_Inquiry(OOP_Object *o, uint8_t lun, void *buffer, uint32_t bufferLength)
{
	STATIC_MID;
	struct pHidd_USBStorage_Inquiry p;

	if (!static_mid) static_mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_Inquiry);

	p.mID = static_mid;
	p.lun = lun;
	p.buffer = buffer;
	p.bufferLength = bufferLength;

	return OOP_DoMethod(o, &p.mID);
}

BOOL HIDD_USBStorage_Read(OOP_Object *o, uint8_t lun, void *buffer, uint32_t lba, uint16_t count)
{
	STATIC_MID;
	struct pHidd_USBStorage_Read p;

	if (!static_mid) static_mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_Read);

	p.mID = static_mid;
	p.lun = lun;
	p.buffer = buffer;
	p.block= lba;
	p.count = count;

	return OOP_DoMethod(o, &p.mID);
}

BOOL HIDD_USBStorage_Write(OOP_Object *o, uint8_t lun, void *buffer, uint32_t lba, uint16_t count)
{
	STATIC_MID;
	struct pHidd_USBStorage_Write p;

	if (!static_mid) static_mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_Write);

	p.mID = static_mid;
	p.lun = lun;
	p.buffer = buffer;
	p.block= lba;
	p.count = count;

	return OOP_DoMethod(o, &p.mID);
}
