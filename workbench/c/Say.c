/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    NAME
        Say

    SYNOPSIS
        RATE/K/N,PITCH/K/N,FREQUENCY/K/N,VOLUME/K/N,MALE/S,FEMALE/S,
        NATURAL/S,ROBOTIC/S,PHONEMES/S,OUT/K,FROM/K,TEXT/F

    FUNCTION
        Speaks English text or narrator phonemes.
*/

#include <devices/audio.h>
#include <devices/narrator.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/translator.h>

#include <string.h>

#include "SayArgs.h"
#include <libraries/aros_resource.h>
#include <libraries/speechcore.h>

#define TEXT_BUFFER_SIZE 512
#define ARG_TEMPLATE "RATE/K/N,PITCH/K/N,FREQUENCY/K/N,VOLUME/K/N," \
    "MALE/S,FEMALE/S,NATURAL/S,ROBOTIC/S,PHONEMES/S,OUT/K,FROM/K,TEXT/F"

enum
{
    ARG_RATE,
    ARG_PITCH,
    ARG_FREQUENCY,
    ARG_VOLUME,
    ARG_MALE,
    ARG_FEMALE,
    ARG_NATURAL,
    ARG_ROBOTIC,
    ARG_PHONEMES,
    ARG_OUT,
    ARG_FROM,
    ARG_TEXT,
    ARG_COUNT
};

const TEXT version[] = "$VER: Say 50.1 (05.08.2026)";
struct Library *TranslatorBase;

static const UBYTE ChannelMasks[] = { 3, 5, 10, 12 };

struct WavSink
{
    BPTR file;
    ULONG samples;
    UBYTE converted[SC_PCM_BLOCK_SIZE];
};

struct SayOutput
{
    struct SCNarratorParams params;
    const struct SCVoiceData *voice;
    struct WavSink wav;
    struct MsgPort *port;
    struct narrator_rb *request;
    BOOL write_wav;
};

struct TextStream
{
    struct SayOutput *output;
    UBYTE text[TEXT_BUFFER_SIZE];
    ULONG length;
    BOOL phoneme_input;
    BOOL sentence_end;
};

static void Usage(void)
{
    PutStr("Usage: Say [-mfrn] [-s rate] [-p pitch] [-x file] [options] text...\n"
           "  -m/-f, --male/--female\n"
           "                        select voice\n"
           "  -r/-n, --robotic/--natural\n"
           "                        select style\n"
           "  -s, --rate N          speaking rate (40..400)\n"
           "  -p, --pitch N         pitch in Hz (65..320)\n"
           "  -x, --input FILE      read input from FILE\n"
           "  --sample-rate N       sample frequency (5000..28000 Hz)\n"
           "  --volume N            volume (0..64)\n"
           "  --phoneme-input       input is narrator phonemes\n"
           "  --wav FILE            write an 8-bit mono WAV file\n"
           "  -h, -?, --help        show this help\n"
           "  --                    end option processing\n");
}

static void *WavAllocate(void *context, size_t bytes)
{
    (void)context;
    return AllocVec(bytes, MEMF_ANY);
}

static void WavRelease(void *context, void *memory)
{
    (void)context;
    FreeVec(memory);
}

static void Put16(UBYTE *p, ULONG v)
{
    p[0] = v & 0xff;
    p[1] = (v >> 8) & 0xff;
}

static void Put32(UBYTE *p, ULONG v)
{
    p[0] = v & 0xff;
    p[1] = (v >> 8) & 0xff;
    p[2] = (v >> 16) & 0xff;
    p[3] = (v >> 24) & 0xff;
}

static void WavHeader(UBYTE header[44], ULONG samples, ULONG rate)
{
    memset(header, 0, 44);
    CopyMem("RIFF", header, 4);
    Put32(header + 4, 36 + samples);
    CopyMem("WAVEfmt ", header + 8, 8);
    Put32(header + 16, 16);
    Put16(header + 20, 1);
    Put16(header + 22, 1);
    Put32(header + 24, rate);
    Put32(header + 28, rate);
    Put16(header + 32, 1);
    Put16(header + 34, 8);
    CopyMem("data", header + 36, 4);
    Put32(header + 40, samples);
}

static ULONG ActualSampleRate(UWORD sample_frequency)
{
    ULONG period = 0x369c78UL / sample_frequency;

    return (3546895UL + period / 2) / period;
}

static int WavPCM(void *context, const int8_t *samples, size_t count,
                  uint16_t period, uint16_t volume)
{
    struct WavSink *sink = context;
    size_t i;

    if (count > sizeof(sink->converted))
        return 1;
    for (i = 0; i < count; ++i)
        sink->converted[i] =
            (UBYTE)(((int)samples[i] * (int)volume) / MAXVOL + 128);
    if (Write(sink->file, sink->converted, count) != (LONG)count)
        return 1;
    sink->samples += count;
    (void)period;
    return 0;
}

static int WavCancelled(void *context)
{
    (void)context;
    return CheckSignal(SIGBREAKF_CTRL_C) != 0;
}

static LONG BeginWav(struct SayOutput *output, CONST_STRPTR path)
{
    UBYTE header[44];

    output->wav.file = Open(path, MODE_NEWFILE);
    output->wav.samples = 0;
    if (output->wav.file == BNULL)
    {
        PrintFault(IoErr(), path);
        return RETURN_FAIL;
    }
    WavHeader(header, 0, ActualSampleRate(output->params.sample_frequency));
    if (Write(output->wav.file, header, sizeof(header)) != sizeof(header))
    {
        PrintFault(IoErr(), path);
        Close(output->wav.file);
        output->wav.file = BNULL;
        return RETURN_FAIL;
    }
    output->write_wav = TRUE;
    return RETURN_OK;
}

static void EndWav(struct SayOutput *output)
{
    UBYTE header[44];

    if (!output->write_wav)
        return;
    WavHeader(header, output->wav.samples,
              ActualSampleRate(output->params.sample_frequency));
    if (Seek(output->wav.file, 0, OFFSET_BEGINNING) != -1)
        Write(output->wav.file, header, sizeof(header));
    Close(output->wav.file);
    output->wav.file = BNULL;
    output->write_wav = FALSE;
}

static LONG WriteWav(struct SayOutput *output, CONST_STRPTR phonemes)
{
    struct SCSink sink = { WavPCM, NULL, WavCancelled, &output->wav,
                           WavAllocate, WavRelease };
    size_t error = 0;
    int result;

    result = SCSynthWith(output->voice, (const uint8_t *)phonemes,
                         strlen(phonemes), &output->params, &sink, &error);
    if (result == SC_OK)
        return RETURN_OK;
    if (result == SC_ERR_CANCELLED)
    {
        PrintFault(ERROR_BREAK, NULL);
        return RETURN_WARN;
    }
    Printf("Say: synthesis failed at byte %lu (error %ld)\n",
           (ULONG)error, (LONG)result);
    return RETURN_FAIL;
}

static LONG BeginPlayback(struct SayOutput *output)
{
    output->port = CreateMsgPort();
    if (output->port == NULL)
        return RETURN_FAIL;
    output->request = (struct narrator_rb *)CreateIORequest(
        output->port, sizeof(*output->request));
    if (output->request == NULL)
    {
        DeleteMsgPort(output->port);
        output->port = NULL;
        return RETURN_FAIL;
    }
    if (OpenDevice(NARRATORNAME, 0,
                   (struct IORequest *)&output->request->message, 0) != 0)
    {
        Printf("Say: cannot open %s\n", NARRATORNAME);
        DeleteIORequest((struct IORequest *)&output->request->message);
        DeleteMsgPort(output->port);
        output->request = NULL;
        output->port = NULL;
        return RETURN_FAIL;
    }
    output->request->ch_masks = (UBYTE *)ChannelMasks;
    output->request->nm_masks = sizeof(ChannelMasks);
    output->request->rate = output->params.rate;
    output->request->pitch = output->params.pitch;
    output->request->mode = output->params.mode;
    output->request->sex = output->params.sex;
    output->request->volume = output->params.volume;
    output->request->sampfreq = output->params.sample_frequency;
    return RETURN_OK;
}

static void EndPlayback(struct SayOutput *output)
{
    if (output->request == NULL)
        return;
    CloseDevice((struct IORequest *)&output->request->message);
    DeleteIORequest((struct IORequest *)&output->request->message);
    DeleteMsgPort(output->port);
    output->request = NULL;
    output->port = NULL;
}

static LONG Play(struct SayOutput *output, CONST_STRPTR phonemes)
{
    struct narrator_rb *request = output->request;
    ULONG signals;

    request->message.io_Command = CMD_WRITE;
    request->message.io_Data = (APTR)phonemes;
    request->message.io_Length = strlen(phonemes);
    request->message.io_Actual = 0;
    request->message.io_Error = 0;
    SendIO((struct IORequest *)&request->message);
    signals = Wait((1UL << output->port->mp_SigBit) | SIGBREAKF_CTRL_C);
    if (signals & SIGBREAKF_CTRL_C)
    {
        AbortIO((struct IORequest *)&request->message);
        PrintFault(ERROR_BREAK, NULL);
    }
    WaitIO((struct IORequest *)&request->message);
    if (request->message.io_Error == 0)
        return RETURN_OK;
    if (request->message.io_Error != IOERR_ABORTED)
    {
        Printf("Say: narrator error %ld at byte %lu\n",
               (LONG)request->message.io_Error, request->message.io_Actual);
    }
    return request->message.io_Error == IOERR_ABORTED
        ? RETURN_WARN : RETURN_FAIL;
}

static LONG OutputPhonemes(struct SayOutput *output, CONST_STRPTR phonemes)
{
    if (output->write_wav)
        return WriteWav(output, phonemes);
    return Play(output, phonemes);
}

static LONG SpeakSegment(struct SayOutput *output, const UBYTE *text,
                         ULONG length, BOOL phoneme_input)
{
    ULONG begin = 0;
    ULONG end = length;
    ULONG output_size;
    STRPTR phonemes;
    LONG result;

    while (begin < end && (text[begin] == ' ' || text[begin] == '\t' ||
                           text[begin] == '\r' || text[begin] == '\n'))
        ++begin;
    while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t' ||
                           text[end - 1] == '\r' || text[end - 1] == '\n'))
        --end;
    if (begin == end)
        return RETURN_OK;
    output_size = phoneme_input ? end - begin + 1 : (end - begin) * 8 + 32;
    phonemes = AllocVec(output_size, MEMF_ANY);
    if (phonemes == NULL)
        return RETURN_FAIL;
    if (phoneme_input)
    {
        CopyMem((APTR)(text + begin), phonemes, end - begin);
        phonemes[end - begin] = 0;
    }
    else if (Translate((CONST_STRPTR)text + begin, end - begin,
                       phonemes, output_size) != 0)
    {
        Printf("Say: translation failed\n");
        FreeVec(phonemes);
        return RETURN_FAIL;
    }
    result = OutputPhonemes(output, phonemes);
    FreeVec(phonemes);
    return result;
}

static LONG FlushStream(struct TextStream *stream, ULONG count)
{
    LONG result;

    if (count == 0)
        return RETURN_OK;
    result = SpeakSegment(stream->output, stream->text, count,
                          stream->phoneme_input);
    if (result != RETURN_OK)
        return result;
    if (count < stream->length)
        memmove(stream->text, stream->text + count, stream->length - count);
    stream->length -= count;
    stream->sentence_end = FALSE;
    return RETURN_OK;
}

static LONG MakeStreamRoom(struct TextStream *stream)
{
    ULONG split;

    if (stream->length < sizeof(stream->text))
        return RETURN_OK;
    split = stream->length;
    while (split > 0 && stream->text[split - 1] != ' ' &&
           stream->text[split - 1] != '\t' &&
           stream->text[split - 1] != '\r' &&
           stream->text[split - 1] != '\n')
        --split;
    if (split == 0)
        split = stream->length;
    return FlushStream(stream, split);
}

static BOOL ClosingPunctuation(UBYTE byte)
{
    return byte == '\'' || byte == '"' || byte == ')' || byte == ']' ||
           byte == '}';
}

static BOOL WhiteSpace(UBYTE byte)
{
    return byte == ' ' || byte == '\t' || byte == '\r' || byte == '\n';
}

static LONG FeedByte(struct TextStream *stream, UBYTE byte)
{
    LONG result;

    if (stream->sentence_end && WhiteSpace(byte))
    {
        result = MakeStreamRoom(stream);
        if (result != RETURN_OK)
            return result;
        stream->text[stream->length++] = byte;
        return FlushStream(stream, stream->length);
    }
    if (stream->sentence_end && !ClosingPunctuation(byte))
        stream->sentence_end = FALSE;
    result = MakeStreamRoom(stream);
    if (result != RETURN_OK)
        return result;
    stream->text[stream->length++] = byte;
    if (byte == '.' || byte == '?' || byte == '!')
        stream->sentence_end = TRUE;
    return RETURN_OK;
}

static LONG FeedWords(struct TextStream *stream, const char **words,
                      size_t count)
{
    size_t word;

    for (word = 0; word < count; ++word)
    {
        const UBYTE *at = (const UBYTE *)words[word];

        if (word != 0 && FeedByte(stream, ' ') != RETURN_OK)
            return RETURN_FAIL;
        while (*at != 0)
        {
            if (FeedByte(stream, *at++) != RETURN_OK)
                return RETURN_FAIL;
        }
    }
    return FlushStream(stream, stream->length);
}

static LONG FeedFile(struct TextStream *stream, CONST_STRPTR path)
{
    UBYTE *buffer;
    BPTR file;
    LONG count;
    LONG result = RETURN_OK;

    buffer = AllocVec(512, MEMF_ANY);
    if (buffer == NULL)
        return RETURN_FAIL;
    file = Open(path, MODE_OLDFILE);
    if (file == BNULL)
    {
        PrintFault(IoErr(), path);
        FreeVec(buffer);
        return RETURN_FAIL;
    }
    while ((count = Read(file, buffer, 512)) > 0)
    {
        LONG at;

        if (CheckSignal(SIGBREAKF_CTRL_C))
        {
            PrintFault(ERROR_BREAK, NULL);
            result = RETURN_WARN;
            break;
        }

        for (at = 0; at < count; ++at)
        {
            result = FeedByte(stream, buffer[at]);
            if (result != RETURN_OK)
                break;
        }
        if (result != RETURN_OK)
            break;
    }
    if (count < 0)
    {
        PrintFault(IoErr(), path);
        result = RETURN_FAIL;
    }
    if (result == RETURN_OK)
        result = FlushStream(stream, stream->length);
    Close(file);
    FreeVec(buffer);
    return result;
}

static LONG ApplyArguments(const struct SayArguments *arguments,
                           struct SCNarratorParams *params)
{
    if (arguments->have_rate)
    {
        if (arguments->rate < MINRATE || arguments->rate > MAXRATE)
        {
            Printf("Say: rate must be %ld..%ld\n", (LONG)MINRATE,
                   (LONG)MAXRATE);
            return RETURN_ERROR;
        }
        params->rate = (UWORD)arguments->rate;
    }
    if (arguments->have_pitch)
    {
        if (arguments->pitch < MINPITCH || arguments->pitch > MAXPITCH)
        {
            Printf("Say: pitch must be %ld..%ld\n", (LONG)MINPITCH,
                   (LONG)MAXPITCH);
            return RETURN_ERROR;
        }
        params->pitch = (UWORD)arguments->pitch;
    }
    if (arguments->have_sample_frequency)
    {
        if (arguments->sample_frequency < MINFREQ ||
            arguments->sample_frequency > MAXFREQ)
        {
            Printf("Say: sample rate must be %ld..%ld\n", (LONG)MINFREQ,
                   (LONG)MAXFREQ);
            return RETURN_ERROR;
        }
        params->sample_frequency = (UWORD)arguments->sample_frequency;
    }
    if (arguments->have_volume)
    {
        if (arguments->volume > MAXVOL)
        {
            Printf("Say: volume must be 0..%ld\n", (LONG)MAXVOL);
            return RETURN_ERROR;
        }
        params->volume = (UWORD)arguments->volume;
    }
    if (arguments->sex >= 0)
        params->sex = (UWORD)arguments->sex;
    if (arguments->mode >= 0)
        params->mode = (UWORD)arguments->mode;
    return RETURN_OK;
}

static int Tokenize(CONST_STRPTR text, char ***argv_out, char **storage_out,
                    int *argc_out)
{
    size_t length = text != NULL ? strlen(text) : 0;
    size_t capacity;
    char **items;
    char *storage;

    if (length > (size_t)-1 - 2 ||
        length + 2 > (size_t)-1 / sizeof(*items))
        return 0;
    if (text == NULL)
        text = "";
    capacity = length + 2;
    items = AllocVec(capacity * sizeof(*items), MEMF_ANY);
    storage = AllocVec(length + 1, MEMF_ANY);

    if (items == NULL || storage == NULL)
    {
        if (items != NULL)
            FreeVec(items);
        if (storage != NULL)
            FreeVec(storage);
        return 0;
    }
    if (!SayTokenize(text, items, capacity, storage, length + 1, argc_out))
    {
        FreeVec(storage);
        FreeVec(items);
        return 0;
    }
    *argv_out = items;
    *storage_out = storage;
    return 1;
}

int main(int argc, char **argv)
{
    struct SayArguments arguments;
    struct SayOutput *output = NULL;
    struct TextStream *stream = NULL;
    struct SCResource resource = { 0 };
    APTR resource_memory = NULL;
    const char **words = NULL;
    char **parsed_argv = NULL;
    char *argument_storage = NULL;
    struct RDArgs *rdargs = NULL;
    IPTR readargs[ARG_COUNT] = { 0 };
    int parsed_argc = 0;
    char error[128];
    LONG result;

    if (argc == 0)
        return RETURN_ERROR;
    (void)argv;
    rdargs = ReadArgs(ARG_TEMPLATE, readargs, NULL);
    if (rdargs == NULL)
    {
        PrintFault(IoErr(), "Say");
        return RETURN_FAIL;
    }
    if (!Tokenize((CONST_STRPTR)readargs[ARG_TEXT], &parsed_argv,
                  &argument_storage, &parsed_argc))
    {
        Printf("Say: could not parse command text\n");
        result = RETURN_FAIL;
        goto out;
    }
    words = AllocVec((parsed_argc > 1 ? parsed_argc - 1 : 1) *
                     sizeof(*words), MEMF_ANY);
    if (words == NULL)
    {
        result = RETURN_FAIL;
        goto out;
    }
    SayInitArguments(&arguments, words,
                     parsed_argc > 1 ? parsed_argc - 1 : 1);
    if (readargs[ARG_MALE] && readargs[ARG_FEMALE])
    {
        Printf("Say: MALE and FEMALE are mutually exclusive\n");
        result = RETURN_ERROR;
        goto out;
    }
    if (readargs[ARG_NATURAL] && readargs[ARG_ROBOTIC])
    {
        Printf("Say: NATURAL and ROBOTIC are mutually exclusive\n");
        result = RETURN_ERROR;
        goto out;
    }
    if (readargs[ARG_RATE])
    {
        arguments.rate = *(LONG *)readargs[ARG_RATE];
        arguments.have_rate = 1;
    }
    if (readargs[ARG_PITCH])
    {
        arguments.pitch = *(LONG *)readargs[ARG_PITCH];
        arguments.have_pitch = 1;
    }
    if (readargs[ARG_FREQUENCY])
    {
        arguments.sample_frequency = *(LONG *)readargs[ARG_FREQUENCY];
        arguments.have_sample_frequency = 1;
    }
    if (readargs[ARG_VOLUME])
    {
        arguments.volume = *(LONG *)readargs[ARG_VOLUME];
        arguments.have_volume = 1;
    }
    if (readargs[ARG_MALE] || readargs[ARG_FEMALE])
        arguments.sex = readargs[ARG_FEMALE] ? FEMALE : MALE;
    if (readargs[ARG_NATURAL] || readargs[ARG_ROBOTIC])
        arguments.mode = readargs[ARG_ROBOTIC] ? ROBOTICF0 : NATURALF0;
    arguments.phoneme_input = readargs[ARG_PHONEMES] != 0;
    arguments.wav_path = (CONST_STRPTR)readargs[ARG_OUT];
    arguments.input_path = (CONST_STRPTR)readargs[ARG_FROM];
    if (!SayParseArguments(parsed_argc, parsed_argv, &arguments,
                           error, sizeof(error)))
    {
        Printf("Say: %s\n", error);
        Usage();
        result = RETURN_ERROR;
        goto out;
    }
    if (arguments.help)
    {
        Usage();
        result = RETURN_OK;
        goto out;
    }
    if (arguments.input_path == NULL && arguments.word_count == 0)
    {
        Usage();
        result = RETURN_ERROR;
        goto out;
    }

    output = AllocVec(sizeof(*output), MEMF_ANY | MEMF_CLEAR);
    stream = AllocVec(sizeof(*stream), MEMF_ANY | MEMF_CLEAR);
    if (output == NULL || stream == NULL)
    {
        result = RETURN_FAIL;
        goto out;
    }
    SCDefaultParams(&output->params);
    result = ApplyArguments(&arguments, &output->params);
    if (result != RETURN_OK)
        goto out;

    if (!arguments.phoneme_input)
    {
        TranslatorBase = OpenLibrary("translator.library", 0);
        if (TranslatorBase == NULL)
        {
            Printf("Say: cannot open translator.library\n");
            result = RETURN_FAIL;
            goto out;
        }
    }
    if (arguments.wav_path != NULL)
    {
        resource_memory = LoadSpeechResource(DOSBase,
                                             "DEVS:Speech/speech.iff",
                                             &resource);
        output->voice = resource.voice != NULL ? resource.voice
                                               : &SCDefaultVoice;
        result = BeginWav(output, arguments.wav_path);
        if (result != RETURN_OK)
            goto out;
    }
    else
    {
        result = BeginPlayback(output);
        if (result != RETURN_OK)
            goto out;
    }

    stream->output = output;
    stream->phoneme_input = arguments.phoneme_input;
    result = arguments.input_path != NULL
        ? FeedFile(stream, arguments.input_path)
        : FeedWords(stream, arguments.words, arguments.word_count);

out:
    if (output != NULL)
    {
        EndPlayback(output);
        EndWav(output);
    }
    if (TranslatorBase != NULL)
        CloseLibrary(TranslatorBase);
    if (resource_memory != NULL)
        FreeVec(resource_memory);
    if (stream != NULL)
        FreeVec(stream);
    if (output != NULL)
        FreeVec(output);
    if (words != NULL)
        FreeVec(words);
    if (argument_storage != NULL)
        FreeVec(argument_storage);
    if (parsed_argv != NULL)
        FreeVec(parsed_argv);
    if (rdargs != NULL)
        FreeArgs(rdargs);
    return result;
}
