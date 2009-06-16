#ifndef CAMDUSBMIDI_CLASS_H
#define CAMDUSBMIDI_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                       Includes for camdusbmidi class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <libraries/gadtools.h>

#include <devices/usb_audio.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "camdusbmidi.h"

/* Protos */

struct NepClassHid * usbAttemptInterfaceBinding(struct NepHidBase *nh, struct PsdInterface *pif);
struct NepClassHid * usbForceInterfaceBinding(struct NepHidBase *nh, struct PsdInterface *pif);
void usbReleaseInterfaceBinding(struct NepHidBase *nh, struct NepClassHid *nch);

void nParseMidi(struct NepClassHid *nch, UBYTE *buf, ULONG len);
void nParseMidiOut(struct NepClassHid *nch);

struct NepClassHid * nAllocHid(void);
void nFreeHid(struct NepClassHid *nch);

BOOL nLoadClassConfig(struct NepHidBase *nh);
LONG nOpenCfgWindow(struct NepHidBase *nh);

void nGUITaskCleanup(struct NepHidBase *nh);

AROS_UFP0(void, nHidTask);
AROS_UFP0(void, nGUITask);

#endif /* CAMDUSBMIDI_CLASS_H */
