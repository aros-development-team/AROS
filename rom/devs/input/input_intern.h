#ifndef INPUT_INTERN_H
#define INPUT_INTERN_H
/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.7  2000/05/28 21:33:52  stegerg
    handle IND_SETTHRESH and IND_SETPERIOD.
    implemented key repeat. Actually still treats
    all keys (except qualifier keys) as repeatable.

    Revision 1.6  2000/02/26 13:20:15  iaint
    Changed the stacksize to be at least AROS_STACKSIZE. This is very important - some of these were allocating stacks that were probably less than the amount required to perform signal processing in emulated systems.

    Revision 1.5  2000/01/22 20:29:31  stegerg
    added ActQualifier to inputbase struct.

    Revision 1.4  1999/10/20 19:36:24  stegerg
    When testing the Workbench background pattern once
    a deadend alert showed up saying something about
    stack overflow on input.device task so I increaed
    the input.device stack from 20000 to 25000 Bytes.

    Revision 1.3  1998/10/20 16:44:24  hkiel
    Amiga Research OS

    Revision 1.2  1998/04/11 19:34:52  nlorentz
    Added IND_WRITEEVENT and fixed bugs


    Desc: Private definitions for Input device.
    Lang:
*/

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

/* Structure passed to the input.device task when it's initialized */
struct IDTaskParams
{
    struct inputbase 	*InputDevice;
    struct Task		*Caller; /* Signal this task.. */
    ULONG		Signal; /* Using this sigs, that the ID task */
    				/* has been initialized and is ready to handle IO requests */
};

/* Prototypes */
VOID ProcessEvents(struct IDTaskParams *taskparams);
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
    struct MsgPort 	*CommandPort;
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

