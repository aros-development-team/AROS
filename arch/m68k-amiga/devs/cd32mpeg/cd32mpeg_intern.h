#ifndef CD32MPEG_INTERN_H
#define CD32MPEG_INTERN_H

#include <exec/devices.h>
#include <exec/interrupts.h>
#include <exec/types.h>

#include LC_LIBDEFS_FILE

struct CD32MPEGBase
{
    struct Device cmb_Device;
    BOOL          cmb_Present;
    BOOL          cmb_Initialized;
    APTR          cmb_BoardAddr;
    struct Task  *cmb_Task;
    struct MsgPort *cmb_MsgPort;
    struct Interrupt cmb_Interrupt;
    BOOL          cmb_InterruptInstalled;
    volatile ULONG cmb_VideoIRQs;
    volatile ULONG cmb_AudioIRQs;
    UBYTE         cmb_VideoByte;
    UBYTE         cmb_AudioByte;
    BOOL          cmb_VideoBytePending;
    BOOL          cmb_AudioBytePending;
};

#endif /* CD32MPEG_INTERN_H */
