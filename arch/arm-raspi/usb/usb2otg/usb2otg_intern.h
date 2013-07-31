#ifndef USB2OTG_INTERN_H
#define USB2OTG_INTERN_H
/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include LC_LIBDEFS_FILE

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <exec/resident.h>
#include <exec/initializers.h>
#include <dos/dos.h>

#include <devices/timer.h>
#include <utility/utility.h>

#include <devices/usbhardware.h>
#include <devices/newstyle.h>

#include <oop/oop.h>

/*
    Force the USB chipset to run in Host mode
    AFAIK Poseidon doesnt support device mode? - TODO
*/
//#define OTG_FORCEHOSTMODE
//#define OTG_FORCEDEVICEMODE

/* Reply the iorequest with success */
#define RC_OK	      0

/* Magic cookie, don't set error fields & don't reply the ioreq */
#define RC_DONTREPLY  -1

#define MAX_ROOT_PORTS	 16

struct USBNSDeviceQueryResult
{
    ULONG               DevQueryFormat;
    ULONG               SizeAvailable;
    UWORD               DeviceType;
    UWORD               DeviceSubType;
    const UWORD         *SupportedCommands;     /* 0 terminated list of cmd's   */
};

struct USB2OTGUnit
{
    struct Unit         hu_Unit;

    struct List		hu_IOPendingQueue;	/* Root Hub Pending IO Requests */

    struct List         hu_TDQueue;
    struct List         hu_PeriodicTDQueue;
    struct List         hu_CtrlXFerQueue;
    struct List         hu_IntXFerQueue;
    struct List         hu_IsoXFerQueue;
    struct List	        hu_BulkXFerQueue;

    struct List         hu_AbortQueue;

    APTR                hu_GlobalIRQHandle;
    struct Interrupt	hu_PendingInt;
    struct Interrupt    hu_NakTimeoutInt;
    struct timerequest  hu_NakTimeoutReq;
    struct MsgPort      hu_NakTimeoutMsgPort;

    UBYTE		hu_OperatingMode;       /* HOST/DEVICE mode */
    UBYTE		hu_HubAddr;
    UBYTE               hu_HostChans;
    UBYTE               hu_DevEPs;
    UBYTE               hu_DevInEPs;

    BOOL		hu_UnitAllocated;       /* unit opened */
    BOOL                hu_HubPortChanged;      /* Root port state change */
};

/* PRIVATE device node */
struct USB2OTGDevice
{
    struct Library	hd_Library;	        /* standard */
    UWORD		hd_Flags;	        /* various flags */

    APTR		hd_KernelBase;		/* kernel.resource base */
    APTR                hd_UtilityBase;	        /* for tags etc */

    APTR		hd_MemPool;	        /* memory pool */

    struct USB2OTGUnit  *hd_Unit;	        /* we only currently support a single unit.. */
    
    struct MsgPort	*hd_MsgPort;
    struct timerequest	*hd_TimerReq;	        /* Timer I/O Requests */

};

#define FNAME_DEV(x)            USB2OTG__Dev__ ## x
#define FNAME_ROOTHUB(x)        USB2OTG__RootHub__ ## x

#ifdef UtilityBase
#undef UtilityBase
#endif

#ifdef KernelBase
#undef KernelBase
#endif

#define	UtilityBase     USB2OTGBase->hd_UtilityBase

#define KernelBase      USB2OTGBase->hd_KernelBase

AROS_INTP(FNAME_DEV(PendingInt));
AROS_INTP(FNAME_DEV(NakTimeoutInt));

struct Unit             *FNAME_DEV(OpenUnit)(struct IOUsbHWReq *, LONG, struct USB2OTGDevice *);
void                    FNAME_DEV(CloseUnit)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);

void                    FNAME_DEV(TermIO)(struct IOUsbHWReq *, struct USB2OTGDevice *);

WORD                    FNAME_DEV(cmdNSDeviceQuery)(struct IOStdReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdQueryDevice)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);

WORD                    FNAME_DEV(cmdReset)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdFlush)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);

WORD                    FNAME_DEV(cmdUsbReset)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdUsbResume)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdUsbSuspend)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdUsbOper)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdControlXFer)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdBulkXFer)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdIntXFer)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_DEV(cmdIsoXFer)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);

void                    FNAME_DEV(Cause)(struct USB2OTGDevice *, struct Interrupt *);

WORD                    FNAME_ROOTHUB(cmdControlXFer)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
WORD                    FNAME_ROOTHUB(cmdIntXFer)(struct IOUsbHWReq *, struct USB2OTGUnit *, struct USB2OTGDevice *);
void                    FNAME_ROOTHUB(PendingIO)(struct USB2OTGUnit *);

#endif /* USB2OTG_INTERN_H */
