/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal definitions for the input.device
    Lang: english
*/

#ifndef INPUT_INTERN_H
#define INPUT_INTERN_H

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef EXEC_DEVICES_H
#   include <exec/devices.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef DEVICES_INPUTEVENT_H
#   include <devices/inputevent.h>
#endif
#ifndef DEVICES_TIMER_H
#   include <devices/timer.h>
#endif

/* Size of the input device's stack */
#define IDTASK_STACKSIZE (AROS_STACKSIZE + 10240)
/* Priority of the input.device task */
#define IDTASK_PRIORITY 20

/* Default key repeat threshold/interval in 1/50 secs */

#define DEFAULT_KEY_REPEAT_THRESHOLD 25
#define DEFAULT_KEY_REPEAT_INTERVAL  2

/* Predeclaration */
struct inputbase;
#if 0
/* Structure passed to the input.device task when it's initialized */
struct IDTaskParams
{
    struct inputbase 	*InputDevice;
    struct Task		*Caller; /* Signal this task.. */
    ULONG		Signal; /* Using this sigs, that the ID task */
    				/* has been initialized and is ready to handle IO requests */
};
#endif

/* Prototypes */
VOID ProcessEvents(struct inputbase *InputDevice);
struct Task *CreateInputTask(APTR taskparams, struct inputbase *InputDevice);
VOID AddEQTail(struct InputEvent *ie, struct inputbase *InputDevice);
struct InputEvent *GetEventsFromQueue(struct inputbase *InputDevice);
BOOL IsQualifierKey(UWORD key);
BOOL IsRepeatableKey(UWORD key);

struct inputbase
{
    struct Device device;
    struct ExecBase * sysBase;
    BPTR seglist;
    
    /* The stuff below will never get deallocated, since
    ** input device is never removed, once it's initialized.
    */
    struct Task 	*InputTask;
    struct MsgPort 	CommandPort;
    struct MinList 	HandlerList;
    struct InputEvent 	*EventQueueHead;
    struct InputEvent 	*EventQueueTail;
    struct timeval	KeyRepeatThreshold;
    struct timeval	KeyRepeatInterval;
    
    UWORD ActQualifier;
};

#define expunge() \
__AROS_LC0(BPTR, expunge, struct inputbase *, InputDevice, 3, Input)

#ifdef SysBase
    #undef SysBase
#endif
#define SysBase InputDevice->sysBase

#endif /* INPUT_INTERN_H */

