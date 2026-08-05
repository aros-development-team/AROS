/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <devices/narrator.h>
#include <devices/speech.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/translator.h>
#include <utility/tagitem.h>

#include <stdint.h>
#include <string.h>

#include <libraries/speechcore.h>

#define TEXT_LIMIT       4096
#define OPTION_LIMIT      511
#define SELECTOR_LIMIT     63
#define INITIAL_CAPACITY  256

static const UBYTE ChannelMasks[] = { 3, 5, 10, 12 };

struct SpeakOptions
{
    UWORD rate;
    UWORD pitch;
    UBYTE sex;
    UBYTE mode;
    UBYTE ParseOptions;
    UBYTE direct_phonemes;
    UBYTE newline_breaks;
    UBYTE engine_set;
    UBYTE sex_set;
    UBYTE mode_set;
    char engine[SELECTOR_LIMIT + 1];
    char voice[SELECTOR_LIMIT + 1];
    char language[SELECTOR_LIMIT + 1];
    char style[SELECTOR_LIMIT + 1];
};

struct SpeakHandle
{
    struct Node node;
    struct SpeakOptions options;
    UBYTE *text;
    ULONG text_length;
    ULONG text_capacity;
    UBYTE probe[4];
    UWORD probe_length;
    char option_line[OPTION_LIMIT + 1];
    UWORD option_length;
    UBYTE line_mode;              /* 0 text, 1 testing OPT/, 2 option line */
    UBYTE last_was_cr;
};

struct SpeakBase
{
    struct ExecBase *sys_base;
    struct DeviceNode *device_node;
    struct Library *translator_base;
    struct MsgPort *narrator_port;
    struct narrator_rb *narrator_request;
    struct MsgPort *speech_port;
    struct IOSpeech *speech_request;
    struct List handles;
    struct SpeakOptions defaults;
};

static int AsciiLower(int c)
{
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

static int EqualNoCase(const char *a, size_t alen, const char *b)
{
    size_t i;
    if (strlen(b) != alen)
        return 0;
    for (i = 0; i < alen; ++i)
        if (AsciiLower((UBYTE)a[i]) != AsciiLower((UBYTE)b[i]))
            return 0;
    return 1;
}

static int StartsOpt(const UBYTE *s, size_t length)
{
    return length >= 4 && AsciiLower(s[0]) == 'o' &&
        AsciiLower(s[1]) == 'p' && AsciiLower(s[2]) == 't' && s[3] == '/';
}

static int ParseNumber(const char *s, size_t length, ULONG *value)
{
    ULONG n = 0;
    size_t i;
    if (length == 0)
        return 0;
    for (i = 0; i < length; ++i)
    {
        if (s[i] < '0' || s[i] > '9')
            return 0;
        n = n * 10 + (s[i] - '0');
        if (n > 65535)
            return 0;
    }
    *value = n;
    return 1;
}

static LONG CopySelector(char dst[SELECTOR_LIMIT + 1],
                          const char *src, size_t length)
{
    if (length == 0)
        return ERROR_KEY_NEEDS_ARG;
    if (length > SELECTOR_LIMIT)
        return ERROR_LINE_TOO_LONG;
    memcpy(dst, src, length);
    dst[length] = 0;
    return 0;
}

static LONG ParseToken(struct SpeakOptions *options,
                        const char *token, size_t length)
{
    const char *equals;
    ULONG number;
    LONG error;

    if (length == 0)
        return 0;
    equals = memchr(token, '=', length);
    if (equals != NULL)
    {
        size_t key_length = equals - token;
        const char *value = equals + 1;
        size_t value_length = length - key_length - 1;

        if (EqualNoCase(token, key_length, "engine"))
        {
            if (EqualNoCase(value, value_length, "default"))
            {
                options->engine_set = 0;
                options->engine[0] = 0;
                return 0;
            }
            error = CopySelector(options->engine, value, value_length);
            if (error == 0)
                options->engine_set = 1;
            return error;
        }
        if (EqualNoCase(token, key_length, "voice"))
        {
            error = CopySelector(options->voice, value, value_length);
            if (error == 0 && strcmp(options->voice, "male") == 0)
                options->sex = MALE;
            else if (error == 0 && strcmp(options->voice, "female") == 0)
                options->sex = FEMALE;
            return error;
        }
        if (EqualNoCase(token, key_length, "language"))
            return CopySelector(options->language, value, value_length);
        if (EqualNoCase(token, key_length, "style"))
        {
            error = CopySelector(options->style, value, value_length);
            if (error == 0 && strcmp(options->style, "natural") == 0)
                options->mode = NATURALF0;
            else if (error == 0 && strcmp(options->style, "robotic") == 0)
                options->mode = ROBOTICF0;
            return error;
        }
        return ERROR_INVALID_COMPONENT_NAME;
    }

    if (length == 1)
    {
        switch (AsciiLower((UBYTE)token[0]))
        {
            case 'm': options->sex = MALE; options->sex_set = 1; return 0;
            case 'f': options->sex = FEMALE; options->sex_set = 1; return 0;
            case 'n': options->mode = NATURALF0; options->mode_set = 1; return 0;
            case 'r': options->mode = ROBOTICF0; options->mode_set = 1; return 0;
        }
    }
    if (length >= 2)
    {
        switch (AsciiLower((UBYTE)token[0]))
        {
            case 'p':
                if (!ParseNumber(token + 1, length - 1, &number) ||
                    number < MINPITCH || number > MAXPITCH)
                    return ERROR_BAD_NUMBER;
                options->pitch = (UWORD)number;
                return 0;
            case 's':
                if (!ParseNumber(token + 1, length - 1, &number) ||
                    number < MINRATE || number > MAXRATE)
                    return ERROR_BAD_NUMBER;
                options->rate = (UWORD)number;
                return 0;
            case 'o':
                if (length != 2 || (token[1] != '0' && token[1] != '1'))
                    return ERROR_BAD_NUMBER;
                options->ParseOptions = token[1] == '1';
                return 0;
            case 'a':
                if (length != 2 || (token[1] != '0' && token[1] != '1'))
                    return ERROR_BAD_NUMBER;
                options->direct_phonemes = token[1] == '1';
                return 0;
            case 'd':
                if (length != 2 || (token[1] != '0' && token[1] != '1'))
                    return ERROR_BAD_NUMBER;
                options->newline_breaks = token[1] == '1';
                return 0;
        }
    }
    return ERROR_INVALID_COMPONENT_NAME;
}

static LONG ParseOptions(struct SpeakOptions *options,
                          const char *text, size_t length)
{
    struct SpeakOptions staged = *options;
    size_t at = 0;
    if (length == 0)
        return 0;
    if (length >= 3 && EqualNoCase(text, 3, "opt"))
    {
        at = 3;
        if (at == length)
            return 0;
        if (text[at++] != '/')
            return ERROR_INVALID_COMPONENT_NAME;
    }
    while (at < length)
    {
        size_t end = at;
        LONG error;
        while (end < length && text[end] != '/')
            ++end;
        error = ParseToken(&staged, text + at, end - at);
        if (error != 0)
            return error;
        at = end + 1;
    }
    *options = staged;
    return 0;
}

static void OptionsDefaults(struct SpeakOptions *options)
{
    memset(options, 0, sizeof(*options));
    options->rate = DEFRATE;
    options->pitch = DEFPITCH;
    options->sex = DEFSEX;
    options->mode = DEFMODE;
}

static void ReplyPacket(struct SpeakBase *base, struct DosPacket *packet)
{
    struct ExecBase *SysBase = base->sys_base;
    struct MsgPort *reply = packet->dp_Port;
    struct Message *message = packet->dp_Link;
    message->mn_Node.ln_Name = (char *)packet;
    packet->dp_Port = &((struct Process *)FindTask(NULL))->pr_MsgPort;
    PutMsg(reply, message);
}

static LONG OpenNarrator(struct SpeakBase *base)
{
    struct ExecBase *SysBase = base->sys_base;

    if (base->narrator_request != NULL)
        return 0;
    base->narrator_port = CreateMsgPort();
    if (base->narrator_port == NULL)
        return ERROR_NO_FREE_STORE;
    base->narrator_request = (struct narrator_rb *)CreateIORequest(
        base->narrator_port, sizeof(*base->narrator_request));
    if (base->narrator_request == NULL)
    {
        DeleteMsgPort(base->narrator_port);
        base->narrator_port = NULL;
        return ERROR_NO_FREE_STORE;
    }
    if (OpenDevice(NARRATORNAME, 0,
                   (struct IORequest *)&base->narrator_request->message,
                   0) != 0)
    {
        DeleteIORequest((struct IORequest *)&base->narrator_request->message);
        DeleteMsgPort(base->narrator_port);
        base->narrator_request = NULL;
        base->narrator_port = NULL;
        return ERROR_INVALID_RESIDENT_LIBRARY;
    }
    return 0;
}

static LONG OpenSpeech(struct SpeakBase *base)
{
    struct ExecBase *SysBase = base->sys_base;

    if (base->speech_request != NULL)
        return 0;
    base->speech_port = CreateMsgPort();
    if (base->speech_port == NULL)
        return ERROR_NO_FREE_STORE;
    base->speech_request = (struct IOSpeech *)CreateIORequest(
        base->speech_port, sizeof(*base->speech_request));
    if (base->speech_request == NULL)
    {
        DeleteMsgPort(base->speech_port);
        base->speech_port = NULL;
        return ERROR_NO_FREE_STORE;
    }
    if (OpenDevice(SPEECHNAME, 0,
                   (struct IORequest *)&base->speech_request->ios_Req,
                   0) != 0)
    {
        DeleteIORequest((struct IORequest *)&base->speech_request->ios_Req);
        DeleteMsgPort(base->speech_port);
        base->speech_request = NULL;
        base->speech_port = NULL;
        return ERROR_INVALID_RESIDENT_LIBRARY;
    }
    return 0;
}

static LONG NarratorWrite(struct SpeakBase *base,
                           const UBYTE *data, ULONG length,
                           const struct SpeakOptions *options)
{
    struct ExecBase *SysBase = base->sys_base;
    struct narrator_rb *request;
    LONG result = OpenNarrator(base);

    if (result != 0)
        return result;
    request = base->narrator_request;
    request->ch_masks = (UBYTE *)ChannelMasks;
    request->nm_masks = sizeof(ChannelMasks);
    request->rate = options->rate;
    request->pitch = options->pitch;
    request->mode = options->mode;
    request->sex = options->sex;
    request->message.io_Command = CMD_WRITE;
    request->message.io_Data = (APTR)data;
    request->message.io_Length = length;
    return DoIO((struct IORequest *)&request->message) == 0
        ? 0 : ERROR_UNKNOWN;
}

static LONG PitchToSemitones(UWORD hz)
{
    LONG low = -(24L << 16), high = 24L << 16;
    unsigned i;
    for (i = 0; i < 24; ++i)
    {
        LONG middle = low + ((high - low) >> 1);
        ULONG value = (ULONG)(((UQUAD)DEFPITCH *
                               SCSemitoneRatio(middle)) >> 16);
        if (value < hz)
            low = middle + 1;
        else
            high = middle;
    }
    return low;
}

static LONG SpeechWrite(struct SpeakBase *base,
                         const UBYTE *data, ULONG length,
                         const struct SpeakOptions *options)
{
    struct ExecBase *SysBase = base->sys_base;
    struct IOSpeech *request;
    struct TagItem tags[8];
    ULONG count = 0;
    CONST_STRPTR voice = options->voice;
    CONST_STRPTR style = options->style;
    BOOL explicit_narrator = options->engine_set &&
        strcmp(options->engine, "narrator") == 0;
    LONG result;

    if (options->engine_set)
    {
        tags[count].ti_Tag = SPEECHA_Engine;
        tags[count++].ti_Data = (IPTR)options->engine;
    }
    if (voice[0] || (explicit_narrator && options->sex_set))
    {
        if (!voice[0])
            voice = options->sex == FEMALE ? "female" : "male";
        tags[count].ti_Tag = SPEECHA_Voice;
        tags[count++].ti_Data = (IPTR)voice;
    }
    if (style[0] || (explicit_narrator && options->mode_set))
    {
        if (!style[0]) style = options->mode == ROBOTICF0
            ? "robotic" : "natural";
        tags[count].ti_Tag = SPEECHA_Style;
        tags[count++].ti_Data = (IPTR)style;
    }
    if (options->language[0])
    {
        tags[count].ti_Tag = SPEECHA_Language;
        tags[count++].ti_Data = (IPTR)options->language;
    }
    tags[count].ti_Tag = SPEECHA_Rate;
    tags[count++].ti_Data = ((ULONG)options->rate << 16) / DEFRATE;
    tags[count].ti_Tag = SPEECHA_Pitch;
    tags[count++].ti_Data = (IPTR)PitchToSemitones(options->pitch);
    tags[count].ti_Tag = TAG_DONE;
    tags[count].ti_Data = 0;

    result = OpenSpeech(base);
    if (result != 0)
        return result;
    request = base->speech_request;
    request->ios_APIVersion = SPEECH_API_VERSION;
    request->ios_Reserved = 0;
    request->ios_Attrs = tags;
    request->ios_Req.io_Command = CMD_WRITE;
    request->ios_Req.io_Data = (APTR)data;
    request->ios_Req.io_Length = length;
    return DoIO((struct IORequest *)&request->ios_Req) == 0
        ? 0 : ERROR_UNKNOWN;
}

static int SelectorIs(const char *value, const char *expected)
{
    return value[0] == 0 || strcmp(value, expected) == 0;
}

static int NarratorCompatible(const struct SpeakOptions *options)
{
    if (options->engine_set && strcmp(options->engine, "narrator") != 0)
        return 0;
    if (options->language[0] != 0 &&
        strcmp(options->language, "en-US") != 0 &&
        strcmp(options->language, "en") != 0)
        return 0;
    if (!SelectorIs(options->voice, "male") &&
        strcmp(options->voice, "female") != 0)
        return 0;
    if (!SelectorIs(options->style, "natural") &&
        strcmp(options->style, "robotic") != 0)
        return 0;
    return 1;
}

static LONG SpeakSegment(struct SpeakBase *base, struct SpeakHandle *handle,
                          const UBYTE *data, ULONG length)
{
    ULONG begin = 0, end = length;
    struct SpeakOptions *options = &handle->options;
    struct ExecBase *SysBase = base->sys_base;

    while (begin < end && (data[begin] == ' ' || data[begin] == '\t' ||
                           data[begin] == '\r' || data[begin] == '\n'))
        ++begin;
    while (end > begin && (data[end - 1] == ' ' || data[end - 1] == '\t' ||
                           data[end - 1] == '\r' || data[end - 1] == '\n'))
        --end;
    if (begin == end)
        return 0;

    if (options->direct_phonemes)
    {
        if (!NarratorCompatible(options))
            return ERROR_NOT_IMPLEMENTED;
        return NarratorWrite(base, data + begin, end - begin, options);
    }
    if (!NarratorCompatible(options))
        return SpeechWrite(base, data + begin, end - begin, options);

    if (base->translator_base == NULL)
        base->translator_base = OpenLibrary("translator.library", 0);
    if (base->translator_base == NULL)
        return ERROR_INVALID_RESIDENT_LIBRARY;
    {
        struct Library *TranslatorBase = base->translator_base;
        ULONG input_length = end - begin;
        ULONG output_length = input_length * 8 + 32;
        char *phonemes = AllocVec(output_length, MEMF_ANY);
        LONG result;
        if (phonemes == NULL)
            return ERROR_NO_FREE_STORE;
        if (Translate((CONST_STRPTR)data + begin, input_length,
                      phonemes, output_length) != 0)
            result = ERROR_UNKNOWN;
        else
            result = NarratorWrite(base, (const UBYTE *)phonemes,
                                    strlen(phonemes), options);
        FreeVec(phonemes);
        return result;
    }
}

/* Retain an incomplete trailing UTF-8 sequence for the next segment. */
static ULONG UTF8SafeLength(const UBYTE *text, ULONG length)
{
    ULONG lead = length;
    ULONG continuation;
    ULONG expected;

    while (lead > 0 && (text[lead - 1] & 0xc0) == 0x80)
        --lead;
    if (lead == 0)
        return length;
    --lead;
    continuation = length - lead - 1;
    if ((text[lead] & 0xe0) == 0xc0)
        expected = 1;
    else if ((text[lead] & 0xf0) == 0xe0)
        expected = 2;
    else if ((text[lead] & 0xf8) == 0xf0)
        expected = 3;
    else
        return length;
    return continuation < expected ? lead : length;
}

static LONG FlushText(struct SpeakBase *base, struct SpeakHandle *handle,
                       ULONG count)
{
    LONG error;
    if (count == 0)
        return 0;
    error = SpeakSegment(base, handle, handle->text, count);
    if (error != 0)
        return error;
    if (count < handle->text_length)
        memmove(handle->text, handle->text + count,
                handle->text_length - count);
    handle->text_length -= count;
    return 0;
}

static LONG MakeRoom(struct SpeakBase *base, struct SpeakHandle *handle)
{
    struct ExecBase *SysBase = base->sys_base;
    if (handle->text_length < handle->text_capacity)
        return 0;
    if (handle->text_capacity < TEXT_LIMIT)
    {
        ULONG capacity = handle->text_capacity ? handle->text_capacity * 2
                                               : INITIAL_CAPACITY;
        UBYTE *grown;
        if (capacity > TEXT_LIMIT)
            capacity = TEXT_LIMIT;
        grown = AllocVec(capacity, MEMF_ANY);
        if (grown == NULL)
            return ERROR_NO_FREE_STORE;
        if (handle->text_length) CopyMem(handle->text, grown,
                                         handle->text_length);
        if (handle->text)
            FreeVec(handle->text);
        handle->text = grown;
        handle->text_capacity = capacity;
        return 0;
    }
    {
        ULONG split = handle->text_length;
        while (split > 0 && handle->text[split - 1] != ' ' &&
               handle->text[split - 1] != '\t')
            --split;
        if (split == 0)
        {
            split = UTF8SafeLength(handle->text, handle->text_length);
            if (split == 0)
                split = handle->text_length;
        }
        return FlushText(base, handle, split);
    }
}

static int AsciiAlpha(UBYTE c)
{
    c = (UBYTE)AsciiLower(c);
    return c >= 'a' && c <= 'z';
}

static int PeriodIsAbbreviation(const UBYTE *text, ULONG period)
{
    static const char *const abbreviations[] = {
        "mr", "mrs", "ms", "dr", "prof", "st", "vs", "etc"
    };
    ULONG begin = period;
    size_t i;

    while (begin > 0 && AsciiAlpha(text[begin - 1]))
        --begin;
    if (period - begin == 1 &&
        text[begin] >= 'A' && text[begin] <= 'Z')
        return 1;
    for (i = 0; i < sizeof(abbreviations) / sizeof(abbreviations[0]); ++i)
        if (EqualNoCase((const char *)text + begin, period - begin,
                         abbreviations[i]))
            return 1;
    return 0;
}

static int SentenceBoundary(const UBYTE *text, ULONG length)
{
    ULONG at = length;
    UBYTE punctuation;

    while (at > 0 && (text[at - 1] == '\'' || text[at - 1] == '"' ||
                      text[at - 1] == ')' || text[at - 1] == ']'))
        --at;
    if (at == 0)
        return 0;
    punctuation = text[at - 1];
    if (punctuation == '?' || punctuation == '!')
        return 1;
    if (punctuation != '.')
        return 0;
    if (at >= 2 && text[at - 2] == '.')
        return 1;
    return !PeriodIsAbbreviation(text, at - 1);
}

static LONG AppendText(struct SpeakBase *base, struct SpeakHandle *handle,
                        UBYTE byte)
{
    LONG error;
    if ((byte == ' ' || byte == '\t') &&
        SentenceBoundary(handle->text, handle->text_length))
        return FlushText(base, handle, handle->text_length);
    error = MakeRoom(base, handle);
    if (error != 0)
        return error;
    handle->text[handle->text_length++] = byte;
    return 0;
}

static void ResetLine(struct SpeakHandle *handle)
{
    handle->line_mode = handle->options.ParseOptions ? 1 : 0;
    handle->probe_length = 0;
    handle->option_length = 0;
}

static LONG FinishLine(struct SpeakBase *base, struct SpeakHandle *handle)
{
    LONG error = 0;
    UWORD i;
    if (handle->line_mode == 2)
    {
        handle->option_line[handle->option_length] = 0;
        error = ParseOptions(&handle->options, handle->option_line,
                              handle->option_length);
    }
    else
    {
        if (handle->line_mode == 1)
            for (i = 0; error == 0 && i < handle->probe_length; ++i)
                error = AppendText(base, handle, handle->probe[i]);
        if (error == 0)
        {
            if (handle->options.newline_breaks)
                error = FlushText(base, handle, handle->text_length);
            else if (handle->text_length != 0 &&
                     handle->text[handle->text_length - 1] != ' ')
                error = AppendText(base, handle, ' ');
        }
    }
    ResetLine(handle);
    return error;
}

static LONG FeedByte(struct SpeakBase *base, struct SpeakHandle *handle,
                      UBYTE byte, BOOL *accepted)
{
    LONG error;
    UWORD i;
    *accepted = FALSE;
    if (byte == '\n' && handle->last_was_cr)
    {
        handle->last_was_cr = 0;
        *accepted = TRUE;
        return 0;
    }
    if (byte == '\r' || byte == '\n')
    {
        handle->last_was_cr = byte == '\r';
        *accepted = TRUE;
        return FinishLine(base, handle);
    }
    handle->last_was_cr = 0;

    if (handle->line_mode == 2)
    {
        if (handle->option_length == OPTION_LIMIT)
            return ERROR_LINE_TOO_LONG;
        handle->option_line[handle->option_length++] = byte;
        *accepted = TRUE;
        return 0;
    }
    if (handle->line_mode == 1)
    {
        static const UBYTE expected[4] = { 'o', 'p', 't', '/' };
        handle->probe[handle->probe_length++] = byte;
        *accepted = TRUE;
        if (AsciiLower(byte) != expected[handle->probe_length - 1])
        {
            handle->line_mode = 0;
            for (i = 0; i < handle->probe_length; ++i)
            {
                error = AppendText(base, handle, handle->probe[i]);
                if (error != 0)
                    return error;
            }
            handle->probe_length = 0;
            return 0;
        }
        if (handle->probe_length == 4)
        {
            memcpy(handle->option_line, handle->probe, 4);
            handle->option_length = 4;
            handle->probe_length = 0;
            handle->line_mode = 2;
        }
        return 0;
    }
    error = AppendText(base, handle, byte);
    if (error == 0)
        *accepted = TRUE;
    return error;
}

static LONG WriteText(struct SpeakBase *base, struct SpeakHandle *handle,
                       const UBYTE *data, ULONG length, ULONG *consumed)
{
    ULONG i;
    *consumed = 0;
    for (i = 0; i < length; ++i)
    {
        BOOL accepted;
        LONG error = FeedByte(base, handle, data[i], &accepted);
        if (accepted)
            ++*consumed;
        if (error != 0)
            return error;
    }
    return 0;
}

static LONG CloseHandle(struct SpeakBase *base, struct SpeakHandle *handle)
{
    struct ExecBase *SysBase = base->sys_base;
    LONG error;
    if (handle->line_mode == 2 || handle->line_mode == 1)
        error = FinishLine(base, handle);
    else
        error = FlushText(base, handle, handle->text_length);
    if (error == 0 && handle->text_length)
        error = FlushText(base, handle, handle->text_length);
    Remove(&handle->node);
    if (handle->text)
        FreeVec(handle->text);
    FreeVec(handle);
    return error;
}

static struct SpeakHandle *OpenHandle(struct SpeakBase *base, BSTR name,
                                       LONG *error)
{
    struct ExecBase *SysBase = base->sys_base;
    struct SpeakHandle *handle = AllocVec(sizeof(*handle), MEMF_CLEAR);
    int length = AROS_BSTR_strlen(name);
    if (handle == NULL)
    {
        *error = ERROR_NO_FREE_STORE;
        return NULL;
    }
    handle->options = base->defaults;
    ResetLine(handle);
    if (length != 0)
    {
        *error = ParseOptions(&handle->options,
            (CONST_STRPTR)AROS_BSTR_ADDR(name), length);
        ResetLine(handle);
        if (*error != 0)
        {
            FreeVec(handle);
            return NULL;
        }
    }
    AddTail(&base->handles, &handle->node);
    *error = 0;
    return handle;
}

static LONG DecodeStartup(struct SpeakBase *base, BPTR startup)
{
    const UBYTE *data;
    size_t length;
    if (startup == BNULL)
        return 0;
    data = (const UBYTE *)AROS_BSTR_ADDR(startup);
    length = AROS_BSTR_strlen(startup);
    if (length != 0 && StartsOpt(data, length))
        return ParseOptions(&base->defaults, (const char *)data, length);
    return 0;
}

LONG speak_handler(struct ExecBase *sysBase)
{
    struct ExecBase *SysBase = sysBase;
    struct Process *process = (struct Process *)FindTask(NULL);
    struct MsgPort *port = &process->pr_MsgPort;
    struct DosPacket *packet;
    struct SpeakBase base;
    BOOL running = TRUE;

    memset(&base, 0, sizeof(base));
    base.sys_base = sysBase;
    NEWLIST(&base.handles);
    OptionsDefaults(&base.defaults);
    WaitPort(port);
    packet = (struct DosPacket *)GetMsg(port)->mn_Node.ln_Name;
    base.device_node = (struct DeviceNode *)BADDR(packet->dp_Arg3);
    packet->dp_Res2 = DecodeStartup(&base, (BPTR)packet->dp_Arg2);
    if (packet->dp_Res2 == 0)
        base.device_node->dn_Task = port;
    packet->dp_Res1 = packet->dp_Res2 == 0 ? DOSTRUE : DOSFALSE;
    if (packet->dp_Res2 != 0)
    {
        ReplyPacket(&base, packet);
        return RETURN_FAIL;
    }

    do
    {
        struct SpeakHandle *handle;
        struct FileHandle *file;
        ReplyPacket(&base, packet);
        WaitPort(port);
        packet = (struct DosPacket *)GetMsg(port)->mn_Node.ln_Name;
        packet->dp_Res2 = 0;
        switch (packet->dp_Type)
        {
            case ACTION_FINDOUTPUT:
            case ACTION_FINDUPDATE:
            {
                LONG error;
                handle = OpenHandle(&base, (BSTR)packet->dp_Arg3, &error);
                packet->dp_Res2 = error;
                if (handle)
                {
                    file = BADDR(packet->dp_Arg1);
                    file->fh_Arg1 = (SIPTR)handle;
                    file->fh_Type = port;
                    packet->dp_Res1 = DOSTRUE;
                }
                else
                    packet->dp_Res1 = DOSFALSE;
                break;
            }
            case ACTION_FINDINPUT:
                packet->dp_Res1 = DOSFALSE;
                packet->dp_Res2 = ERROR_READ_PROTECTED;
                break;
            case ACTION_WRITE:
                handle = (struct SpeakHandle *)packet->dp_Arg1;
                if (handle == NULL || packet->dp_Arg3 < 0)
                {
                    packet->dp_Res1 = -1;
                    packet->dp_Res2 = handle == NULL
                        ? ERROR_OBJECT_WRONG_TYPE : ERROR_BAD_NUMBER;
                }
                else
                {
                    ULONG consumed;
                    packet->dp_Res2 = WriteText(&base, handle,
                        (const UBYTE *)packet->dp_Arg2, packet->dp_Arg3,
                        &consumed);
                    packet->dp_Res1 = packet->dp_Res2 != 0 && consumed == 0
                        ? -1 : (LONG)consumed;
                }
                break;
            case ACTION_END:
                handle = (struct SpeakHandle *)packet->dp_Arg1;
                if (handle == NULL)
                {
                    packet->dp_Res1 = DOSFALSE;
                    packet->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
                }
                else
                {
                    packet->dp_Res2 = CloseHandle(&base, handle);
                    packet->dp_Res1 = packet->dp_Res2 == 0
                        ? DOSTRUE : DOSFALSE;
                }
                break;
            case ACTION_DIE:
                if (IsListEmpty(&base.handles))
                {
                    Forbid();
                    if (base.device_node->dn_Task == port)
                        base.device_node->dn_Task = NULL;
                    Permit();
                    packet->dp_Res1 = DOSTRUE;
                    running = FALSE;
                }
                else
                {
                    packet->dp_Res1 = DOSFALSE;
                    packet->dp_Res2 = ERROR_OBJECT_IN_USE;
                }
                break;
            default:
                packet->dp_Res1 = DOSFALSE;
                packet->dp_Res2 = ERROR_ACTION_NOT_KNOWN;
                break;
        }
    } while (running);

    ReplyPacket(&base, packet);
    if (base.translator_base)
        CloseLibrary(base.translator_base);
    if (base.narrator_request != NULL)
    {
        CloseDevice((struct IORequest *)&base.narrator_request->message);
        DeleteIORequest((struct IORequest *)&base.narrator_request->message);
        DeleteMsgPort(base.narrator_port);
    }
    if (base.speech_request != NULL)
    {
        CloseDevice((struct IORequest *)&base.speech_request->ios_Req);
        DeleteIORequest((struct IORequest *)&base.speech_request->ios_Req);
        DeleteMsgPort(base.speech_port);
    }
    return RETURN_OK;
}
