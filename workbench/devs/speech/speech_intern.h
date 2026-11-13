#ifndef SPEECH_INTERN_H
#define SPEECH_INTERN_H

/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <devices/audio.h>
#include <devices/speech.h>
#include <exec/devices.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>

#include <libraries/speechcore.h>

struct DosLibrary;
struct UtilityBase;

struct SpeechBase
{
    struct Device    sb_Device;
    struct Task     *sb_Task;
    struct MsgPort  *sb_CommandPort;
    struct SignalSemaphore sb_Lock;
    struct IOSpeech * volatile sb_Current;
    volatile UBYTE   sb_Cancel;
    volatile UBYTE   sb_Stopped;
    volatile UBYTE   sb_Closing;
    UWORD            sb_ControlsPending;
    struct MsgPort  *sb_AudioPort;
    struct IOAudio  *sb_Audio[2];
    UBYTE           *sb_AudioBuffer[2];
    struct DosLibrary *sb_DOSBase;
    struct UtilityBase *sb_UtilityBase;
    struct SCResource sb_Resource;
    APTR              sb_ResourceMemory;
};

void SpeechWorker(struct SpeechBase *base);

#endif /* SPEECH_INTERN_H */
