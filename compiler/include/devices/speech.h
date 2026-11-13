#ifndef DEVICES_SPEECH_H
#define DEVICES_SPEECH_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Text-level speech synthesis device API
    Lang: english
*/

#ifndef EXEC_IO_H
#   include <exec/io.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#define SPEECHNAME              "speech.device"
#define SPEECH_API_VERSION      1

#define SDCMD_QUERY             (CMD_NONSTD + 0)

#define SDERR_BADUTF8           (-20)
#define SDERR_UNSUPPORTEDCHAR   (-21)
#define SDERR_NOENGINE          (-22)
#define SDERR_BADLANGUAGE       (-23)
#define SDERR_BACKEND           (-24)
#define SDERR_BADVERSION        (-25)
#define SDERR_BADVOICE          (-26)
#define SDERR_BADSTYLE          (-27)
#define SDERR_BADFREQUENCY      (-28)

#define SPEECH_TAGBASE          (TAG_USER | 0x535000UL)
#define SPEECHA_Engine          (SPEECH_TAGBASE + 1) /* CONST_STRPTR */
#define SPEECHA_Language        (SPEECH_TAGBASE + 2) /* CONST_STRPTR */
#define SPEECHA_Rate            (SPEECH_TAGBASE + 3) /* U16.16, 1.0 default */
#define SPEECHA_Pitch           (SPEECH_TAGBASE + 4) /* S16.16 semitones */
#define SPEECHA_Volume          (SPEECH_TAGBASE + 5) /* U16.16, 1.0 default */
#define SPEECHA_Voice           (SPEECH_TAGBASE + 6) /* CONST_STRPTR */
#define SPEECHA_Style           (SPEECH_TAGBASE + 7) /* CONST_STRPTR */
#define SPEECHA_SampleFrequency (SPEECH_TAGBASE + 8) /* ULONG, Hz */

#define SPEECH_FIXED_ONE        0x00010000UL

#define SPEECHCAP_UTF8          (1UL << 0)
#define SPEECHCAP_PLAYBACK      (1UL << 1)
#define SPEECHCAP_RATE          (1UL << 2)
#define SPEECHCAP_PITCH         (1UL << 3)
#define SPEECHCAP_VOLUME        (1UL << 4)
#define SPEECHCAP_VOICE         (1UL << 5)
#define SPEECHCAP_STYLE         (1UL << 6)
#define SPEECHCAP_SAMPLEFREQ    (1UL << 7)

struct IOSpeech
{
    struct IOStdReq        ios_Req;
    ULONG                  ios_APIVersion;
    ULONG                  ios_Reserved;
    const struct TagItem  *ios_Attrs;
};

struct SpeechEngineInfo
{
    ULONG                   sei_Size;
    ULONG                   sei_Reserved0;
    CONST_STRPTR             sei_Name;
    CONST_STRPTR             sei_DisplayName;
    const CONST_STRPTR      *sei_Languages;
    ULONG                   sei_Capabilities;
    ULONG                   sei_Reserved1;
    /* Optional tail fields; test sei_Size before reading them. */
    const CONST_STRPTR      *sei_Voices;
    const CONST_STRPTR      *sei_Styles;
    ULONG                   sei_MinSampleFrequency;
    ULONG                   sei_MaxSampleFrequency;
};

struct SpeechDeviceQuery
{
    ULONG                           sdq_Size;
    ULONG                           sdq_APIVersion;
    ULONG                           sdq_EngineCount;
    ULONG                           sdq_Reserved;
    const struct SpeechEngineInfo *const *sdq_Engines;
    CONST_STRPTR                    sdq_DefaultEngine;
};

#endif /* DEVICES_SPEECH_H */
