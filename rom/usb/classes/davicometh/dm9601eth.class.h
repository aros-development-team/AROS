#ifndef DM9601ETH_CLASS_H
#define DM9601ETH_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for dm9601eth class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <libraries/gadtools.h>

#include <devices/newstyle.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "dm9601eth.h"
#include "dev.h"

/* Protos */

struct NepClassEth * usbAttemptDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd);
struct NepClassEth * usbForceDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd);
void usbReleaseDeviceBinding(struct NepEthBase *nh, struct NepClassEth *ncp);

struct NepClassEth * nAllocEth(void);
void nFreeEth(struct NepClassEth *ncp);

LONG nReadDMReg(struct NepClassEth *ncp, ULONG reg);
LONG nReadDMRegs(struct NepClassEth *ncp, UBYTE *data, ULONG len, ULONG offset);
LONG nWriteDMReg(struct NepClassEth *ncp, ULONG reg, ULONG value);
LONG nWriteDMRegs(struct NepClassEth *ncp, UBYTE *data, ULONG len, ULONG offset);
BOOL nReadEEPROMMAC(struct NepClassEth *ncp, UBYTE *macptr);
LONG nReadPhyWord(struct NepClassEth *ncp, ULONG phyreg);
BOOL nWritePhyWord(struct NepClassEth *ncp, ULONG phyreg, ULONG value);

BOOL nInitDavicom(struct NepClassEth *ncp);
void nSetOnline(struct NepClassEth *ncp);

void nDoEvent(struct NepClassEth *ncp, ULONG events);
BOOL nWritePacket(struct NepClassEth *ncp, struct IOSana2Req *ioreq);
BOOL nReadPacket(struct NepClassEth *ncp, UBYTE *pktptr, ULONG len);

BOOL nLoadClassConfig(struct NepEthBase *nh);
BOOL nLoadBindingConfig(struct NepClassEth *ncp);
LONG nOpenBindingCfgWindow(struct NepEthBase *nh, struct NepClassEth *ncp);

void nGUITaskCleanup(struct NepClassEth *nh);

AROS_UFP0(void, nEthTask);
AROS_UFP0(void, nGUITask);

#endif /* DM9601ETH_CLASS_H */
