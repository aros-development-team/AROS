#ifndef UHWCMD_H
#define UHWCMD_H

#include "debug.h"
#include "pci_aros.h"

#include "uhcichip.h"
#include "ohcichip.h"
#include "ehcichip.h"
#include "pciusb.h"

struct Unit *Open_Unit(struct IOUsbHWReq *ioreq,
                       LONG unitnr,
                       struct PCIDevice *base);
void Close_Unit(struct PCIDevice *base, struct PCIUnit *unit,
                struct IOUsbHWReq *ioreq);

void uhwDelayMS(ULONG milli, struct PCIUnit *unit, struct PCIDevice *base);

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

WORD cmdFlush(struct IOUsbHWReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);

WORD cmdNSDeviceQuery(struct IOStdReq *ioreq, struct PCIUnit *unit, struct PCIDevice *base);

BOOL cmdAbortIO(struct IOUsbHWReq *ioreq, struct PCIDevice *base);

void TermIO(struct IOUsbHWReq *ioreq, struct PCIDevice *base);

AROS_UFP1(void, uhwNakTimeoutInt,
          AROS_UFPA(struct PCIUnit *,  unit, A1));

BOOL pciInit(struct PCIDevice *hd);
void pciExpunge(struct PCIDevice *hd);
BOOL pciAllocUnit(struct PCIUnit *hu);
void pciFreeUnit(struct PCIUnit *hu);
APTR pciGetPhysical(struct PCIController *hc, APTR virtaddr);

void uhciCompleteInt(struct PCIController *hc);
void uhciIntCode(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw);

void uhciUpdateIntTree(struct PCIController *hc);
void uhciFreeQContext(struct PCIController *hc, struct UhciQH *uqh);
inline struct UhciQH * uhciAllocQH(struct PCIController *hc);
inline void uhciFreeQH(struct PCIController *hc, struct UhciQH *uqh);
inline struct UhciTD * uhciAllocTD(struct PCIController *hc);
inline void uhciFreeTD(struct PCIController *hc, struct UhciTD *utd);


void ohciCompleteInt(struct PCIController *hc);
void ohciIntCode(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw);

void ohciUpdateIntTree(struct PCIController *hc);
void ohciFreeEDContext(struct PCIController *hc, struct OhciED *oed);
inline struct OhciED * ohciAllocED(struct PCIController *hc);
inline void ohciFreeED(struct PCIController *hc, struct OhciED *oed);
inline struct OhciTD * ohciAllocTD(struct PCIController *hc);
inline void ohciFreeTD(struct PCIController *hc, struct OhciTD *otd);


void ehciCompleteInt(struct PCIController *hc);
void ehciIntCode(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw);

void ehciUpdateIntTree(struct PCIController *hc);
void ehciFreeAsyncContext(struct PCIController *hc, struct EhciQH *eqh);
void ehciFreePeriodicContext(struct PCIController *hc, struct EhciQH *eqh);
inline struct EhciQH * ehciAllocQH(struct PCIController *hc);
inline void ehciFreeQH(struct PCIController *hc, struct EhciQH *eqh);
inline struct EhciTD * ehciAllocTD(struct PCIController *hc);
inline void ehciFreeTD(struct PCIController *hc, struct EhciTD *etd);

struct my_NSDeviceQueryResult
{
    ULONG   DevQueryFormat;         /* this is type 0               */
    ULONG   SizeAvailable;          /* bytes available              */
    UWORD   DeviceType;             /* what the device does         */
    UWORD   DeviceSubType;          /* depends on the main type     */
    const UWORD *SupportedCommands; /* 0 terminated list of cmd's   */
};

#endif /* UHWCMD_H */

