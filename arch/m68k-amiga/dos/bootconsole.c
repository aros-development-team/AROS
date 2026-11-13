/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include "dos_intern.h"

/* CD boot keeps NIL standard streams. A windowless console port also
 * answers legacy setup packets, without charging every game for a CON
 * process, Workbench libraries or a window. The read request is inactive,
 * as it is for an AUTO console whose window has not been opened. */
AROS_UFH2(static void, boot_console_packets,
    AROS_UFHA(struct MsgPort *, port, D0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct BootConsole *state = (struct BootConsole *)port;
    struct Message *message;

    while ((message = GetMsg(port)) != NULL)
    {
        struct DosPacket *packet = (struct DosPacket *)message->mn_Node.ln_Name;
        struct MsgPort *reply = packet->dp_Port;
        SIPTR result = DOSTRUE;
        LONG error = 0;

        switch (packet->dp_Type)
        {
            case ACTION_FINDINPUT:
            case ACTION_FINDUPDATE:
            case ACTION_FINDOUTPUT:
            {
                struct FileHandle *fh = BADDR(packet->dp_Arg1);
                fh->fh_Type = port;
                fh->fh_Interactive = DOSFALSE;
                break;
            }
            case ACTION_READ:
            case ACTION_SEEK:
            case ACTION_PARENT_FH:
                result = 0;
                break;
            case ACTION_WRITE:
                result = packet->dp_Arg3;
                break;
            case ACTION_END:
            case ACTION_CHANGE_SIGNAL:
                break;
            case ACTION_IS_FILESYSTEM:
                result = DOSFALSE;
                break;
            case ACTION_SCREEN_MODE:
                state->raw = packet->dp_Arg1 != 0;
                break;
            case ACTION_DISK_INFO:
            {
                struct InfoData *info = BADDR(packet->dp_Arg1);
                memset(info, 0, sizeof(*info));
                info->id_DiskType = state->raw ? 0x52415700 : 0x434f4e00;
                info->id_InUse = (IPTR)&state->read_request;
                break;
            }
            default:
                result = DOSFALSE;
                error = ERROR_ACTION_NOT_KNOWN;
                break;
        }
        packet->dp_Res1 = result;
        packet->dp_Res2 = error;
        packet->dp_Port = port;
        PutMsg(reply, message);
    }

    AROS_USERFUNC_EXIT
}

BPTR internal_OpenBootConsole(BOOL quiet, struct BootConsole *state,
    struct DosLibrary *DOSBase)
{
    struct Process *process = (struct Process *)FindTask(NULL);
    BPTR input;

    state->quiet = FALSE;
    if (!quiet)
        return Open(BOOT_CONSOLE_NAME, MODE_OLDFILE);
    input = Open("NIL:", MODE_OLDFILE);
    if (!input)
        return BNULL;
    memset(state, 0, sizeof(*state));
    state->saved_console = process->pr_ConsoleTask;
    state->port.mp_Node.ln_Type = NT_MSGPORT;
    state->port.mp_Node.ln_Name = "CD boot console";
    state->port.mp_Flags = PA_CALL;
    state->port.mp_SigTask = (struct Task *)boot_console_packets;
    NewList(&state->port.mp_MsgList);
    state->read_request.io_Message.mn_ReplyPort = &state->port;
    state->read_request.io_Message.mn_Length = sizeof(state->read_request);
    process->pr_ConsoleTask = &state->port;
    state->quiet = TRUE;
    return input;
}

void internal_CloseBootConsole(struct BootConsole *state,
    struct DosLibrary *DOSBase)
{
    (void)DOSBase;
    if (state->quiet)
    {
        struct Process *process = (struct Process *)FindTask(NULL);
        process->pr_ConsoleTask = state->saved_console;
    }
}

BPTR internal_OpenConsoleOutput(struct DosLibrary *DOSBase)
{
    struct Process *process = (struct Process *)FindTask(NULL);
    struct MsgPort *port = process->pr_ConsoleTask;

    /* The endpoint supplies a console identity for legacy setup packets,
     * while quiet standard streams remain NIL, including in child shells.
     * Opening a handler-backed stream here would add unnecessary buffers. */
    if (port && port->mp_Flags == PA_CALL &&
        port->mp_SigTask == (struct Task *)boot_console_packets)
        return Open("NIL:", MODE_NEWFILE);

    return Open("*", MODE_NEWFILE);
}
