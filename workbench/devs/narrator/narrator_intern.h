#ifndef NARRATOR_INTERN_H
#define NARRATOR_INTERN_H

/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <devices/audio.h>
#include <devices/narrator.h>
#include <exec/devices.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
#include <stddef.h>

#include <libraries/speechcore.h>

#define NARRATOR_PRE_V37_SIZE offsetof(struct narrator_rb, F0enthusiasm)
#define MOUTH_PRE_V37_SIZE    (NARRATOR_PRE_V37_SIZE + 4)

struct DosLibrary;

struct NarratorBase
{
    struct Device    nb_Device;
    struct Task     *nb_Task;
    struct MsgPort  *nb_CommandPort;
    struct List      nb_ReadQueue;
    struct SignalSemaphore nb_Lock;
    struct narrator_rb * volatile nb_Current;
    volatile UBYTE   nb_Cancel;
    volatile UBYTE   nb_Stopped;
    volatile UBYTE   nb_Closing;
    UWORD            nb_ControlsPending;
    volatile UWORD  nb_WritesPending;
    struct MsgPort  *nb_AudioPort;
    struct IOAudio  *nb_Audio[2];
    UBYTE           *nb_AudioBuffer[2];
    UBYTE           *nb_ChannelMasks;
    UWORD            nb_ChannelMaskCount;
    struct DosLibrary *nb_DOSBase;
    struct SCResource nb_Resource;
    APTR              nb_ResourceMemory;
};

void NarratorWorker(struct NarratorBase *base);

#endif /* NARRATOR_INTERN_H */
