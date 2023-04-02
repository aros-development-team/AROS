#ifndef UHWCMD_H
#define UHWCMD_H

#include "debug.h"
#include "pci_aros.h"
#include "pciusb.h"

/* Uncomment to enable the W.I.P Isochornous transfer stubs */
//#define PCIUSB_WIP_ISO

#if (__WORDSIZE == 64)

APTR usbGetBuffer(APTR data, ULONG len, UWORD dir);
void usbReleaseBuffer(APTR buffer, APTR data, ULONG len, UWORD dir);

#else

/* On 32-bit systems we don't need mirroring */

#define usbGetBuffer(data, len, dir) ({ (void)(len); (void)(dir); (data);})
#define usbReleaseBuffer(buffer, data, len, dir) do { (void)(buffer); (void)(data); (void)(len); (void)(dir); } while (0)

#endif

struct Unit *Open_Unit(struct IOUsbHWReq *ioreq, LONG unitnr, struct PCIDevice *base);
void Close_Unit(struct PCIDevice *base, struct PCIUnit *unit, struct IOUsbHWReq *ioreq);

void SureCause(struct PCIDevice *base, struct Interrupt *interrupt);
BOOL uhwOpenTimer(struct PCIUnit *unit, struct PCIDevice *base);
void uhwDelayMS(ULONG milli, struct PCIUnit *unit);
void uhwCheckSpecialCtrlTransfers(struct PCIController *hc, struct IOUsbHWReq *ioreq);
void uhwCheckRootHubChanges(struct PCIUnit *unit);

WORD cmdReset(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdUsbReset(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdUsbResume(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdUsbSuspend(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdUsbOper(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);

WORD cmdQueryDevice(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);

WORD cmdControlXFer(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdBulkXFer(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdIntXFer(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdIsoXFer(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);

#if defined(PCIUSB_WIP_ISO)
WORD cmdAddIsoHandler(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdRemIsoHandler(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdStartRTIso(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
WORD cmdStopRTIso(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);
#endif

WORD cmdFlush(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);

WORD cmdNSDeviceQuery(struct IOStdReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);

BOOL cmdAbortIO(struct IOUsbHWReq *ioreq, struct PCIDevice *base);

void TermIO(struct IOUsbHWReq *ioreq, struct PCIDevice *base);

AROS_INTP(uhwPeriodicInt);
AROS_INTP(uhwNakTimeoutInt);

BOOL pciInit(struct PCIDevice *hd);
void pciExpunge(struct PCIDevice *hd);
BOOL pciAllocUnit(struct PCIUnit *hu);
void pciFreeUnit(struct PCIUnit *hu);
APTR pciGetPhysical(struct PCIController *hc, APTR virtaddr);

UBYTE PCIXReadConfigByte(struct PCIController *hc, UBYTE offset);
UWORD PCIXReadConfigWord(struct PCIController *hc, UBYTE offset);
ULONG PCIXReadConfigLong(struct PCIController *hc, UBYTE offset);
void PCIXWriteConfigByte(struct PCIController *hc, ULONG offset, UBYTE value);
void PCIXWriteConfigWord(struct PCIController *hc, ULONG offset, UWORD value);
void PCIXWriteConfigLong(struct PCIController *hc, ULONG offset, ULONG value);
BOOL PCIXAddInterrupt(struct PCIController *hc, struct Interrupt *interrupt);

void uhwDelayMicro(ULONG micro, struct PCIUnit *unit);

struct my_NSDeviceQueryResult
{
    ULONG   DevQueryFormat;         /* this is type 0               */
    ULONG   SizeAvailable;          /* bytes available              */
    UWORD   DeviceType;             /* what the device does         */
    UWORD   DeviceSubType;          /* depends on the main type     */
    const UWORD *SupportedCommands; /* 0 terminated list of cmd's   */
};

#endif /* UHWCMD_H */

