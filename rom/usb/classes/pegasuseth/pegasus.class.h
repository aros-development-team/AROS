#ifndef PEGASUS_CLASS_H
#define PEGASUS_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for pegasus class
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

#include "pegasus.h"
#include "dev.h"

/* Protos */

struct NepClassEth * usbAttemptDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd);
struct NepClassEth * usbForceDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd);
void usbReleaseDeviceBinding(struct NepEthBase *nh, struct NepClassEth *ncp);

struct NepClassEth * nAllocEth(void);
void nFreeEth(struct NepClassEth *ncp);

LONG nReadPegReg(struct NepClassEth *ncp, ULONG reg);
LONG nReadPegRegs(struct NepClassEth *ncp, UBYTE *data, ULONG len, ULONG offset);
LONG nWritePegReg(struct NepClassEth *ncp, ULONG reg, ULONG value);
LONG nWritePegRegs(struct NepClassEth *ncp, UBYTE *data, ULONG len, ULONG offset);
BOOL nReadEEPROMMAC(struct NepClassEth *ncp, UBYTE *macptr);
LONG nReadPhyWord(struct NepClassEth *ncp, ULONG phyid, ULONG phyreg);
BOOL nWritePhyWord(struct NepClassEth *ncp, ULONG phyid, ULONG phyreg, ULONG value);

BOOL nInitPegasus(struct NepClassEth *ncp);
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

#endif /* PEGASUS_CLASS_H */
