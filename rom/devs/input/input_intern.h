/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1998/04/07 20:49:18  nlorentz
    Initial revision


    Desc: Private definitions for Input device.
    Lang:
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

/* Size of the input device's stack */
#define IDTASK_STACKSIZE 20000

/* Predeclaration */
struct inputbase;

/* Prototypes */
VOID ProcessEvents();

struct Interrupt *InitIIH(struct inputbase *InputDevice);
VOID CleanupIIH(struct Interrupt *handler, struct inputbase *InputDevice);
struct Task *CreateInputTask(ULONG stacksize, struct inputbase *InputDevice);



struct inputbase
{
    struct Device device;
    struct ExecBase * sysBase;
    BPTR seglist;
    
    /* The stuff below will never get deallocated, since
    ** input device is never removed, once it's initialized.
    */
    struct Task *InputTask;
    struct MsgPort *CommandPort;
    struct MinList HandlerList;
    struct SignalSemaphore HandlerSema;
    struct Interrupt *IntuiInputHandler;
};

#define expunge() \
__AROS_LC0(BPTR, expunge, struct inputbase *, InputDevice, 3, Input)

#ifdef SysBase
    #undef SysBase
#endif
#define SysBase InputDevice->sysBase

#endif /* INPUT_INTERN_H */

