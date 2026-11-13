/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <devices/audio.h>
#include <devices/narrator.h>
#include <devices/newstyle.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <proto/alib.h>
#include <proto/exec.h>

#include <string.h>

#include "narrator_intern.h"
#include <libraries/aros_resource.h>
#include <libraries/speechcore.h>

#include LC_LIBDEFS_FILE

#define WORKER_STACK AROS_STACKSIZE
#define RESOURCE_PATH "DEVS:Speech/speech.iff"
#define PLAYBACK_BUFFER 8192
#define NARRATOR_RELEASE_AUDIO 0xfffe

#if PLAYBACK_BUFFER % SC_PCM_BLOCK_SIZE != 0
#error PLAYBACK_BUFFER must contain complete speechcore blocks
#endif

static const UBYTE DefaultMasks[] = { 3, 5, 10, 12 };
static const UWORD SupportedCommands[] = {
    CMD_RESET, CMD_READ, CMD_WRITE, CMD_UPDATE, CMD_CLEAR,
    CMD_STOP, CMD_START, CMD_FLUSH, NSCMD_DEVICEQUERY, 0
};

struct NarratorSink
{
    struct NarratorBase *base;
    struct narrator_rb *write;
    struct IOAudio *audio[2];
    UBYTE *audio_buffer[2];
    ULONG used[2];
    UBYTE in_flight[2];
    UBYTE next;
    UWORD period;
    UWORD volume;
    UBYTE last_shape;
    UBYTE have_shape;
};

static void *SinkAllocate(void *context, size_t bytes)
{
    (void)context;
    return AllocVec(bytes, MEMF_ANY);
}

static void SinkRelease(void *context, void *memory)
{
    (void)context;
    FreeVec(memory);
}

static void ReplyRequest(struct narrator_rb *request, BYTE error)
{
    request->message.io_Error = error;
    ReplyMsg(&request->message.io_Message);
}

static BOOL NodeIsListed(const struct List *list, const struct Node *wanted)
{
    const struct Node *node;

    for (node = list->lh_Head; node->ln_Succ != NULL; node = node->ln_Succ)
        if (node == wanted)
            return TRUE;
    return FALSE;
}

static struct narrator_rb *GetRequest(struct NarratorBase *base)
{
    struct narrator_rb *request;

    ObtainSemaphore(&base->nb_Lock);
    request = (struct narrator_rb *)GetMsg(base->nb_CommandPort);
    if (request != NULL && request->message.io_Command == CMD_WRITE)
    {
        if (base->nb_ControlsPending == 0 && !base->nb_Closing)
            base->nb_Cancel = 0;
        base->nb_Current = request;
    }
    ReleaseSemaphore(&base->nb_Lock);
    return request;
}

static void FinishRequest(struct NarratorBase *base,
                           struct narrator_rb *request, BYTE error)
{
    ObtainSemaphore(&base->nb_Lock);
    if (base->nb_Current == request)
        base->nb_Current = NULL;
    ReleaseSemaphore(&base->nb_Lock);
    ReplyRequest(request, error);
}

static int WaitUntilRunning(struct NarratorBase *base)
{
    BOOL stopped, cancelled;

    for (;;)
    {
        ObtainSemaphore(&base->nb_Lock);
        stopped = base->nb_Stopped;
        cancelled = base->nb_Cancel;
        ReleaseSemaphore(&base->nb_Lock);
        if (!stopped || cancelled)
            return cancelled;
        Wait(1UL << base->nb_CommandPort->mp_SigBit);
    }
}

static int SinkCancelled(void *context)
{
    struct NarratorSink *sink = context;
    return WaitUntilRunning(sink->base);
}

static int SinkWaitAudio(struct NarratorSink *sink, unsigned slot,
                           int cancellable)
{
    struct IOAudio *audio = sink->audio[slot];

    if (!sink->in_flight[slot])
        return 0;
    while (CheckIO(&audio->ioa_Request) == NULL)
    {
        Wait((1UL << audio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit) |
             (1UL << sink->base->nb_CommandPort->mp_SigBit));
        if (cancellable && sink->base->nb_Cancel)
        {
            AbortIO(&audio->ioa_Request);
            WaitIO(&audio->ioa_Request);
            sink->in_flight[slot] = 0;
            return 1;
        }
    }
    sink->in_flight[slot] = 0;
    return WaitIO(&audio->ioa_Request) == 0 ? 0 : 1;
}

static int SinkPCM(void *context, const int8_t *samples, size_t count,
                    uint16_t period, uint16_t volume)
{
    struct NarratorSink *sink = context;
    unsigned slot = sink->next;
    struct IOAudio *audio = sink->audio[slot];

    if (SinkCancelled(context))
        return 1;
    if (count > PLAYBACK_BUFFER - sink->used[slot])
        return 1;
    if (sink->used[slot] == 0 && SinkWaitAudio(sink, slot, 1) != 0)
        return 1;
    CopyMem((APTR)samples, sink->audio_buffer[slot] + sink->used[slot],
            count);
    sink->used[slot] += count;
    sink->period = period;
    sink->volume = volume;
    if (sink->used[slot] != PLAYBACK_BUFFER)
        return 0;
    audio->ioa_Request.io_Command = CMD_WRITE;
    audio->ioa_Request.io_Flags = ADIOF_PERVOL;
    audio->ioa_Data = sink->audio_buffer[slot];
    audio->ioa_Length = sink->used[slot];
    audio->ioa_Period = period;
    audio->ioa_Volume = volume;
    audio->ioa_Cycles = 1;
    /* Preserve ADIOF_PERVOL; SendIO() would clear it before BeginIO(). */
    BeginIO(&audio->ioa_Request);
    sink->in_flight[slot] = 1;
    sink->used[slot] = 0;
    sink->next ^= 1;
    return 0;
}

static int SinkFinishAudio(struct NarratorSink *sink)
{
    unsigned slot = sink->next;
    struct IOAudio *audio = sink->audio[slot];

    if (sink->used[slot] != 0)
    {
        if (SinkWaitAudio(sink, slot, 1) != 0)
            return 1;
        audio->ioa_Request.io_Command = CMD_WRITE;
        audio->ioa_Request.io_Flags = ADIOF_PERVOL;
        audio->ioa_Data = sink->audio_buffer[slot];
        audio->ioa_Length = sink->used[slot];
        audio->ioa_Period = sink->period;
        audio->ioa_Volume = sink->volume;
        audio->ioa_Cycles = 1;
        BeginIO(&audio->ioa_Request);
        sink->in_flight[slot] = 1;
        sink->used[slot] = 0;
    }
    if (SinkWaitAudio(sink, 0, 1) != 0)
        return 1;
    return SinkWaitAudio(sink, 1, 1);
}

static int SinkMouth(void *context, const uint8_t *shapes, size_t count)
{
    struct NarratorSink *sink = context;
    size_t at;

    for (at = 0; at < count; ++at)
    {
        struct narrator_rb *request;
        UBYTE *event;

        if (sink->have_shape && sink->last_shape == shapes[at])
            continue;
        sink->last_shape = shapes[at];
        sink->have_shape = 1;
        ObtainSemaphore(&sink->base->nb_Lock);
        request = (struct narrator_rb *)RemHead(&sink->base->nb_ReadQueue);
        ReleaseSemaphore(&sink->base->nb_Lock);
        if (request == NULL)
            continue;
        if (request->message.io_Message.mn_Length >= sizeof(struct mouth_rb) &&
            (request->flags & NDF_NEWIORB))
            event = &((struct mouth_rb *)request)->width;
        else
            event = (UBYTE *)request + NARRATOR_PRE_V37_SIZE;
        event[0] = shapes[at] & 0x0f;
        event[1] = shapes[at] >> 4;
        event[2] = shapes[at];
        event[3] = NDF_READMOUTH;
        request->message.io_Actual = 1;
        ReplyRequest(request, 0);
    }
    return 0;
}

static void FinishReads(struct NarratorBase *base, BYTE error)
{
    struct narrator_rb *request;

    for (;;)
    {
        ObtainSemaphore(&base->nb_Lock);
        request = (struct narrator_rb *)RemHead(&base->nb_ReadQueue);
        ReleaseSemaphore(&base->nb_Lock);
        if (request == NULL)
            break;
        ReplyRequest(request, error);
    }
}

static BYTE ValidateParams(const struct narrator_rb *request)
{
    if (request->rate < MINRATE || request->rate > MAXRATE)
        return ND_RateErr;
    if (request->pitch < MINPITCH || request->pitch > MAXPITCH)
        return ND_PitchErr;
    if (request->sex > FEMALE)
        return ND_SexErr;
    if (request->mode > MANUALF0)
        return ND_ModeErr;
    if (request->sampfreq < MINFREQ || request->sampfreq > MAXFREQ)
        return ND_FreqErr;
    if (request->volume > MAXVOL)
        return ND_VolErr;
    if ((request->flags & NDF_NEWIORB) &&
        request->message.io_Message.mn_Length >= sizeof(*request) &&
        request->centralize > MAXCENT)
        return ND_DCentErr;
    return 0;
}

static void CompleteWrite(struct NarratorBase *base)
{
    BOOL pending;

    ObtainSemaphore(&base->nb_Lock);
    if (base->nb_WritesPending != 0)
        --base->nb_WritesPending;
    pending = base->nb_WritesPending != 0;
    ReleaseSemaphore(&base->nb_Lock);
    if (!pending)
        FinishReads(base, ND_NoWrite);
}

static void QueueRead(struct NarratorBase *base, struct narrator_rb *request)
{
    BOOL pending;

    if (request->message.io_Message.mn_Length < MOUTH_PRE_V37_SIZE)
    {
        ReplyRequest(request, IOERR_BADLENGTH);
        return;
    }
    request->message.io_Actual = 0;
    ObtainSemaphore(&base->nb_Lock);
    pending = base->nb_WritesPending != 0;
    if (pending)
        AddTail(&base->nb_ReadQueue, &request->message.io_Message.mn_Node);
    ReleaseSemaphore(&base->nb_Lock);
    if (!pending)
        ReplyRequest(request, ND_NoWrite);
}

static void ReleaseAudio(struct NarratorBase *base)
{
    if (base->nb_Audio[0] != NULL &&
        base->nb_Audio[0]->ioa_Request.io_Device != NULL)
        CloseDevice(&base->nb_Audio[0]->ioa_Request);
    if (base->nb_AudioBuffer[0] != NULL)
        FreeVec(base->nb_AudioBuffer[0]);
    if (base->nb_AudioBuffer[1] != NULL)
        FreeVec(base->nb_AudioBuffer[1]);
    if (base->nb_Audio[1] != NULL)
        DeleteIORequest(&base->nb_Audio[1]->ioa_Request);
    if (base->nb_Audio[0] != NULL)
        DeleteIORequest(&base->nb_Audio[0]->ioa_Request);
    if (base->nb_AudioPort != NULL)
        DeleteMsgPort(base->nb_AudioPort);
    if (base->nb_ChannelMasks != NULL)
        FreeVec(base->nb_ChannelMasks);
    base->nb_AudioBuffer[0] = base->nb_AudioBuffer[1] = NULL;
    base->nb_Audio[0] = base->nb_Audio[1] = NULL;
    base->nb_AudioPort = NULL;
    base->nb_ChannelMasks = NULL;
    base->nb_ChannelMaskCount = 0;
}

static BYTE AcquireAudio(struct NarratorBase *base,
                          const struct narrator_rb *request)
{
    const UBYTE *masks = request->ch_masks != NULL && request->nm_masks != 0
        ? request->ch_masks : DefaultMasks;
    UWORD maskCount = request->ch_masks != NULL && request->nm_masks != 0
        ? request->nm_masks : sizeof(DefaultMasks);

    if (base->nb_Audio[0] != NULL &&
        base->nb_ChannelMaskCount == maskCount &&
        memcmp(base->nb_ChannelMasks, masks, maskCount) == 0)
        return 0;
    ReleaseAudio(base);
    base->nb_ChannelMasks = AllocVec(maskCount, MEMF_ANY);
    if (base->nb_ChannelMasks == NULL)
        return ND_NoMem;
    CopyMem((APTR)masks, base->nb_ChannelMasks, maskCount);
    base->nb_ChannelMaskCount = maskCount;
    base->nb_AudioPort = CreateMsgPort();
    if (base->nb_AudioPort == NULL)
    {
        ReleaseAudio(base);
        return ND_NoMem;
    }
    base->nb_Audio[0] = (struct IOAudio *)CreateIORequest(
        base->nb_AudioPort, sizeof(struct IOAudio));
    if (base->nb_Audio[0] == NULL)
        goto no_memory;
    base->nb_Audio[0]->ioa_Data = base->nb_ChannelMasks;
    base->nb_Audio[0]->ioa_Length = base->nb_ChannelMaskCount;
    if (OpenDevice(AUDIONAME, 0, &base->nb_Audio[0]->ioa_Request, 0) != 0)
    {
        base->nb_Audio[0]->ioa_Request.io_Device = NULL;
        ReleaseAudio(base);
        return ND_NoAudLib;
    }
    base->nb_Audio[0]->ioa_Request.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    base->nb_Audio[1] = (struct IOAudio *)CreateIORequest(
        base->nb_AudioPort, sizeof(struct IOAudio));
    if (base->nb_Audio[1] != NULL)
        CopyMem(base->nb_Audio[0], base->nb_Audio[1], sizeof(struct IOAudio));
    base->nb_AudioBuffer[0] = AllocVec(PLAYBACK_BUFFER, MEMF_CHIP);
    base->nb_AudioBuffer[1] = AllocVec(PLAYBACK_BUFFER, MEMF_CHIP);
    if (base->nb_Audio[1] == NULL || base->nb_AudioBuffer[0] == NULL ||
        base->nb_AudioBuffer[1] == NULL)
        goto no_memory;
    return 0;

no_memory:
    ReleaseAudio(base);
    return ND_NoMem;
}

static BYTE Speak(struct narrator_rb *request, struct NarratorBase *base)
{
    struct NarratorSink context;
    struct SCSink sink;
    struct SCNarratorParams params;
    size_t error_at = 0;
    BYTE error = 0;
    int rc;

    if (request->message.io_Data == NULL)
        return IOERR_BADLENGTH;
    error = ValidateParams(request);
    if (error != 0)
        return error;
    error = AcquireAudio(base, request);
    if (error != 0)
        return error;
    memset(&context, 0, sizeof(context));
    context.base = base;
    context.write = request;
    context.audio[0] = base->nb_Audio[0];
    context.audio[1] = base->nb_Audio[1];
    context.audio_buffer[0] = base->nb_AudioBuffer[0];
    context.audio_buffer[1] = base->nb_AudioBuffer[1];

    params.rate = request->rate;
    params.pitch = request->pitch;
    params.mode = request->mode;
    params.sex = request->sex;
    params.volume = request->volume;
    params.sample_frequency = request->sampfreq;
    params.mouths = request->mouths;
    sink.pcm = SinkPCM;
    sink.mouth = SinkMouth;
    sink.cancelled = SinkCancelled;
    sink.context = &context;
    sink.allocate = SinkAllocate;
    sink.release = SinkRelease;

    rc = SCSynthWith(base->nb_Resource.voice != NULL
            ? base->nb_Resource.voice : &SCDefaultVoice,
        (const uint8_t *)request->message.io_Data,
        request->message.io_Length, &params, &sink, &error_at);
    if (SinkFinishAudio(&context) != 0)
        rc = base->nb_Cancel ? SC_ERR_CANCELLED : SC_ERR_OUTPUT;
    request->message.io_Actual = rc == SC_ERR_BAD_PHONEME
        ? (ULONG)error_at : request->message.io_Length;
    if (context.in_flight[0])
    {
        AbortIO(&context.audio[0]->ioa_Request);
        WaitIO(&context.audio[0]->ioa_Request);
    }
    if (context.in_flight[1])
    {
        AbortIO(&context.audio[1]->ioa_Request);
        WaitIO(&context.audio[1]->ioa_Request);
    }
    if (rc == SC_ERR_BAD_PHONEME)
        error = ND_PhonErr;
    else if (rc == SC_ERR_BAD_PARAM)
        error = ND_MakeBad;
    else if (rc == SC_ERR_CANCELLED)
        error = IOERR_ABORTED;
    else if (rc != SC_OK)
        error = ND_NoAudLib;
    return error;
}

void NarratorWorker(struct NarratorBase *base)
{
    struct narrator_rb *request;

    for (;;)
    {
        WaitPort(base->nb_CommandPort);
        while ((request = GetRequest(base)) != NULL)
        {
            BYTE error = 0;
            ObtainSemaphore(&base->nb_Lock);
            error = (base->nb_ControlsPending != 0 || base->nb_Closing) &&
                request->message.io_Command != CMD_FLUSH &&
                request->message.io_Command != CMD_RESET &&
                request->message.io_Command != NARRATOR_RELEASE_AUDIO
                ? IOERR_ABORTED : 0;
            ReleaseSemaphore(&base->nb_Lock);
            if (error != 0)
            {
                if (request->message.io_Command == CMD_WRITE)
                    CompleteWrite(base);
                FinishRequest(base, request, IOERR_ABORTED);
                continue;
            }
            switch (request->message.io_Command)
            {
                case CMD_WRITE:
                    if (WaitUntilRunning(base))
                        error = IOERR_ABORTED;
                    else
                    {
                        error = Speak(request, base);
                    }
                    CompleteWrite(base);
                    break;
                case CMD_RESET:
                    ObtainSemaphore(&base->nb_Lock);
                    base->nb_Stopped = 0;
                    if (base->nb_ControlsPending != 0)
                        --base->nb_ControlsPending;
                    if (base->nb_ControlsPending == 0 && !base->nb_Closing)
                        base->nb_Cancel = 0;
                    ReleaseSemaphore(&base->nb_Lock);
                    break;
                case CMD_FLUSH:
                {
                    FinishReads(base, IOERR_ABORTED);
                    ObtainSemaphore(&base->nb_Lock);
                    if (base->nb_ControlsPending != 0)
                        --base->nb_ControlsPending;
                    if (base->nb_ControlsPending == 0 && !base->nb_Closing)
                        base->nb_Cancel = 0;
                    ReleaseSemaphore(&base->nb_Lock);
                    break;
                }
                case NARRATOR_RELEASE_AUDIO:
                    FinishReads(base, IOERR_ABORTED);
                    ReleaseAudio(base);
                    ObtainSemaphore(&base->nb_Lock);
                    base->nb_Closing = 0;
                    if (base->nb_ControlsPending == 0)
                        base->nb_Cancel = 0;
                    ReleaseSemaphore(&base->nb_Lock);
                    break;
                case CMD_UPDATE:
                case CMD_CLEAR:
                    break;
                default:
                    error = IOERR_NOCMD;
                    break;
            }
            FinishRequest(base, request, error);
        }
    }
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR base)
{
    NEWLIST(&base->nb_ReadQueue);
    InitSemaphore(&base->nb_Lock);
    base->nb_Current = NULL;
    base->nb_Cancel = 0;
    base->nb_Stopped = 0;
    base->nb_Closing = 0;
    base->nb_ControlsPending = 0;
    base->nb_WritesPending = 0;
    base->nb_AudioPort = NULL;
    base->nb_Audio[0] = base->nb_Audio[1] = NULL;
    base->nb_AudioBuffer[0] = base->nb_AudioBuffer[1] = NULL;
    base->nb_DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
    memset(&base->nb_Resource, 0, sizeof(base->nb_Resource));
    base->nb_ResourceMemory = LoadSpeechResource(base->nb_DOSBase,
                                                    RESOURCE_PATH,
                                                    &base->nb_Resource);
    base->nb_Task = NewCreateTask(TASKTAG_NAME, "narrator.device",
        TASKTAG_PRI, 0, TASKTAG_STACKSIZE, WORKER_STACK,
        TASKTAG_TASKMSGPORT, &base->nb_CommandPort,
        TASKTAG_PC, NarratorWorker, TASKTAG_ARG1, base, TAG_DONE);
    if (base->nb_Task == NULL)
    {
        if (base->nb_ResourceMemory != NULL)
            FreeVec(base->nb_ResourceMemory);
        base->nb_ResourceMemory = NULL;
        if (base->nb_DOSBase != NULL)
            CloseLibrary((struct Library *)base->nb_DOSBase);
        base->nb_DOSBase = NULL;
        return FALSE;
    }
    return TRUE;
}

static int GM_UNIQUENAME(Open)(LIBBASETYPEPTR base,
    struct IORequest *io, ULONG unit, ULONG flags)
{
    struct narrator_rb *request = (struct narrator_rb *)io;
    (void)flags;
    if (unit != 0 ||
        (io->io_Message.mn_Length != 0 &&
         io->io_Message.mn_Length < NARRATOR_PRE_V37_SIZE))
    {
        io->io_Error = IOERR_OPENFAIL;
        return FALSE;
    }
    io->io_Unit = (struct Unit *)base;
    io->io_Error = 0;
    request->rate = DEFRATE;
    request->pitch = DEFPITCH;
    request->mode = DEFMODE;
    request->sex = DEFSEX;
    request->ch_masks = NULL;
    request->nm_masks = 0;
    request->volume = DEFVOL;
    request->sampfreq = DEFFREQ;
    request->mouths = 0;
    request->chanmask = 0;
    request->numchan = 0;
    if ((request->flags & NDF_NEWIORB) &&
        io->io_Message.mn_Length >= sizeof(*request))
    {
        request->F0enthusiasm = DEFF0ENTHUS;
        request->F0perturb = DEFF0PERT;
        request->F1adj = request->F2adj = request->F3adj = 0;
        request->A1adj = request->A2adj = request->A3adj = 0;
        request->articulate = DEFARTIC;
        request->centralize = DEFCENTRAL;
        request->centphon = NULL;
        request->AVbias = request->AFbias = 0;
        request->priority = DEFPRIORITY;
    }
    return TRUE;
}

static int GM_UNIQUENAME(Close)(LIBBASETYPEPTR base,
                                struct IORequest *io)
{
    if (base->nb_Device.dd_Library.lib_OpenCnt == 1)
    {
        struct MsgPort *port = CreateMsgPort();
        struct IOStdReq *release = port != NULL
            ? (struct IOStdReq *)CreateIORequest(port, sizeof(*release))
            : NULL;

        if (release != NULL)
        {
            release->io_Command = NARRATOR_RELEASE_AUDIO;
            ObtainSemaphore(&base->nb_Lock);
            base->nb_Closing = 1;
            base->nb_Cancel = 1;
            PutMsg(base->nb_CommandPort, &release->io_Message);
            ReleaseSemaphore(&base->nb_Lock);
            Signal(base->nb_Task, 1UL << base->nb_CommandPort->mp_SigBit);
            WaitPort(port);
            GetMsg(port);
            DeleteIORequest((struct IORequest *)release);
        }
        if (port != NULL)
            DeleteMsgPort(port);
    }
    io->io_Unit = NULL;
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

AROS_LH1(void, beginio,
    AROS_LHA(struct narrator_rb *, request, A1),
    struct NarratorBase *, base, 5, Narrator)
{
    AROS_LIBFUNC_INIT
    request->message.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    request->message.io_Error = 0;
    request->message.io_Actual = 0;
    if (request->message.io_Command == NARRATOR_RELEASE_AUDIO)
    {
        request->message.io_Error = IOERR_NOCMD;
        if (!(request->message.io_Flags & IOF_QUICK))
            ReplyRequest(request, IOERR_NOCMD);
        return;
    }
    if (request->message.io_Command == NSCMD_DEVICEQUERY)
    {
        if (request->message.io_Data == NULL ||
            request->message.io_Length < sizeof(struct NSDeviceQueryResult))
            request->message.io_Error = IOERR_BADLENGTH;
        else
        {
            struct NSDeviceQueryResult *query = request->message.io_Data;
            query->DevQueryFormat = 0;
            query->SizeAvailable = sizeof(*query);
            query->DeviceType = NSDEVTYPE_UNKNOWN;
            query->DeviceSubType = 0;
            query->SupportedCommands = (UWORD *)SupportedCommands;
            request->message.io_Actual = sizeof(*query);
        }
        if (!(request->message.io_Flags & IOF_QUICK))
            ReplyRequest(request, request->message.io_Error);
        return;
    }
    request->message.io_Flags &= ~IOF_QUICK;

    if (request->message.io_Command == CMD_READ)
    {
        QueueRead(base, request);
        return;
    }
    if (request->message.io_Command == CMD_WRITE)
    {
        ObtainSemaphore(&base->nb_Lock);
        ++base->nb_WritesPending;
        ReleaseSemaphore(&base->nb_Lock);
    }

    /* STOP/START must affect an in-flight audio stream immediately.  They do
       not cancel it: the renderer sleeps on the worker port signal and
       resumes at the next bounded PCM block. */
    if (request->message.io_Command == CMD_STOP)
    {
        ObtainSemaphore(&base->nb_Lock);
        base->nb_Stopped = 1;
        ReleaseSemaphore(&base->nb_Lock);
        ReplyRequest(request, 0);
        return;
    }
    if (request->message.io_Command == CMD_START)
    {
        ObtainSemaphore(&base->nb_Lock);
        base->nb_Stopped = 0;
        ReleaseSemaphore(&base->nb_Lock);
        Signal(base->nb_Task, 1UL << base->nb_CommandPort->mp_SigBit);
        ReplyRequest(request, 0);
        return;
    }

    /* Cancellation must be observable while the worker is blocked in the
       renderer/audio loop, rather than only after it dequeues this request. */
    if (request->message.io_Command == CMD_RESET ||
        request->message.io_Command == CMD_FLUSH)
    {
        ObtainSemaphore(&base->nb_Lock);
        base->nb_Cancel = 1;
        ++base->nb_ControlsPending;
        PutMsg(base->nb_CommandPort, &request->message.io_Message);
        ReleaseSemaphore(&base->nb_Lock);
        Signal(base->nb_Task, 1UL << base->nb_CommandPort->mp_SigBit);
        return;
    }
    ObtainSemaphore(&base->nb_Lock);
    PutMsg(base->nb_CommandPort, &request->message.io_Message);
    ReleaseSemaphore(&base->nb_Lock);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
    AROS_LHA(struct narrator_rb *, request, A1),
    struct NarratorBase *, base, 6, Narrator)
{
    AROS_LIBFUNC_INIT
    BOOL removed = FALSE;
    BOOL aborted = FALSE;

    if (request->message.io_Command == CMD_READ)
    {
        ObtainSemaphore(&base->nb_Lock);
        if (NodeIsListed(&base->nb_ReadQueue,
                           &request->message.io_Message.mn_Node))
        {
            Remove(&request->message.io_Message.mn_Node);
            removed = TRUE;
            aborted = TRUE;
        }
        ReleaseSemaphore(&base->nb_Lock);
    }
    else
    {
        ObtainSemaphore(&base->nb_Lock);
        if (base->nb_Current == request)
        {
            base->nb_Cancel = 1;
            aborted = TRUE;
            Signal(base->nb_Task, 1UL << base->nb_CommandPort->mp_SigBit);
        }
        else if (NodeIsListed(&base->nb_CommandPort->mp_MsgList,
                                &request->message.io_Message.mn_Node))
        {
            Remove(&request->message.io_Message.mn_Node);
            removed = TRUE;
            aborted = TRUE;
        }
        ReleaseSemaphore(&base->nb_Lock);
        if (removed && request->message.io_Command == CMD_WRITE)
        {
            ObtainSemaphore(&base->nb_Lock);
            if (base->nb_WritesPending != 0)
                --base->nb_WritesPending;
            ReleaseSemaphore(&base->nb_Lock);
        }
        if (removed && (request->message.io_Command == CMD_RESET ||
                        request->message.io_Command == CMD_FLUSH))
        {
            ObtainSemaphore(&base->nb_Lock);
            if (base->nb_ControlsPending != 0)
                --base->nb_ControlsPending;
            if (base->nb_ControlsPending == 0 && !base->nb_Closing)
                base->nb_Cancel = 0;
            ReleaseSemaphore(&base->nb_Lock);
        }
    }
    if (removed)
        ReplyRequest(request, IOERR_ABORTED);
    return aborted ? 0 : -1;
    AROS_LIBFUNC_EXIT
}
