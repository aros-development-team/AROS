#ifndef XHCICHIP_H
#define XHCICHIP_H

/*
 *----------------------------------------------------------------------------
 *             Includes for XHCI USB Controllers
 *----------------------------------------------------------------------------
 *
 */

#include <exec/types.h>
#include "hccommon.h"

#define XHCI_SBRN      0x60    // (B) Serial Bus Release Number Register
#define XHCI_FLADJ     0x61    // (B) Frame Length Adjustment Register
#define XHCI_DBES      0x62    // (B) 4bit Default Best Effort Service Latency (DBESL) : 4bit Default Best Effort Service Latency Deep (DBESLD)

#endif /* XHCICHIP_H */
