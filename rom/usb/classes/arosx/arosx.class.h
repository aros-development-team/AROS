#ifndef AROSX_CLASS_H
#define AROSX_CLASS_H

#include "common.h"

#include <devices/keyboard.h>
#include <libraries/gadtools.h>

#include <devices/usb_audio.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "arosx.h"

static const STRPTR libname = MOD_NAME_STRING;

//struct AROSXClassController * usbAttemptInterfaceBinding(struct AROSXClassBase *nh, struct PsdInterface *pif);
//void usbReleaseInterfaceBinding(struct AROSXClassBase *nh, struct AROSXClassController *nch);

void nParseMsg(struct AROSXClassController *nch, UBYTE *buf, ULONG len);

struct AROSXClassController * nAllocHid(void);
void nFreeHid(struct AROSXClassController *nch);

LONG nOpenCfgWindow(struct AROSXClassController *nch);

void nGUITaskCleanup(struct AROSXClassController *nch);

AROS_UFP0(void, nHidTask);
AROS_UFP0(void, nGUITask);

void nDebugMem(struct Library *ps, UBYTE *rptr, ULONG rptlen);

#endif /* AROSX_CLASS_H */
