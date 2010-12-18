#ifndef AUDIO_DEVICE_H
#define AUDIO_DEVICE_H

#include <exec/types.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <exec/tasks.h>

struct AudioBase
{
    struct Device td_device;
};

#endif
