/* Playback through AHI for Rust.

   By default the sound goes to the AHI Music unit (the audio mode chosen in
   AHI prefs), through the low-level API: two dynamic sounds of one write
   each, played back to back. AHI calls the sound hook when one starts; the
   hook queues the other to follow and wakes the writer to refill it.

   The environment variable AHI_UNIT=0..3 sends the sound to that ahi.device
   unit instead: two CMD_WRITE requests in turn, each linked to the one
   before so AHI plays them back to back.

   The writer's signal and the reply port belong to the task that opened
   the stream, so one task must make every call. */
#include <exec/memory.h>
#include <devices/ahi.h>
#include <utility/hooks.h>
#include <proto/exec.h>
#include <proto/ahi.h>
#include <stdlib.h>
#include <string.h>

/* Above the GUI, below input.device (20). */
#define AUDIO_PRI 5

struct aros_ahi {
    struct MsgPort *port;
    struct AHIRequest *req[2];
    APTR buf[2];
    ULONG cap[2];
    int unit;                   /* AHI_NO_UNIT: the Music unit */

    /* ahi.device units: CMD_WRITE */
    BOOL sent[2];
    struct AHIRequest *link;
    int cur;

    /* the Music unit: AHI_AllocAudio() */
    struct Library *ahibase;
    struct AHIAudioCtrl *actrl;
    struct Hook hook;
    struct AHISampleInfo sample[2];
    struct Task *task;
    BYTE sig;
    BOOL loaded, playing;
    ULONG bytes;                /* of each sound */
    unsigned type, rate;
    /* set by the hook, read by the writer under Disable() */
    volatile int queued;        /* the sound that plays next: the one to fill */
    volatile ULONG starts;      /* sounds started so far */
    ULONG filled;               /* starts value when the queued sound was filled */
    volatile BOOL fresh[2];     /* refilled since it last started */
};

static void wait_req(struct aros_ahi *a, int i)
{
    if (a->sent[i]) {
        WaitIO((struct IORequest *)a->req[i]);
        a->sent[i] = FALSE;
    }
}

/* AHI calls this when a sound starts: queue the other one after it. A sound
   that was not refilled in time plays silence rather than the old data. */
AROS_UFH3(ULONG, soundFunc,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(struct AHIAudioCtrl *, actrl, A2),
          AROS_UFHA(struct AHISoundMessage *, smsg, A1))
{
    AROS_USERFUNC_INIT
    struct aros_ahi *a = hook->h_Data;
    struct Library *AHIBase = a->ahibase;
    int now = a->queued;

    (void)smsg;
    if (!a->fresh[now])
        memset(a->buf[now], 0, a->bytes);
    a->fresh[now] = FALSE;
    a->queued = now ^ 1;
    AHI_SetSound(0, a->queued, 0, 0, actrl, AHISF_NONE);
    a->starts++;
    Signal(a->task, 1UL << a->sig);
    return 0;
    AROS_USERFUNC_EXIT
}

static int unit_from_env(void)
{
    const char *s = getenv("AHI_UNIT");

    if (s && s[0] >= '0' && s[0] <= '3' && s[1] == '\0')
        return s[0] - '0';
    return AHI_NO_UNIT;
}

struct aros_ahi *aros_ahi_open(void)
{
    struct aros_ahi *a = AllocVec(sizeof(*a), MEMF_ANY | MEMF_CLEAR);

    if (!a)
        return NULL;
    a->unit = unit_from_env();
    a->sig = -1;
    if ((a->port = CreateMsgPort())
        && (a->req[0] = (struct AHIRequest *)CreateIORequest(a->port, sizeof(struct AHIRequest)))) {
        a->req[0]->ahir_Version = 4;
        if (OpenDevice((CONST_STRPTR)AHINAME, a->unit, (struct IORequest *)a->req[0], 0) == 0) {
            if (a->unit == AHI_NO_UNIT) {
                struct Library *AHIBase = (struct Library *)a->req[0]->ahir_Std.io_Device;

                a->ahibase = AHIBase;
                a->task = FindTask(NULL);
                a->hook.h_Entry = (HOOKFUNC)soundFunc;
                a->hook.h_Data = a;
                if ((a->sig = AllocSignal(-1)) != -1
                    && (a->actrl = AHI_AllocAudio(AHIA_AudioID, AHI_DEFAULT_ID,
                                                  AHIA_MixFreq, AHI_DEFAULT_FREQ,
                                                  AHIA_Channels, 1,
                                                  AHIA_Sounds, 2,
                                                  AHIA_SoundFunc, (IPTR)&a->hook,
                                                  TAG_DONE))) {
                    SetTaskPri(a->task, AUDIO_PRI);
                    return a;
                }
                /* the Music unit is exclusive: another program may have it */
                if (a->sig != -1)
                    FreeSignal(a->sig);
            } else if ((a->req[1] = AllocVec(sizeof(struct AHIRequest), MEMF_ANY))) {
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

static int unit_write(struct aros_ahi *a, const void *data, unsigned bytes, unsigned type, unsigned rate)
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

static ULONG frame_bytes(unsigned type)
{
    return (type == AHIST_S16S) ? 4 : (type == AHIST_M16S) ? 2 : 0;
}

/* The first write sets the sound size, type and rate, loads both sounds
   and starts playback with the data in sound 0. */
static int music_start(struct aros_ahi *a, const void *data, unsigned bytes, unsigned type, unsigned rate)
{
    struct Library *AHIBase = a->ahibase;
    ULONG fb = frame_bytes(type);
    int i;

    if (fb == 0 || bytes < fb)
        return AHIE_BADSAMPLETYPE;
    a->bytes = bytes - bytes % fb;
    a->type = type;
    a->rate = rate;
    for (i = 0; i < 2; i++) {
        if (!(a->buf[i] = AllocVec(a->bytes, MEMF_PUBLIC | MEMF_CLEAR)))
            return AHIE_NOMEM;
        a->cap[i] = a->bytes;
        a->sample[i].ahisi_Type = type;
        a->sample[i].ahisi_Address = a->buf[i];
        a->sample[i].ahisi_Length = a->bytes / fb;
    }
    for (i = 0; i < 2; i++)
        if (AHI_LoadSound(i, AHIST_DYNAMICSAMPLE, &a->sample[i], a->actrl) != AHIE_OK) {
            if (i)
                AHI_UnloadSound(0, a->actrl);
            return AHIE_UNKNOWN;
        }
    a->loaded = TRUE;

    CopyMem((APTR)data, a->buf[0], a->bytes);
    a->fresh[0] = TRUE;
    a->queued = 0;              /* the hook's first call is for sound 0 */
    a->starts = 0;
    a->filled = 0;
    SetSignal(0, 1UL << a->sig);
    AHI_Play(a->actrl,
             AHIP_BeginChannel, 0,
             AHIP_Freq, rate,
             AHIP_Vol, 0x10000,
             AHIP_Pan, 0x8000,
             AHIP_Sound, 0,
             AHIP_EndChannel, 0,
             TAG_DONE);
    if (AHI_ControlAudio(a->actrl, AHIC_Play, TRUE, TAG_DONE) != AHIE_OK)
        return AHIE_UNKNOWN;
    a->playing = TRUE;
    return 0;
}

/* Fills the queued sound once per sound started, waiting for the start. */
static int music_write(struct aros_ahi *a, const void *data, unsigned bytes, unsigned type, unsigned rate)
{
    struct Library *AHIBase = a->ahibase;
    ULONG n;
    int q;

    if (!a->playing)
        return music_start(a, data, bytes, type, rate);
    if (rate != a->rate) {
        AHI_SetFreq(0, rate, a->actrl, AHISF_IMM);
        a->rate = rate;
    }
    for (;;) {
        Disable();
        n = a->starts;
        q = a->queued;
        Enable();
        if (n != a->filled)
            break;
        Wait(1UL << a->sig);
    }
    n = bytes < a->bytes ? bytes : a->bytes;
    CopyMem((APTR)data, a->buf[q], n);
    if (n < a->bytes)
        memset((UBYTE *)a->buf[q] + n, 0, a->bytes - n);
    a->fresh[q] = TRUE;
    a->filled = a->starts;
    return 0;
}

int aros_ahi_write(struct aros_ahi *a, const void *data, unsigned bytes, unsigned type, unsigned rate)
{
    if (a->unit == AHI_NO_UNIT)
        return music_write(a, data, bytes, type, rate);
    return unit_write(a, data, bytes, type, rate);
}

void aros_ahi_drain(struct aros_ahi *a)
{
    if (a->unit == AHI_NO_UNIT) {
        ULONG until;

        if (!a->playing)
            return;
        /* the last sound filled starts, then plays out */
        Disable();
        until = a->starts + 2;
        Enable();
        while ((LONG)(a->starts - until) < 0)
            Wait(1UL << a->sig);
        return;
    }
    wait_req(a, a->cur);
    wait_req(a, a->cur ^ 1);
    a->link = NULL;
}

void aros_ahi_close(struct aros_ahi *a)
{
    int i;

    if (a->unit == AHI_NO_UNIT) {
        struct Library *AHIBase = a->ahibase;

        if (a->playing)
            AHI_ControlAudio(a->actrl, AHIC_Play, FALSE, TAG_DONE);
        if (a->loaded) {
            AHI_UnloadSound(0, a->actrl);
            AHI_UnloadSound(1, a->actrl);
        }
        AHI_FreeAudio(a->actrl);
        FreeSignal(a->sig);
    } else {
        for (i = 0; i < 2; i++) {
            if (a->sent[i]) {
                AbortIO((struct IORequest *)a->req[i]);
                wait_req(a, i);
            }
        }
        FreeVec(a->req[1]);
    }
    CloseDevice((struct IORequest *)a->req[0]);
    DeleteIORequest((struct IORequest *)a->req[0]);
    DeleteMsgPort(a->port);
    FreeVec(a->buf[0]);
    FreeVec(a->buf[1]);
    FreeVec(a);
}
