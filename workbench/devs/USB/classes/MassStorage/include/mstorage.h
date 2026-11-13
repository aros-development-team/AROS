#ifndef USB_STORAGE_H
#define USB_STORAGE_H

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
#include <oop/oop.h>
#include <usb/usb.h>

#define CLID_Hidd_USBStorage "Bus::USB::MassStorage"
#define IID_Hidd_USBStorage  "IBus::USB::MassStorage"

#define HiddUSBStorageAttrBase __IHidd_USBStorage

enum {
	moHidd_USBStorage_GetMaxLUN,
	moHidd_USBStorage_Reset,
	moHidd_USBStorage_DirectSCSI,

	moHidd_USBStorage_TestUnitReady,
	moHidd_USBStorage_RequestSense,
	moHidd_USBStorage_Read,
	moHidd_USBStorage_Write,
	moHidd_USBStorage_ReadCapacity,
	moHidd_USBStorage_Inquiry,

    NUM_HIDD_USBStorage_METHODS
};

struct pHidd_USBStorage_GetMaxLUN {
	OOP_MethodID	mID;
};

struct pHidd_USBStorage_Reset {
	OOP_MethodID	mID;
};

struct pHidd_USBStorage_DirectSCSI {
	OOP_MethodID	mID;
	uint8_t			lun;
	uint8_t			*cmd;
	uint8_t			cmdLen;
	void			*data;
	uint32_t		dataLen;
	uint8_t			read;
};

struct pHidd_USBStorage_TestUnitReady {
	OOP_MethodID	mID;
	uint8_t			lun;
};

struct pHidd_USBStorage_RequestSense {
	OOP_MethodID	mID;
	uint8_t			lun;
	void 			*buffer;
	uint32_t		bufferLength;
};

struct pHidd_USBStorage_Read {
	OOP_MethodID	mID;
	uint8_t			lun;
	void 			*buffer;
	uint32_t		block;
	uint16_t		count;
};

struct pHidd_USBStorage_Write {
	OOP_MethodID	mID;
	uint8_t			lun;
	void 			*buffer;
	uint32_t		block;
	uint16_t		count;
};

struct pHidd_USBStorage_ReadCapacity {
	OOP_MethodID	mID;
	uint8_t			lun;
	uint32_t		*blockTotal;
	uint32_t		*blockSize;
};

struct pHidd_USBStorage_Inquiry {
	OOP_MethodID	mID;
	uint8_t			lun;
	void 			*buffer;
	uint32_t		bufferLength;
};

BOOL HIDD_USBStorage_Reset(OOP_Object *o);
uint8_t HIDD_USBStorage_GetMaxLUN(OOP_Object *o);
uint32_t HIDD_USBStorage_DirectSCSI(OOP_Object *o, uint8_t lun, uint8_t *cmd, uint8_t cmdLen, void *data, uint32_t dataLen, uint8_t read);
BOOL HIDD_USBStorage_TestUnitReady(OOP_Object *o, uint8_t lun);
BOOL HIDD_USBStorage_RequestSense(OOP_Object *o, uint8_t lun, void *buffer, uint32_t bufferLength);
BOOL HIDD_USBStorage_Read(OOP_Object *o, uint8_t lun, void *buffer, uint32_t lba, uint16_t count);
BOOL HIDD_USBStorage_Write(OOP_Object *o, uint8_t lun, void *buffer, uint32_t lba, uint16_t count);
BOOL HIDD_USBStorage_ReadCapacity(OOP_Object *o, uint8_t lun, uint32_t *blockTotal, uint32_t *blockSize);
BOOL HIDD_USBStorage_Inquiry(OOP_Object *o, uint8_t lun, void *buffer, uint32_t bufferLength);

#endif /* USB_STORAGE_H */
