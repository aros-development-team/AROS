#ifndef AROSXCONTROLLER_H
#define AROSXCONTROLLER_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: controller.hidd devices for the XInput gamepad class.

    Each bound pad is registered as a device of a CLID_Hidd_Controller
    subclass owned by this class, with the fixed control, output and binding
    tables from arosxctrl_map.c. Input messages are pushed as raw reports;
    rumble and the player LED are driven through the pad's interrupt OUT
    pipe from the pad's own task.
*/

#include "arosx.h"
#include "arosxctrl_map.h"

struct AROSXCtrl
{
    APTR                            Device;         /* controller.hidd device object */
    struct pHidd_Controller_RawReport Raw;
    UWORD                           RumbleLow;      /* pending output, task context sends it */
    UWORD                           RumbleHigh;
    WORD                            Player;
    BOOL                            RumblePending;
    BOOL                            LEDPending;
};

BOOL arosxCtrlInit(struct AROSXClassBase *arosxb);
void arosxCtrlExit(struct AROSXClassBase *arosxb);
BOOL arosxCtrlDeferBinding(struct AROSXClassBase *arosxb);
void arosxCtrlDOSAvailable(struct AROSXClassBase *arosxb);
void arosxCtrlAttach(struct AROSXClassController *arosxc);
void arosxCtrlDetach(struct AROSXClassController *arosxc);
void arosxCtrlHandleReport(struct AROSXClassController *arosxc, UBYTE *buf, ULONG len);
void arosxCtrlFlushOutput(struct AROSXClassController *arosxc);

#endif /* AROSXCONTROLLER_H */
