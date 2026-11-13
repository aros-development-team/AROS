#ifndef UHWCMD_H
#define UHWCMD_H

#include "debug.h"
#include "denebusb.h"

#if !defined(__AROS__)
#include "denebusb.device_VERSION.h"
#endif

struct Unit *Open_Unit(struct IOUsbHWReq *ioreq,
                       LONG unitnr,
                       struct DenebDevice *base);
void Close_Unit(struct DenebDevice *base, struct DenebUnit *unit,
                struct IOUsbHWReq *ioreq);

WORD cmdReset(struct IOUsbHWReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);
WORD cmdUsbReset(struct IOUsbHWReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);
WORD cmdUsbResume(struct IOUsbHWReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);
WORD cmdUsbSuspend(struct IOUsbHWReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);
WORD cmdUsbOper(struct IOUsbHWReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);

WORD cmdQueryDevice(struct IOUsbHWReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);

WORD cmdControlXFer(struct IOUsbHWReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);
WORD cmdBulkXFer(struct IOUsbHWReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);
WORD cmdIntXFer(struct IOUsbHWReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);
WORD cmdIsoXFer(struct IOUsbHWReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);

WORD cmdAddIsoHandler(struct IOUsbHWReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);
WORD cmdRemIsoHandler(struct IOUsbHWReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);
WORD cmdStartRTIso(struct IOUsbHWReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);
WORD cmdStopRTIso(struct IOUsbHWReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);

WORD cmdFlush(struct IOUsbHWReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);

WORD cmdNSDeviceQuery(struct IOStdReq *ioreq, struct DenebUnit *unit, struct DenebDevice *base);

BOOL cmdAbortIO(struct IOUsbHWReq *ioreq,
                struct DenebDevice *base);
void TermIO(struct IOUsbHWReq *ioreq, struct DenebDevice *base);
void FreeATL(struct PTDNode *ptd, struct DenebUnit *unit);
void FreeInt(struct PTDNode *ptd, struct DenebUnit *unit);
void FreeIso(struct PTDNode *ptd, struct DenebUnit *unit);

#if !defined(__AROS__)
void DECLFUNC_1(uhwSoftInt, a1, struct DenebUnit *, unit);
void DECLFUNC_1(uhwNakTimeoutInt, a1, struct DenebUnit *, unit);
ULONG DECLFUNC_1(uhwLevel6Int, a1, struct DenebUnit *, unit);
void DECLFUNC_0(uhwDMATask);
#else
AROS_INTP(uhwSoftInt);
AROS_INTP(uhwNakTimeoutInt);
AROS_INTP(uhwLevel6Int);
void uhwDMATask();
#endif

#ifndef  __MORPHOS__

struct my_NSDeviceQueryResult
{
    ULONG   DevQueryFormat;         /* this is type 0               */
    ULONG   SizeAvailable;          /* bytes available              */
    UWORD   DeviceType;             /* what the device does         */
    UWORD   DeviceSubType;          /* depends on the main type     */
    const UWORD *SupportedCommands; /* 0 terminated list of cmd's   */
};

#else /* __MORPHOS__ */

struct my_NSDeviceQueryResult
{
    ULONG   DevQueryFormat;
    ULONG   SizeAvailable;
    UWORD   DeviceType;
    UWORD   DeviceSubType;
    const UWORD *SupportedCommands;
} __attribute__((packed));

#endif /* __MORPHOS__ */

#endif /* UHWCMD_H */

