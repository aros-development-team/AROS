/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/ports.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#define NUM_MESSAGES (1000000)

AROS_UFH3(void, taskentry,
          AROS_UFHA(STRPTR,            argPtr,  A0),
          AROS_UFHA(ULONG,             argSize, D0),
          AROS_UFHA(struct ExecBase *, SysBase, A6)) {
    AROS_USERFUNC_INIT

    int i;
    struct MsgPort *port = (struct MsgPort *) FindTask(NULL)->tc_UserData;

    for (i = 0; i < NUM_MESSAGES; i++) {
        WaitPort(port);
        ReplyMsg(GetMsg(port));
    }

    AROS_USERFUNC_EXIT
}

AROS_INTH1(intentry, struct MsgPort *, port)
{
    AROS_INTFUNC_INIT

    WaitPort(port);
    ReplyMsg(GetMsg(port));

    return 0;

    AROS_INTFUNC_EXIT
}

AROS_UFH4(void, fastentry,
          AROS_UFHA(APTR,              data,    A1),
          AROS_UFHA(APTR,              code,    A5),
          AROS_UFHA(struct Message *,  msg,     D0),
          AROS_UFHA(struct ExecBase *, SysBase, A6)) {
    AROS_USERFUNC_INIT

    ReplyMsg(msg);

    AROS_USERFUNC_EXIT
}

AROS_UFH2(void, callentry,
          AROS_UFHA(struct MsgPort *,  port,    D0),
          AROS_UFHA(struct ExecBase *, SysBase, A6)) {
    AROS_USERFUNC_INIT

    WaitPort(port);
    ReplyMsg(GetMsg(port));

    AROS_USERFUNC_EXIT
}

int main(int argc, char **argv) {
    struct MsgPort *port, *reply;
    struct Message *msg;
    struct Process *proc;
    int i;
    struct timeval start, end;
    struct Interrupt *intr;

    printf("testing with %d messages\n", NUM_MESSAGES);

    port = CreateMsgPort();
    reply = CreateMsgPort();

    msg = AllocVec(sizeof(struct Message), MEMF_PUBLIC | MEMF_CLEAR);
    msg->mn_Length = sizeof(struct Message);
    msg->mn_ReplyPort = reply;

    proc = CreateNewProcTags(NP_Entry,    (IPTR) taskentry,
                             NP_Name,     "timeport task",
                             NP_UserData, (IPTR) port,
                             TAG_DONE);

    port->mp_Flags = PA_SIGNAL;
    port->mp_SigTask = (struct Task *) proc;

    gettimeofday(&start, NULL);

    for (i = 0; i < NUM_MESSAGES; i++) {
        PutMsg(port, msg);
        WaitPort(reply);
        GetMsg(reply);
    }

    gettimeofday(&end, NULL);

    while (end.tv_usec < start.tv_usec) {
        end.tv_sec--;
        end.tv_usec += 1000000;
    }

    printf("PA_SIGNAL: %ld.%lds\n",(long)( end.tv_sec - start.tv_sec), (long)(end.tv_usec - start.tv_usec) / 1000);

    intr = AllocVec(sizeof(struct Interrupt), MEMF_PUBLIC | MEMF_CLEAR);
    intr->is_Code = (APTR)intentry;
    intr->is_Data = port;

    port->mp_Flags = PA_SOFTINT;
    port->mp_SoftInt = intr;

    gettimeofday(&start, NULL);

    for (i = 0; i < NUM_MESSAGES; i++) {
        PutMsg(port, msg);
        WaitPort(reply);
        GetMsg(reply);
    }

    gettimeofday(&end, NULL);

    while (end.tv_usec < start.tv_usec) {
        end.tv_sec--;
        end.tv_usec += 1000000;
    }

    printf("PA_SOFTINT: %ld.%lds\n", (long)(end.tv_sec - start.tv_sec), (long)(end.tv_usec - start.tv_usec) / 1000);

    port->mp_Flags = PA_CALL;
    port->mp_SigTask = callentry;

    gettimeofday(&start, NULL);

    for (i = 0; i < NUM_MESSAGES; i++) {
        PutMsg(port, msg);
        WaitPort(reply);
        GetMsg(reply);
    }

    gettimeofday(&end, NULL);

    while (end.tv_usec < start.tv_usec) {
        end.tv_sec--;
        end.tv_usec += 1000000;
    }

    printf("PA_CALL: %ld.%lds\n", (long)(end.tv_sec - start.tv_sec), (long)(end.tv_usec - start.tv_usec) / 1000);

    intr->is_Code = fastentry;

    port->mp_Flags = PA_FASTCALL;
    port->mp_SoftInt = intr;

    gettimeofday(&start, NULL);

    for (i = 0; i < NUM_MESSAGES; i++) {
        PutMsg(port, msg);
        WaitPort(reply);
        GetMsg(reply);
    }

    gettimeofday(&end, NULL);

    while (end.tv_usec < start.tv_usec) {
        end.tv_sec--;
        end.tv_usec += 1000000;
    }

    printf("PA_FASTCALL: %ld.%lds\n", (long)(end.tv_sec - start.tv_sec), (long)(end.tv_usec - start.tv_usec) / 1000);

    FreeVec(intr);

    FreeVec(msg);

    DeleteMsgPort(reply);
    DeleteMsgPort(port);

    return 0;
}
