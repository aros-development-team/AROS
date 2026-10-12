/* Playback through ahi.device for Rust: two CMD_WRITE requests in turn,
   each linked to the one before so AHI plays them back to back.

   The reply port signals the task that created it, so one task must make
   every call. */
#include <exec/memory.h>
#include <devices/ahi.h>
#include <proto/exec.h>

/* Above the GUI, below input.device (20). */
#define AUDIO_PRI 5

struct aros_ahi {
    struct MsgPort *port;
    struct AHIRequest *req[2];
    BOOL sent[2];
    APTR buf[2];
    ULONG cap[2];
    struct AHIRequest *link;
    int cur;
};

static void wait_req(struct aros_ahi *a, int i)
{
    if (a->sent[i]) {
        WaitIO((struct IORequest *)a->req[i]);
        a->sent[i] = FALSE;
    }
}

struct aros_ahi *aros_ahi_open(void)
{
    struct aros_ahi *a = AllocVec(sizeof(*a), MEMF_ANY | MEMF_CLEAR);

    if (!a)
        return NULL;
    if ((a->port = CreateMsgPort())
        && (a->req[0] = (struct AHIRequest *)CreateIORequest(a->port, sizeof(struct AHIRequest)))) {
        a->req[0]->ahir_Version = 4;
        if (OpenDevice((CONST_STRPTR)AHINAME, AHI_DEFAULT_UNIT, (struct IORequest *)a->req[0], 0) == 0) {
            if ((a->req[1] = AllocVec(sizeof(struct AHIRequest), MEMF_ANY))) {
                CopyMem(a->req[0], a->req[1], sizeof(struct AHIRequest));
                SetTaskPri(FindTask(NULL), AUDIO_PRI);
                return a;
            }
            CloseDevice((struct IORequest *)a->req[0]);
        }
    }
    if (a->req[0])
        DeleteIORequest((struct IORequest *)a->req[0]);
    if (a->port)
        DeleteMsgPort(a->port);
    FreeVec(a);
    return NULL;
}

int aros_ahi_write(struct aros_ahi *a, const void *data, unsigned bytes, unsigned type, unsigned rate)
{
    int i = a->cur, err = 0;
    struct AHIRequest *r = a->req[i];

    wait_req(a, i);
    if (bytes > a->cap[i]) {
        FreeVec(a->buf[i]);
        a->cap[i] = 0;
        if (!(a->buf[i] = AllocVec(bytes, MEMF_PUBLIC)))
            return AHIE_NOMEM;
        a->cap[i] = bytes;
    }
    CopyMem((APTR)data, a->buf[i], bytes);

    r->ahir_Std.io_Message.mn_Node.ln_Pri = 0;
    r->ahir_Std.io_Command = CMD_WRITE;
    r->ahir_Std.io_Data = a->buf[i];
    r->ahir_Std.io_Length = bytes;
    r->ahir_Std.io_Offset = 0;
    r->ahir_Type = type;
    r->ahir_Frequency = rate;
    r->ahir_Volume = 0x10000;
    r->ahir_Position = 0x8000;
    /* AHI starts it at once if the linked request has already finished. */
    r->ahir_Link = a->link;
    SendIO((struct IORequest *)r);
    a->sent[i] = TRUE;

    if (a->link) {
        wait_req(a, i ^ 1);
        err = a->req[i ^ 1]->ahir_Std.io_Error;
    }
    a->link = r;
    a->cur = i ^ 1;
    return err;
}

void aros_ahi_drain(struct aros_ahi *a)
{
    wait_req(a, a->cur);
    wait_req(a, a->cur ^ 1);
    a->link = NULL;
}

void aros_ahi_close(struct aros_ahi *a)
{
    int i;

    for (i = 0; i < 2; i++) {
        if (a->sent[i]) {
            AbortIO((struct IORequest *)a->req[i]);
            wait_req(a, i);
        }
    }
    CloseDevice((struct IORequest *)a->req[0]);
    FreeVec(a->req[1]);
    DeleteIORequest((struct IORequest *)a->req[0]);
    DeleteMsgPort(a->port);
    FreeVec(a->buf[0]);
    FreeVec(a->buf[1]);
    FreeVec(a);
}
