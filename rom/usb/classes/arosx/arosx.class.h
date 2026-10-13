#ifndef AROSX_CLASS_H
#define AROSX_CLASS_H

#include "common.h"

#include "arosx.h"

static const STRPTR libname = MOD_NAME_STRING;

//struct AROSXClassController * usbAttemptInterfaceBinding(struct AROSXClassBase *nh, struct PsdInterface *pif);
//void usbReleaseInterfaceBinding(struct AROSXClassBase *nh, struct AROSXClassController *arosxc);

BOOL Gamepad_ParseMsg(struct AROSXClassController *arosxc, UBYTE *buf, ULONG len);

/* connect/disconnect/input notifications for the class' own GUI (formerly arosx.library) */
BOOL AROSXClass_SendEvent(struct AROSXClassBase *arosxb, ULONG ehmt, APTR param1, APTR param2);
struct AROSX_EventHook *AROSXClass_AddEventHandler(struct AROSXClassBase *arosxb, struct MsgPort *mp, ULONG msgmask);
void AROSXClass_RemEventHandler(struct AROSXClassBase *arosxb, struct AROSX_EventHook *eh);

struct AROSXClassController * nAllocHid(void);
void nFreeHid(struct AROSXClassController *arosxc);

LONG nOpenCfgWindow(struct AROSXClassController *arosxc);

void nGUITaskCleanup(struct AROSXClassController *arosxc);

AROS_UFP0(void, nHidTask);
AROS_UFP0(void, nGUITask);

void nDebugMem(struct Library *ps, UBYTE *rptr, ULONG rptlen);

#endif /* AROSX_CLASS_H */
