/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Task used for wainting on events from linux
    Lang: English.
*/

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/arossupport.h>

#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/lists.h>

#include <oop/oop.h>
#include <hidd/mouse.h>
#include <hidd/unixio.h>

#define DEBUG 0
#include <aros/debug.h>

#include "linuxinput_intern.h"

#include <linux/input.h>

#define INPUTTASK_PRIORITY      50

#define INPUTTASK_STACKSIZE     (AROS_STACKSIZE)
#define INPUTTASK_NAME          "LinuxInput event task"

struct inputtask_params
{
    struct Task *creator;
    ULONG ok_signal;
    ULONG nok_signal;
    struct EventHandler * eh;
};

#define BUFF_SIZE   (64 * sizeof(struct input_event))

#define vHidd_Mouse_NoMotion (23839)

static VOID HIDD_LinuxInput_HandleMouseEvent(struct EventHandler * eh, struct pHidd_Mouse_Event *mouse_event)
{
    if ((eh->capabilities & CAP_MOUSE) && eh->mousehidd)
        HIDD_LinuxMouse_HandleEvent((OOP_Object *)eh->mousehidd, mouse_event);
}

static VOID HIDD_LinuxInput_HandleKbdEvent(struct EventHandler * eh, UBYTE scanCode)
{
    if ((eh->capabilities & CAP_KEYBOARD) && eh->kbdhidd)
        HIDD_LinuxKbd_HandleEvent((OOP_Object *)eh->kbdhidd, scanCode);
}

static VOID process_input_event(struct input_event * event, struct EventHandler * eh)
{

    if (event->type == EV_REL)
    {
        struct pHidd_Mouse_Event mouse_event;
        mouse_event.button = vHidd_Mouse_NoButton;
        mouse_event.type = vHidd_Mouse_NoMotion;

        if (event->code == REL_X)
        {
            mouse_event.type = vHidd_Mouse_Motion;
            mouse_event.x = event->value;
            mouse_event.y = 0;
        }

        if (event->code == REL_Y)
        {
            mouse_event.type = vHidd_Mouse_Motion;
            mouse_event.x = 0;
            mouse_event.y = event->value;
        }

        if (event->code == REL_WHEEL)
        {
            mouse_event.type = vHidd_Mouse_WheelMotion;
            mouse_event.x = 0;
            mouse_event.y = -event->value;
        }

        if (mouse_event.type != vHidd_Mouse_NoMotion)
            HIDD_LinuxInput_HandleMouseEvent(eh, &mouse_event);
    }

    if (event->type == EV_KEY )
    {
        struct pHidd_Mouse_Event mouse_event;
        mouse_event.button = vHidd_Mouse_NoButton;
        switch(event->code)
        {
        case(BTN_LEFT): mouse_event.button = vHidd_Mouse_Button1; break;
        case(BTN_RIGHT): mouse_event.button = vHidd_Mouse_Button2; break;
        case(BTN_MIDDLE): mouse_event.button = vHidd_Mouse_Button3; break;
        }

        if (mouse_event.button != vHidd_Mouse_NoButton)
        {
            mouse_event.type = event->value == 1 ? vHidd_Mouse_Press : vHidd_Mouse_Release;
            HIDD_LinuxInput_HandleMouseEvent(eh, &mouse_event);
        }

        if ((event->code >= KEY_RESERVED) && (event->code <= KEY_COMPOSE))
        {
            UBYTE scanCode = (UBYTE)event->code;
            if (event->value == 0) scanCode |= 0x80;
            HIDD_LinuxInput_HandleKbdEvent(eh, scanCode);
        }
    }
}

static VOID inputtask_entry()
{
    struct EventHandler * eh;
    struct inputtask_params itp;
    BYTE * buff = NULL;
    int ioerr;
    LONG bytesread = 0, items = 0, i;
    BYTE * ptr = NULL;

    /* We must copy the parameter struct because they are allocated
     on the parent's stack */

    D(bug("INSIDE INPUT TASK\n"));
    itp = *((struct inputtask_params *)FindTask(NULL)->tc_UserData);
    eh = itp.eh;
    D(bug("in inputtask: lsd = %p\n", lsd));
    D(bug("FDS: %d\n", lsd->mousedev));

    buff = AllocMem(BUFF_SIZE, MEMF_PUBLIC);
    if (!buff)
        goto failexit;

    Signal(itp.creator, itp.ok_signal);

    for (;;)
    {
        /* TODO: wait for SIGBREAKF_CTRL_C and then signal "caller" task */

        Hidd_UnixIO_Wait(eh->unixio, eh->eventdev, vHidd_UnixIO_Read);
        bytesread = Hidd_UnixIO_ReadFile(eh->unixio, eh->eventdev, buff, BUFF_SIZE, &ioerr);
        items = bytesread / sizeof(struct input_event);

        ptr = buff;

        for (i = 0; i < items; i++)
        {
            process_input_event((struct input_event *)ptr, eh);
            ptr += sizeof(struct input_event);
        }
    }

    FreeMem(buff, BUFF_SIZE);

    return;

failexit:

    if (buff)
        FreeMem(buff, BUFF_SIZE);

    Signal(itp.creator, itp.nok_signal);

    return;
    
}

static struct Task *create_inputtask( struct inputtask_params *params)
{
    struct Task *task;
    APTR stack;
    
    task = AllocMem(sizeof (struct Task), MEMF_PUBLIC|MEMF_CLEAR);
    if (NULL != task)
    {
        NEWLIST(&task->tc_MemEntry);
        task->tc_Node.ln_Type=NT_TASK;
        task->tc_Node.ln_Name= INPUTTASK_NAME;
        task->tc_Node.ln_Pri = INPUTTASK_PRIORITY;

        stack=AllocMem(INPUTTASK_STACKSIZE, MEMF_PUBLIC);
        if(NULL != stack)
        {
            task->tc_SPLower=stack;
            task->tc_SPUpper=(BYTE *)stack + INPUTTASK_STACKSIZE;
            task->tc_UserData=params;

#if AROS_STACK_GROWS_DOWNWARDS
            task->tc_SPReg=(BYTE *)task->tc_SPUpper-SP_OFFSET-sizeof(APTR);
#else
            task->tc_SPReg=(BYTE *)task->tc_SPLower-SP_OFFSET+sizeof(APTR);
#endif
            /* You have to clear signals first. */
            SetSignal(0, params->ok_signal | params->nok_signal);

            if(AddTask(task, inputtask_entry, NULL) != NULL)
            {
                /* Everything went OK. Wait for task to initialize */
                ULONG sigset;

                D(bug("WAITING FOR SIGNAL\n"));

                sigset = Wait( params->ok_signal | params->nok_signal );
                D(bug("GOT SIGNAL\n"));
                if (sigset & params->ok_signal)
                    return task;
            }

            FreeMem(stack, INPUTTASK_STACKSIZE);
        }
        
        FreeMem(task,sizeof(struct Task));
    }

    return NULL;
}

VOID Init_LinuxInput_inputtask(struct EventHandler * eh)
{
    struct inputtask_params p;
    p.ok_signal = AllocSignal(-1L);
    p.nok_signal = AllocSignal(-1L);
    p.eh = eh;
    p.creator = FindTask(NULL);

    D(bug("init_input_task: p.lsd = %p, p.creator = %p\n", p.lsd, p.creator));
    
    D(bug("SIGNALS ALLOCATED\n"));
    
    eh->inputtask = create_inputtask(&p);
    
    D(bug("INPUTTASK CREATED\n"));

    /* No need for these anymore */
    FreeSignal(p.ok_signal);
    FreeSignal(p.nok_signal);
}

VOID Kill_LinuxInput_inputtask(struct EventHandler * eh)
{
    /* TODO: Create signal in this task and pass it somehow to input_task */
    Signal(eh->inputtask, SIGBREAKF_CTRL_C);
    /* TODO: wait input task to mark previously crated signal */
}
