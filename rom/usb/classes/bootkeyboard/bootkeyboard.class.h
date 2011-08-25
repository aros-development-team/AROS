#ifndef BOOTKEYBOARD_CLASS_H
#define BOOTKEYBOARD_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                       Includes for Bootkeyboard class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <devices/usb_hid.h>

#include "bootkeyboard.h"

/* Protos */

struct NepClassHid * bootkbd_AttemptInterfaceBinding(struct NepHidBase *nh, struct PsdInterface *pif);
struct NepClassHid * bootkbd_ForceInterfaceBinding(struct NepHidBase *nh, struct PsdInterface *pif);
void bootkbd_ReleaseInterfaceBinding(struct NepHidBase *nh, struct NepClassHid *nch);

void nParseKeys(struct NepClassHid *nch, UBYTE *buf);

struct NepClassHid * bootkbd_AllocHid(void);
void bootkbd_FreeHid(struct NepClassHid *nch);

BOOL bootkbd_LoadClassConfig(struct NepHidBase *nh);
LONG nOpenCfgWindow(struct NepHidBase *nh);

void bootkbd_GUITaskCleanup(struct NepHidBase *nh);

AROS_UFP0(void, bootkbd_HidTask);
AROS_UFP0(void, bootkbd_GUITask);

#endif /* BOOTKEYBOARD_CLASS_H */
