#ifndef STORAGE_H_
#define STORAGE_H_

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
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <dos/bptr.h>
#include <dos/dosextens.h>

#include <proto/exec.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <oop/oop.h>

#include <aros/arossupportbase.h>
#include <exec/execbase.h>

#include LC_LIBDEFS_FILE

#undef HiddPCIDeviceAttrBase
#undef HiddUSBDeviceAttrBase
#undef HiddUSBHubAttrBase
#undef HiddUSBDrvAttrBase
#undef HiddUSBAttrBase
#undef HiddUSBStorageAttrBase
#undef HiddAttrBase

#define HiddPCIDeviceAttrBase (SD(cl)->HiddPCIDeviceAB)
#define HiddUSBDeviceAttrBase (SD(cl)->HiddUSBDeviceAB)
#define HiddUSBHubAttrBase (SD(cl)->HiddUSBHubAB)
#define HiddUSBDrvAttrBase (SD(cl)->HiddUSBDrvAB)
#define HiddAttrBase (SD(cl)->HiddAB)
#define HiddUSBAttrBase (SD(cl)->HiddUSBAB)
#define HiddUSBStorageAttrBase (SD(cl)->HiddUSBStorageAB)

typedef struct __attribute__((packed)) {
	uint32_t	dCBWSignature;
	uint32_t	dCBWTag;
	uint32_t	dCBWDataTransferLength;
	uint8_t		bmCBWFlags;
	uint8_t		bCBWLUN;
	uint8_t		bCBWCBLength;
	uint8_t		CBWCB[16];
} cbw_t;

#define CBW_SIGNATURE	0x43425355
#define CBW_FLAGS_IN	0x80
#define CBW_FLAGS_OUT	0x00

typedef struct __attribute__((packed)) {
	uint32_t	dCSWSignature;
	uint32_t	dCSWTag;
	uint32_t	dCSWDataResidue;
	uint8_t		bCSWStatus;
} csw_t;

#define CSW_SIGNATURE		0x53425355
#define CSW_STATUS_OK		0x00
#define CSW_STATUS_FAIL		0x01
#define CSW_STATUS_PHASE	0x02

typedef struct {
	struct Unit		msu_unit;
	uint32_t		msu_unitNum;
	uint32_t		msu_blockSize;
	uint32_t		msu_blockCount;
	uint32_t		msu_changeNum;
	uint8_t			msu_blockShift;
	uint8_t			msu_flags;
	uint8_t			msu_lun;
	OOP_Class		*msu_class;
	OOP_Object		*msu_object;
	struct Interrupt *msu_removeInt;
	struct List		msu_diskChangeList;
	struct Task		*msu_handler;
	uint8_t			msu_inquiry[8];
} mss_unit_t;

struct mss_staticdata
{
    struct SignalSemaphore  Lock;
    void                    *MemPool;
    OOP_Class               *mssClass;

	struct MinList			unitList;
	struct MinList			unitCache;

    uint32_t				tid;
    uint32_t				unitNum;

    OOP_AttrBase            HiddPCIDeviceAB;
    OOP_AttrBase            HiddUSBDeviceAB;
    OOP_AttrBase            HiddUSBHubAB;
    OOP_AttrBase            HiddUSBDrvAB;
    OOP_AttrBase            HiddUSBAB;
    OOP_AttrBase            HiddUSBStorageAB;
    OOP_AttrBase            HiddAB;
};

typedef struct {
	struct MinNode	node;
	struct List		units;

	uint16_t		productID;
	uint16_t		vendorID;
	char			*productName;
	char			*manufacturerName;
	char			*serialNumber;

	uint32_t		unitNumber;
} unit_cache_t;

typedef struct MSSData {
	struct SignalSemaphore  	lock;
	unit_cache_t				*cache;

	struct mss_staticdata		*sd;
	OOP_Object					*o;

	usb_config_descriptor_t		*cdesc;
	usb_device_descriptor_t 	ddesc;
	usb_interface_descriptor_t	*iface;

	usb_endpoint_descriptor_t 	*ep_in;
	usb_endpoint_descriptor_t	*ep_out;

	void						*pipe_in;
	void						*pipe_out;
	struct Task					*handler[16];
	mss_unit_t					*unit[16];

	uint32_t					blocksize[16];
	uint8_t						maxLUN;
	uint32_t					unitNum;
} StorageData;




struct mssbase
{
    struct Library          LibNode;
    struct mss_staticdata   sd;
};

#define METHOD(base, id, name) \
    base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib)((struct mssbase*)(lib))
#define SD(cl) (&BASE(cl->UserData)->sd)

#define IOStdReq(io) ((struct IOStdReq *)io)


static inline volatile uint32_t getTID(struct mss_staticdata *sd)
{
	uint32_t id;
	Disable(); id = sd->tid++; Enable();
	return id;
}

/* exec device interface */

typedef struct {
	struct Device			mss_device;
	struct mss_staticdata	*mss_static;
} mss_device_t;


#define MSF_DiskChanged		1
#define MSF_DiskPresent		2
#define MSF_DeviceRemoved	4

extern void StorageTask(OOP_Class *cl, OOP_Object *o, uint32_t unitnum, struct Task *parent);
extern void HandleIO(struct IORequest *io, mss_device_t *device, mss_unit_t *unit);

#endif /* STORAGE_H_ */
