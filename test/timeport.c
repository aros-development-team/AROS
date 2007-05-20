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

struct MsgPort *port;

AROS_UFH3(void, taskentry,
          AROS_UFHA(STRPTR,            argPtr,  A0),
          AROS_UFHA(ULONG,             argSize, D0),
          AROS_UFHA(struct ExecBase *, SysBase, A6)) {
    AROS_USERFUNC_INIT

    int i;

    for (i = 0; i < NUM_MESSAGES; i++) {
        WaitPort(port);
        ReplyMsg(GetMsg(port));
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, intentry,
          AROS_UFHA(APTR,              data,    A1),
          AROS_UFHA(APTR,              code,    A5),
          AROS_UFHA(struct ExecBase *, SysBase, A6)) {
    AROS_USERFUNC_INIT

    WaitPort(port);
    ReplyMsg(GetMsg(port));

    AROS_USERFUNC_EXIT
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

AROS_UFH1(void, callentry,
          AROS_UFHA(struct ExecBase *, SysBase, A6)) {
    AROS_USERFUNC_INIT

    WaitPort(port);
    ReplyMsg(GetMsg(port));

    AROS_USERFUNC_EXIT
}

int main(int argc, char **argv) {
    struct MsgPort *reply;
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

    printf("PA_SIGNAL: %ld.%lds\n", end.tv_sec - start.tv_sec, (end.tv_usec - start.tv_usec) / 1000);

    intr = AllocVec(sizeof(struct Interrupt), MEMF_PUBLIC | MEMF_CLEAR);
    intr->is_Code = intentry;

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

    printf("PA_SOFTINT: %ld.%lds\n", end.tv_sec - start.tv_sec, (end.tv_usec - start.tv_usec) / 1000);

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

    printf("PA_CALL: %ld.%lds\n", end.tv_sec - start.tv_sec, (end.tv_usec - start.tv_usec) / 1000);

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

    printf("PA_FASTCALL: %ld.%lds\n", end.tv_sec - start.tv_sec, (end.tv_usec - start.tv_usec) / 1000);

    FreeVec(intr);

    FreeVec(msg);

    DeleteMsgPort(reply);
    DeleteMsgPort(port);

    return 0;
}
