/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <devices/audio.h>
#include <devices/narrator.h>
#include <devices/newstyle.h>
#include <devices/speech.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <string.h>

#include "speech_intern.h"
#include <libraries/aros_resource.h>
#include <libraries/speechcore.h>

#include LC_LIBDEFS_FILE

#define SPEECH_STACK       AROS_STACKSIZE
#define SPEECH_MAX_INPUT   65535UL
#define PLAYBACK_BUFFER    8192
#define RESOURCE_PATH      "DEVS:Speech/speech.iff"
#define SPEECH_RELEASE_AUDIO 0xfffe

#if PLAYBACK_BUFFER % SC_PCM_BLOCK_SIZE != 0
#error PLAYBACK_BUFFER must contain complete speechcore blocks
#endif

static const UBYTE AudioMasks[] = { 3, 5, 10, 12 };
static const UWORD SupportedCommands[] = {
    CMD_RESET, CMD_WRITE, CMD_STOP, CMD_START, CMD_FLUSH,
    SDCMD_QUERY, NSCMD_DEVICEQUERY, 0
};
static CONST_STRPTR NarratorLanguages[] = { "en-US", NULL };
static CONST_STRPTR NarratorVoices[] = { "male", "female", NULL };
static CONST_STRPTR NarratorStyles[] = { "natural", "robotic", NULL };
static const struct SpeechEngineInfo NarratorInfo = {
    .sei_Size = sizeof(struct SpeechEngineInfo),
    .sei_Name = "narrator",
    .sei_DisplayName = "AROS Narrator",
    .sei_Languages = NarratorLanguages,
    .sei_Capabilities = SPEECHCAP_UTF8 | SPEECHCAP_PLAYBACK | SPEECHCAP_RATE |
        SPEECHCAP_PITCH | SPEECHCAP_VOLUME | SPEECHCAP_VOICE |
        SPEECHCAP_STYLE | SPEECHCAP_SAMPLEFREQ,
    .sei_Voices = NarratorVoices,
    .sei_Styles = NarratorStyles,
    .sei_MinSampleFrequency = MINFREQ,
    .sei_MaxSampleFrequency = MAXFREQ
};
static const struct SpeechEngineInfo *const SpeechEngines[] = {
    &NarratorInfo
};
static const struct SpeechDeviceQuery SpeechQuery = {
    .sdq_Size = sizeof(struct SpeechDeviceQuery),
    .sdq_APIVersion = SPEECH_API_VERSION,
    .sdq_EngineCount = 1,
    .sdq_Engines = SpeechEngines,
    .sdq_DefaultEngine = "narrator"
};

struct Playback
{
    struct SpeechBase *base;
    struct IOAudio *audio[2];
    UBYTE *audio_buffer[2];
    ULONG used[2];
    UBYTE in_flight[2];
    UBYTE next;
    UWORD period;
    UWORD volume;
};

static void *PlaybackAllocate(void *context, size_t bytes)
{
    (void)context;
    return AllocVec(bytes, MEMF_ANY);
}

static void PlaybackRelease(void *context, void *memory)
{
    (void)context;
    FreeVec(memory);
}

static IPTR TagValue(struct SpeechBase *base, const struct TagItem *tags,
                     Tag wanted, IPTR fallback)
{
    struct UtilityBase *UtilityBase = base->sb_UtilityBase;

    return GetTagData(wanted, fallback, tags);
}

static int PlaybackCancelled(void *context)
{
    struct Playback *playback = context;
    while (playback->base->sb_Stopped && !playback->base->sb_Cancel)
        Wait(1UL << playback->base->sb_CommandPort->mp_SigBit);
    return playback->base->sb_Cancel != 0;
}

static int PlaybackWait(struct Playback *playback, unsigned slot,
                         int cancellable)
{
    struct IOAudio *audio = playback->audio[slot];

    if (!playback->in_flight[slot])
        return 0;
    while (CheckIO(&audio->ioa_Request) == NULL)
    {
        Wait((1UL << audio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit) |
             (1UL << playback->base->sb_CommandPort->mp_SigBit));
        if (cancellable && playback->base->sb_Cancel)
        {
            AbortIO(&audio->ioa_Request);
            WaitIO(&audio->ioa_Request);
            playback->in_flight[slot] = 0;
            return 1;
        }
    }
    playback->in_flight[slot] = 0;
    return WaitIO(&audio->ioa_Request) == 0 ? 0 : 1;
}

static int PlaybackPCM(void *context, const int8_t *samples, size_t count,
                        uint16_t period, uint16_t volume)
{
    struct Playback *playback = context;
    unsigned slot = playback->next;
    struct IOAudio *audio = playback->audio[slot];

    if (PlaybackCancelled(context))
        return 1;
    if (count > PLAYBACK_BUFFER - playback->used[slot])
        return 1;
    if (playback->used[slot] == 0 &&
        PlaybackWait(playback, slot, 1) != 0)
        return 1;
    CopyMem((APTR)samples, playback->audio_buffer[slot] +
            playback->used[slot], count);
    playback->used[slot] += count;
    playback->period = period;
    playback->volume = volume;
    if (playback->used[slot] != PLAYBACK_BUFFER)
        return 0;
    audio->ioa_Request.io_Command = CMD_WRITE;
    audio->ioa_Request.io_Flags = ADIOF_PERVOL;
    audio->ioa_Data = playback->audio_buffer[slot];
    audio->ioa_Length = playback->used[slot];
    audio->ioa_Period = period;
    audio->ioa_Volume = volume;
    audio->ioa_Cycles = 1;
    /* SendIO() clears every I/O flag on AROS, including ADIOF_PERVOL.
       audio.device requires that flag for this request's period/volume. */
    BeginIO(&audio->ioa_Request);
    playback->in_flight[slot] = 1;
    playback->used[slot] = 0;
    playback->next ^= 1;
    return 0;
}

static int PlaybackFinish(struct Playback *playback)
{
    unsigned slot = playback->next;
    struct IOAudio *audio = playback->audio[slot];

    if (playback->used[slot] != 0)
    {
        if (PlaybackWait(playback, slot, 1) != 0)
            return 1;
        audio->ioa_Request.io_Command = CMD_WRITE;
        audio->ioa_Request.io_Flags = ADIOF_PERVOL;
        audio->ioa_Data = playback->audio_buffer[slot];
        audio->ioa_Length = playback->used[slot];
        audio->ioa_Period = playback->period;
        audio->ioa_Volume = playback->volume;
        audio->ioa_Cycles = 1;
        BeginIO(&audio->ioa_Request);
        playback->in_flight[slot] = 1;
        playback->used[slot] = 0;
    }
    if (PlaybackWait(playback, 0, 1) != 0)
        return 1;
    return PlaybackWait(playback, 1, 1);
}

static void ReleaseAudio(struct SpeechBase *base)
{
    if (base->sb_Audio[0] != NULL &&
        base->sb_Audio[0]->ioa_Request.io_Device != NULL)
        CloseDevice(&base->sb_Audio[0]->ioa_Request);
    if (base->sb_AudioBuffer[0] != NULL)
        FreeVec(base->sb_AudioBuffer[0]);
    if (base->sb_AudioBuffer[1] != NULL)
        FreeVec(base->sb_AudioBuffer[1]);
    if (base->sb_Audio[1] != NULL)
        DeleteIORequest(&base->sb_Audio[1]->ioa_Request);
    if (base->sb_Audio[0] != NULL)
        DeleteIORequest(&base->sb_Audio[0]->ioa_Request);
    if (base->sb_AudioPort != NULL)
        DeleteMsgPort(base->sb_AudioPort);
    base->sb_AudioBuffer[0] = base->sb_AudioBuffer[1] = NULL;
    base->sb_Audio[0] = base->sb_Audio[1] = NULL;
    base->sb_AudioPort = NULL;
}

static BYTE AcquireAudio(struct SpeechBase *base)
{
    if (base->sb_Audio[0] != NULL)
        return 0;
    base->sb_AudioPort = CreateMsgPort();
    if (base->sb_AudioPort == NULL)
        return IOERR_OPENFAIL;
    base->sb_Audio[0] = (struct IOAudio *)CreateIORequest(
        base->sb_AudioPort, sizeof(struct IOAudio));
    if (base->sb_Audio[0] == NULL)
        goto no_memory;
    base->sb_Audio[0]->ioa_Data = (UBYTE *)AudioMasks;
    base->sb_Audio[0]->ioa_Length = sizeof(AudioMasks);
    if (OpenDevice(AUDIONAME, 0, &base->sb_Audio[0]->ioa_Request, 0) != 0)
    {
        base->sb_Audio[0]->ioa_Request.io_Device = NULL;
        ReleaseAudio(base);
        return SDERR_BACKEND;
    }
    base->sb_Audio[0]->ioa_Request.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    base->sb_Audio[1] = (struct IOAudio *)CreateIORequest(
        base->sb_AudioPort, sizeof(struct IOAudio));
    if (base->sb_Audio[1] != NULL)
        CopyMem(base->sb_Audio[0], base->sb_Audio[1], sizeof(struct IOAudio));
    base->sb_AudioBuffer[0] = AllocVec(PLAYBACK_BUFFER, MEMF_CHIP);
    base->sb_AudioBuffer[1] = AllocVec(PLAYBACK_BUFFER, MEMF_CHIP);
    if (base->sb_Audio[1] == NULL || base->sb_AudioBuffer[0] == NULL ||
        base->sb_AudioBuffer[1] == NULL)
        goto no_memory;
    return 0;

no_memory:
    ReleaseAudio(base);
    return IOERR_OPENFAIL;
}

static long TranslateAlloc(const struct SCTranslatorData *translator,
                            const char *input, size_t input_length,
                            char **output)
{
    size_t maximum = input_length * 8U + 32U;
    size_t size = input_length * 2U + 32U;
    long result;

    *output = NULL;
    if (size > maximum)
        size = maximum;
    for (;;)
    {
        *output = AllocVec(size, MEMF_ANY);
        if (*output == NULL)
            return -1;
        result = SCTranslateWith(translator, input, input_length,
                                   *output, size);
        if (result == 0 || size == maximum)
            return result;
        FreeVec(*output);
        *output = NULL;
        size = size > maximum / 2U ? maximum : size * 2U;
    }
}

static BYTE NarratorSpeak(struct IOSpeech *request, struct SpeechBase *base)
{
    CONST_STRPTR engine = (CONST_STRPTR)TagValue(base, request->ios_Attrs,
        SPEECHA_Engine, (IPTR)"narrator");
    CONST_STRPTR language = (CONST_STRPTR)TagValue(base, request->ios_Attrs,
        SPEECHA_Language, (IPTR)"en-US");
    CONST_STRPTR voice_name = (CONST_STRPTR)TagValue(base, request->ios_Attrs,
        SPEECHA_Voice, (IPTR)"male");
    CONST_STRPTR style_name = (CONST_STRPTR)TagValue(base, request->ios_Attrs,
        SPEECHA_Style, (IPTR)"natural");
    ULONG rate = (ULONG)TagValue(base, request->ios_Attrs, SPEECHA_Rate,
                                 SPEECH_FIXED_ONE);
    LONG pitch = (LONG)TagValue(base, request->ios_Attrs, SPEECHA_Pitch, 0);
    ULONG volume = (ULONG)TagValue(base, request->ios_Attrs, SPEECHA_Volume,
                                   SPEECH_FIXED_ONE);
    ULONG sample_frequency = (ULONG)TagValue(base, request->ios_Attrs,
        SPEECHA_SampleFrequency, DEFFREQ);
    struct SCNarratorParams params;
    struct Playback playback;
    struct SCSink sink;
    char *phonemes = NULL;
    size_t error_at = 0;
    long translated;
    UQUAD scaled;
    int rc;
    BYTE result = 0;

    memset(&playback, 0, sizeof(playback));
    playback.base = base;

    if (engine == NULL || strcmp(engine, "narrator") != 0)
        return SDERR_NOENGINE;
    if (language == NULL || (strcmp(language, "en-US") != 0 &&
                             strcmp(language, "en") != 0))
        return SDERR_BADLANGUAGE;
    if (voice_name == NULL || (strcmp(voice_name, "male") != 0 &&
                               strcmp(voice_name, "female") != 0))
        return SDERR_BADVOICE;
    if (style_name == NULL || (strcmp(style_name, "natural") != 0 &&
                               strcmp(style_name, "robotic") != 0))
        return SDERR_BADSTYLE;
    if (sample_frequency < MINFREQ || sample_frequency > MAXFREQ)
        return SDERR_BADFREQUENCY;
    if (request->ios_Req.io_Length > SPEECH_MAX_INPUT ||
        request->ios_Req.io_Data == NULL)
        return IOERR_BADLENGTH;
    rc = SCValidateEnglishUtf8(request->ios_Req.io_Data,
                                  request->ios_Req.io_Length, &error_at);
    if (rc != SC_OK)
    {
        request->ios_Req.io_Actual = (ULONG)error_at;
        return rc == SC_ERR_BAD_UTF8 ? SDERR_BADUTF8 : SDERR_UNSUPPORTEDCHAR;
    }

    translated = TranslateAlloc(base->sb_Resource.translator != NULL
            ? base->sb_Resource.translator : &SCDefaultTranslator,
        request->ios_Req.io_Data,
        request->ios_Req.io_Length, &phonemes);
    if (phonemes == NULL)
        return IOERR_OPENFAIL;
    if (translated != 0)
    {
        result = SDERR_BACKEND;
        goto out;
    }

    result = AcquireAudio(base);
    if (result != 0)
        goto out;
    playback.audio[0] = base->sb_Audio[0];
    playback.audio[1] = base->sb_Audio[1];
    playback.audio_buffer[0] = base->sb_AudioBuffer[0];
    playback.audio_buffer[1] = base->sb_AudioBuffer[1];

    SCDefaultParams(&params);
    params.sex = strcmp(voice_name, "female") == 0;
    params.mode = strcmp(style_name, "robotic") == 0;
    params.sample_frequency = (UWORD)sample_frequency;
    scaled = ((UQUAD)DEFRATE * rate) >> 16;
    params.rate = (UWORD)(scaled < MINRATE ? MINRATE
                          : scaled > MAXRATE ? MAXRATE : scaled);
    scaled = ((UQUAD)DEFPITCH * SCSemitoneRatio(pitch)) >> 16;
    params.pitch = (UWORD)(scaled < MINPITCH ? MINPITCH
                           : scaled > MAXPITCH ? MAXPITCH : scaled);
    scaled = ((UQUAD)MAXVOL * volume) >> 16;
    params.volume = (UWORD)(scaled > MAXVOL ? MAXVOL : scaled);
    sink.pcm = PlaybackPCM;
    sink.mouth = NULL;
    sink.cancelled = PlaybackCancelled;
    sink.context = &playback;
    sink.allocate = PlaybackAllocate;
    sink.release = PlaybackRelease;
    rc = SCSynthWith(base->sb_Resource.voice != NULL
            ? base->sb_Resource.voice : &SCDefaultVoice,
        (const uint8_t *)phonemes, strlen(phonemes),
        &params, &sink, &error_at);
    if (PlaybackFinish(&playback) != 0)
        rc = base->sb_Cancel ? SC_ERR_CANCELLED : SC_ERR_OUTPUT;
    if (rc == SC_ERR_CANCELLED)
        result = IOERR_ABORTED;
    else if (rc != SC_OK)
        result = SDERR_BACKEND;
    else
        request->ios_Req.io_Actual = request->ios_Req.io_Length;

out:
    if (playback.in_flight[0])
    {
        AbortIO(&playback.audio[0]->ioa_Request);
        WaitIO(&playback.audio[0]->ioa_Request);
    }
    if (playback.in_flight[1])
    {
        AbortIO(&playback.audio[1]->ioa_Request);
        WaitIO(&playback.audio[1]->ioa_Request);
    }
    if (phonemes != NULL)
        FreeVec(phonemes);
    return result;
}

static void ReplyRequest(struct IOSpeech *request, BYTE error)
{
    request->ios_Req.io_Error = error;
    ReplyMsg(&request->ios_Req.io_Message);
}

static BOOL RequestIsQueued(struct MsgPort *port, struct Message *message)
{
    struct Node *node;

    for (node = port->mp_MsgList.lh_Head; node->ln_Succ != NULL;
         node = node->ln_Succ)
        if (node == &message->mn_Node)
            return TRUE;
    return FALSE;
}

static struct IOSpeech *GetRequest(struct SpeechBase *base)
{
    struct IOSpeech *request;

    ObtainSemaphore(&base->sb_Lock);
    request = (struct IOSpeech *)GetMsg(base->sb_CommandPort);
    if (request != NULL && request->ios_Req.io_Command == CMD_WRITE)
    {
        /* Publish ownership atomically with dequeueing.  AbortIO must never
           mistake a detached message for one that is still on the port. */
        if (base->sb_ControlsPending == 0 && !base->sb_Closing)
            base->sb_Cancel = 0;
        base->sb_Current = request;
    }
    ReleaseSemaphore(&base->sb_Lock);
    return request;
}

static void FinishRequest(struct SpeechBase *base,
                           struct IOSpeech *request, BYTE error)
{
    ObtainSemaphore(&base->sb_Lock);
    if (base->sb_Current == request)
        base->sb_Current = NULL;
    ReleaseSemaphore(&base->sb_Lock);
    ReplyRequest(request, error);
}

void SpeechWorker(struct SpeechBase *base)
{
    struct IOSpeech *request;
    for (;;)
    {
        WaitPort(base->sb_CommandPort);
        while ((request = GetRequest(base)) != NULL)
        {
            BYTE error = 0;
            ObtainSemaphore(&base->sb_Lock);
            error = (base->sb_ControlsPending != 0 || base->sb_Closing) &&
                request->ios_Req.io_Command != CMD_FLUSH &&
                request->ios_Req.io_Command != CMD_RESET &&
                request->ios_Req.io_Command != SPEECH_RELEASE_AUDIO
                ? IOERR_ABORTED : 0;
            ReleaseSemaphore(&base->sb_Lock);
            if (error != 0)
            {
                FinishRequest(base, request, IOERR_ABORTED);
                continue;
            }
            switch (request->ios_Req.io_Command)
            {
                case CMD_WRITE:
                    if (request->ios_APIVersion != SPEECH_API_VERSION)
                        error = SDERR_BADVERSION;
                    else
                    {
                        while (base->sb_Stopped && !base->sb_Cancel)
                            Wait(1UL << base->sb_CommandPort->mp_SigBit);
                        if (base->sb_Cancel)
                            error = IOERR_ABORTED;
                        else
                            error = NarratorSpeak(request, base);
                    }
                    break;
                case SDCMD_QUERY:
                    if (request->ios_Req.io_Data == NULL ||
                        request->ios_Req.io_Length < sizeof(SpeechQuery))
                        error = IOERR_BADLENGTH;
                    else
                    {
                        CopyMem((APTR)&SpeechQuery, request->ios_Req.io_Data,
                                sizeof(SpeechQuery));
                        request->ios_Req.io_Actual = sizeof(SpeechQuery);
                    }
                    break;
                case CMD_RESET:
                    ObtainSemaphore(&base->sb_Lock);
                    base->sb_Stopped = 0;
                    if (base->sb_ControlsPending != 0)
                        --base->sb_ControlsPending;
                    if (base->sb_ControlsPending == 0 && !base->sb_Closing)
                        base->sb_Cancel = 0;
                    ReleaseSemaphore(&base->sb_Lock);
                    break;
                case CMD_FLUSH:
                    ObtainSemaphore(&base->sb_Lock);
                    if (base->sb_ControlsPending != 0)
                        --base->sb_ControlsPending;
                    if (base->sb_ControlsPending == 0 && !base->sb_Closing)
                        base->sb_Cancel = 0;
                    ReleaseSemaphore(&base->sb_Lock);
                    break;
                case SPEECH_RELEASE_AUDIO:
                    ReleaseAudio(base);
                    ObtainSemaphore(&base->sb_Lock);
                    base->sb_Closing = 0;
                    if (base->sb_ControlsPending == 0)
                        base->sb_Cancel = 0;
                    ReleaseSemaphore(&base->sb_Lock);
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
    base->sb_Current = NULL;
    base->sb_Cancel = 0;
    base->sb_Stopped = 0;
    InitSemaphore(&base->sb_Lock);
    base->sb_Closing = 0;
    base->sb_ControlsPending = 0;
    base->sb_AudioPort = NULL;
    base->sb_Audio[0] = base->sb_Audio[1] = NULL;
    base->sb_AudioBuffer[0] = base->sb_AudioBuffer[1] = NULL;
    base->sb_DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
    base->sb_UtilityBase = (struct UtilityBase *)OpenLibrary(
        "utility.library", 0);
    if (base->sb_UtilityBase == NULL)
    {
        if (base->sb_DOSBase != NULL)
            CloseLibrary((struct Library *)base->sb_DOSBase);
        return FALSE;
    }
    memset(&base->sb_Resource, 0, sizeof(base->sb_Resource));
    base->sb_ResourceMemory = LoadSpeechResource(base->sb_DOSBase,
                                                    RESOURCE_PATH,
                                                    &base->sb_Resource);
    base->sb_Task = NewCreateTask(TASKTAG_NAME, "speech.device",
        TASKTAG_PRI, 0, TASKTAG_STACKSIZE, SPEECH_STACK,
        TASKTAG_TASKMSGPORT, &base->sb_CommandPort,
        TASKTAG_PC, SpeechWorker, TASKTAG_ARG1, base, TAG_DONE);
    if (base->sb_Task == NULL)
    {
        if (base->sb_ResourceMemory != NULL)
            FreeVec(base->sb_ResourceMemory);
        base->sb_ResourceMemory = NULL;
        if (base->sb_DOSBase != NULL)
            CloseLibrary((struct Library *)base->sb_DOSBase);
        CloseLibrary((struct Library *)base->sb_UtilityBase);
        base->sb_DOSBase = NULL;
        base->sb_UtilityBase = NULL;
        return FALSE;
    }
    return TRUE;
}

static int GM_UNIQUENAME(Open)(LIBBASETYPEPTR base,
    struct IORequest *io, ULONG unit, ULONG flags)
{
    (void)flags;
    if (unit != 0 || io->io_Message.mn_Length < sizeof(struct IOSpeech))
    {
        io->io_Error = IOERR_OPENFAIL;
        return FALSE;
    }
    io->io_Unit = (struct Unit *)base;
    ((struct IOSpeech *)io)->ios_Reserved = 0;
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)

static int GM_UNIQUENAME(Close)(LIBBASETYPEPTR base,
                                struct IORequest *io)
{
    if (base->sb_Device.dd_Library.lib_OpenCnt == 1)
    {
        struct MsgPort *port = CreateMsgPort();
        struct IOStdReq *release = port != NULL
            ? (struct IOStdReq *)CreateIORequest(port, sizeof(*release))
            : NULL;

        if (release != NULL)
        {
            release->io_Command = SPEECH_RELEASE_AUDIO;
            ObtainSemaphore(&base->sb_Lock);
            base->sb_Closing = 1;
            base->sb_Cancel = 1;
            PutMsg(base->sb_CommandPort, &release->io_Message);
            ReleaseSemaphore(&base->sb_Lock);
            Signal(base->sb_Task, 1UL << base->sb_CommandPort->mp_SigBit);
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

ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

AROS_LH1(void, beginio,
    AROS_LHA(struct IOSpeech *, request, A1),
    struct SpeechBase *, base, 5, Speech)
{
    AROS_LIBFUNC_INIT
    request->ios_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    request->ios_Req.io_Error = 0;
    request->ios_Req.io_Actual = 0;
    if (request->ios_Req.io_Command == SPEECH_RELEASE_AUDIO)
    {
        request->ios_Req.io_Error = IOERR_NOCMD;
        if (!(request->ios_Req.io_Flags & IOF_QUICK))
            ReplyRequest(request, IOERR_NOCMD);
        return;
    }
    if (request->ios_Req.io_Command == NSCMD_DEVICEQUERY)
    {
        if (request->ios_Req.io_Data == NULL ||
            request->ios_Req.io_Length < sizeof(struct NSDeviceQueryResult))
            request->ios_Req.io_Error = IOERR_BADLENGTH;
        else
        {
            struct NSDeviceQueryResult *query = request->ios_Req.io_Data;
            query->DevQueryFormat = 0;
            query->SizeAvailable = sizeof(*query);
            query->DeviceType = NSDEVTYPE_UNKNOWN;
            query->DeviceSubType = 0;
            query->SupportedCommands = (UWORD *)SupportedCommands;
            request->ios_Req.io_Actual = sizeof(*query);
        }
        if (!(request->ios_Req.io_Flags & IOF_QUICK))
            ReplyRequest(request, request->ios_Req.io_Error);
        return;
    }
    request->ios_Req.io_Flags &= ~IOF_QUICK;

    if (request->ios_Req.io_Command == CMD_STOP)
    {
        ObtainSemaphore(&base->sb_Lock);
        base->sb_Stopped = 1;
        ReleaseSemaphore(&base->sb_Lock);
        ReplyRequest(request, 0);
        return;
    }
    if (request->ios_Req.io_Command == CMD_START)
    {
        ObtainSemaphore(&base->sb_Lock);
        base->sb_Stopped = 0;
        ReleaseSemaphore(&base->sb_Lock);
        Signal(base->sb_Task, 1UL << base->sb_CommandPort->mp_SigBit);
        ReplyRequest(request, 0);
        return;
    }

    /* These flags must reach a busy renderer before its worker dequeues the
       control request. The worker still owns completion and queue ordering. */
    if (request->ios_Req.io_Command == CMD_RESET ||
        request->ios_Req.io_Command == CMD_FLUSH)
    {
        ObtainSemaphore(&base->sb_Lock);
        base->sb_Cancel = 1;
        ++base->sb_ControlsPending;
        PutMsg(base->sb_CommandPort, &request->ios_Req.io_Message);
        ReleaseSemaphore(&base->sb_Lock);
        Signal(base->sb_Task, 1UL << base->sb_CommandPort->mp_SigBit);
        return;
    }
    ObtainSemaphore(&base->sb_Lock);
    PutMsg(base->sb_CommandPort, &request->ios_Req.io_Message);
    ReleaseSemaphore(&base->sb_Lock);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
    AROS_LHA(struct IOSpeech *, request, A1),
    struct SpeechBase *, base, 6, Speech)
{
    AROS_LIBFUNC_INIT
    BOOL aborted = FALSE;
    BOOL removed = FALSE;

    ObtainSemaphore(&base->sb_Lock);
    if (base->sb_Current == request)
    {
        base->sb_Cancel = 1;
        aborted = TRUE;
        Signal(base->sb_Task, 1UL << base->sb_CommandPort->mp_SigBit);
    }
    else if (RequestIsQueued(base->sb_CommandPort,
                               &request->ios_Req.io_Message))
    {
        Remove(&request->ios_Req.io_Message.mn_Node);
        aborted = TRUE;
        removed = TRUE;
    }
    if (removed &&
        (request->ios_Req.io_Command == CMD_RESET ||
         request->ios_Req.io_Command == CMD_FLUSH))
    {
        if (base->sb_ControlsPending != 0)
            --base->sb_ControlsPending;
        if (base->sb_ControlsPending == 0 && !base->sb_Closing)
            base->sb_Cancel = 0;
    }
    ReleaseSemaphore(&base->sb_Lock);
    if (removed)
        ReplyRequest(request, IOERR_ABORTED);
    return aborted ? 0 : -1;
    AROS_LIBFUNC_EXIT
}
