/*
    Copyright � 2004, The AROS Development Team. All rights reserved.
    $Id: hid_stubs.c 28727 2008-05-17 22:02:30Z schulz $

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

#include <hidd/hidd.h>
#include <usb/usb.h>
#include <usb/storage.h>

#include <proto/oop.h>

#undef OOPBase
#define OOPBase (OOP_OOPBASE(o))

#ifdef AROS_CREATE_ROM
#define STATIC_MID  OOP_MethodID mid = 0
#else
#define STATIC_MID  static OOP_MethodID mid
#endif

BOOL HIDD_USBStorage_Reset(OOP_Object *o)
{
	STATIC_MID;
	struct pHidd_USBStorage_Reset p;

	if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_Reset);

	p.mID = mid;

	return OOP_DoMethod(o, &p.mID);
}

uint8_t HIDD_USBStorage_GetMaxLUN(OOP_Object *o)
{
	STATIC_MID;
	struct pHidd_USBStorage_GetMaxLUN p;

	if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_GetMaxLUN);

	p.mID = mid;

	return OOP_DoMethod(o, &p.mID);
}

BOOL HIDD_USBStorage_TestUnitReady(OOP_Object *o, uint8_t lun)
{
	STATIC_MID;
	struct pHidd_USBStorage_TestUnitReady p;

	if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_TestUnitReady);

	p.mID = mid;
	p.lun = lun;

	return OOP_DoMethod(o, &p.mID);
}

uint32_t HIDD_USBStorage_DirectSCSI(OOP_Object *o, uint8_t lun, uint8_t *cmd, uint8_t cmdLen, void *data, uint32_t dataLen, uint8_t read)
{
	STATIC_MID;
	struct pHidd_USBStorage_DirectSCSI p;

	if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_DirectSCSI);

	p.mID = mid;
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

	if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_ReadCapacity);

	p.mID = mid;
	p.lun = lun;
	p.blockTotal = blockTotal;
	p.blockSize = blockSize;

	return OOP_DoMethod(o, &p.mID);
}

BOOL HIDD_USBStorage_RequestSense(OOP_Object *o, uint8_t lun, void *buffer, uint32_t bufferLength)
{
	STATIC_MID;
	struct pHidd_USBStorage_RequestSense p;

	if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_RequestSense);

	p.mID = mid;
	p.lun = lun;
	p.buffer = buffer;
	p.bufferLength = bufferLength;

	return OOP_DoMethod(o, &p.mID);
}

BOOL HIDD_USBStorage_Inquiry(OOP_Object *o, uint8_t lun, void *buffer, uint32_t bufferLength)
{
	STATIC_MID;
	struct pHidd_USBStorage_Inquiry p;

	if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_Inquiry);

	p.mID = mid;
	p.lun = lun;
	p.buffer = buffer;
	p.bufferLength = bufferLength;

	return OOP_DoMethod(o, &p.mID);
}

BOOL HIDD_USBStorage_Read(OOP_Object *o, uint8_t lun, void *buffer, uint32_t lba, uint16_t count)
{
	STATIC_MID;
	struct pHidd_USBStorage_Read p;

	if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_Read);

	p.mID = mid;
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

	if (!mid) mid = OOP_GetMethodID((STRPTR)IID_Hidd_USBStorage, moHidd_USBStorage_Write);

	p.mID = mid;
	p.lun = lun;
	p.buffer = buffer;
	p.block= lba;
	p.count = count;

	return OOP_DoMethod(o, &p.mID);
}
