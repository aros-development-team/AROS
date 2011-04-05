/*
 * $Id$
 */

#ifndef RNDIS_CLASS_H
#define RNDIS_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for rndis class
 *----------------------------------------------------------------------------
 */

#include "common.h"

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <libraries/gadtools.h>

#include <devices/newstyle.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "if_urndisreg.h"
#include "rndis.h"
#include "dev.h"

/* Protos */

struct NepClassEth * usbAttemptDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd);
struct NepClassEth * usbForceDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd);
void usbReleaseDeviceBinding(struct NepEthBase *nh, struct NepClassEth *ncp);

struct NepClassEth * nAllocEth(void);
void nFreeEth(struct NepClassEth *ncp);

void nSetOnline(struct NepClassEth *ncp);

void nDoEvent(struct NepClassEth *ncp, ULONG events);
BOOL nWritePacket(struct NepClassEth *ncp, struct IOSana2Req *ioreq);
BOOL nReadPacket(struct NepClassEth *ncp, UBYTE *pktptr, ULONG len);

BOOL nLoadClassConfig(struct NepEthBase *nh);
BOOL nLoadBindingConfig(struct NepClassEth *ncp);
LONG nOpenBindingCfgWindow(struct NepEthBase *nh, struct NepClassEth *ncp);

void nGUITaskCleanup(struct NepClassEth *nh);

uint32_t urndis_ctrl_init(struct NepClassEth *ncp);
uint32_t urndis_ctrl_handle(struct NepClassEth *ncp, struct urndis_comp_hdr *hdr,void **buf, size_t *bufsz);
void urndis_attach(struct NepClassEth *ncp);
long urndis_encap(struct NepClassEth *ncp, BYTE *m,LONG len );
void urndis_decap(struct NepClassEth *ncp, BYTE **buf, LONG *len);

AROS_UFP0(void, nEthTask);
AROS_UFP0(void, nGUITask);

#endif /* RNDIS_CLASS_H */
