/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <dos/dostags.h>
#include <exec/memory.h>
#include <string.h>

static ULONG failures;

static void check(BOOL okay, CONST_STRPTR what)
{
    if (!okay)
    {
        bug("CD BOOT CONSOLE FAIL: %s\n", what);
        failures++;
    }
}

int main(int argc, char **argv)
{
    struct Process *process = (struct Process *)FindTask(NULL);
    struct FileHandle *input = BADDR(Input());
    struct FileHandle *output = BADDR(Output());
    struct FileHandle *error_output = BADDR(process->pr_CES);
    struct MsgPort *reply = NULL;
    struct StandardPacket packet;
    struct InfoData info;
    ULONG query_free, query_largest;
    UBYTE byte;

    if (argc > 1 && !strcmp(argv[1], "CHILD"))
    {
        check(process->pr_ConsoleTask != NULL,
              "synchronous quiet child retains the console identity");
        check(input && !input->fh_Type && output && !output->fh_Type &&
              error_output && !error_output->fh_Type,
              "default child streams remain NIL despite a console identity");
        bug("CD BOOT CONSOLE CHILD TEST: %lu failures\n", (unsigned long)failures);
        return failures ? 20 : 0;
    }

    /* Run directly from the CD's Startup-Sequence, without host --run. */
    check(process->pr_ConsoleTask != NULL, "CD boot retains a process console port");
    check(input && !input->fh_Type && !IsInteractive(Input()),
          "quiet boot input remains a NIL source");
    check(FindTask("CON") == NULL, "boot does not eagerly create a console handler");
    bug("CD BOOT CONSOLE MEMORY BEFORE QUERY: free %lu largest %lu\n",
        (unsigned long)AvailMem(MEMF_CHIP),
        (unsigned long)AvailMem(MEMF_CHIP | MEMF_LARGEST));
    check(output && !output->fh_Type && !IsInteractive(Output()),
          "quiet boot output is a NIL sink");
    check(error_output && !IsInteractive(process->pr_CES),
          "default error output does not instantiate a console");
    check(Read(Input(), &byte, 1) == 0, "quiet input returns end-of-file");
    check(error_output && Write(process->pr_CES, "test", 4) == 4,
          "default stderr accepts output without a window");
    if (!process->pr_ConsoleTask)
        goto end;
    reply = CreateMsgPort();
    check(reply != NULL, "create packet reply port");
    if (!reply)
        goto end;

    query_free = AvailMem(MEMF_CHIP);
    query_largest = AvailMem(MEMF_CHIP | MEMF_LARGEST);

    /* Match legacy launchers that send a packet with PutMsg, bypassing
     * DoPkt's special handling of NIL. Never send to a NULL port. */
    memset(&packet, 0, sizeof(packet));
    memset(&info, 0, sizeof(info));
    packet.sp_Msg.mn_Node.ln_Name = (STRPTR)&packet.sp_Pkt;
    packet.sp_Msg.mn_Length = sizeof(packet);
    packet.sp_Pkt.dp_Link = &packet.sp_Msg;
    packet.sp_Pkt.dp_Port = reply;
    packet.sp_Pkt.dp_Type = ACTION_DISK_INFO;
    packet.sp_Pkt.dp_Arg1 = MKBADDR(&info);
    PutMsg(process->pr_ConsoleTask, &packet.sp_Msg);
    WaitPort(reply);
    check(GetMsg(reply) == &packet.sp_Msg, "console replies to direct DOS packets");
    check(packet.sp_Pkt.dp_Res1 == DOSTRUE &&
          info.id_DiskType == 0x434f4e00, /* CON\0 */
          "console information identifies an interactive console");
    check(info.id_InUse != 0, "console information exposes the read request");
    check(FindTask("CON") == NULL, "information query needs no console process");
    if (info.id_InUse)
    {
        struct IOStdReq *request = (struct IOStdReq *)info.id_InUse;
        check(request->io_Message.mn_Length == sizeof(*request) &&
              request->io_Device == NULL && request->io_Unit == NULL,
              "windowless console exposes an inactive read request");
    }
    check(info.id_VolumeNode == BNULL, "information query leaves AUTO window closed");
    check(AvailMem(MEMF_CHIP) == query_free &&
          AvailMem(MEMF_CHIP | MEMF_LARGEST) == query_largest,
          "console information query does not allocate or fragment Chip RAM");
    check(SystemTags("cdbootconsole CHILD", SYS_Input, Input(),
                     SYS_Output, Output(), SYS_Asynch, FALSE, TAG_DONE) == 0,
          "synchronous quiet child keeps NIL defaults without extra buffering");
end:
    if (reply)
        DeleteMsgPort(reply);
    bug("CD BOOT CONSOLE MEMORY: free %lu largest %lu\n",
        (unsigned long)AvailMem(MEMF_CHIP),
        (unsigned long)AvailMem(MEMF_CHIP | MEMF_LARGEST));
    bug("CD BOOT CONSOLE TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
