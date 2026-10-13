#ifndef DEVICES_CD32MPEG_H
#define DEVICES_CD32MPEG_H

/* Public interface of Commodore's CD32 MPEG decoder device. */

#include <exec/io.h>

#define CD32MPEGNAME "cd32mpeg.device"

#define MPEGCMD_PLAY             9
#define MPEGCMD_PAUSE           10
#define MPEGCMD_SLOWMOTION      11
#define MPEGCMD_SINGLESTEP      12
#define MPEGCMD_SEARCH          13
#define MPEGCMD_RECORD          14
#define MPEGCMD_GETDEVINFO      15
#define MPEGCMD_SETWINDOW       16
#define MPEGCMD_SETBORDER       17
#define MPEGCMD_GETVIDEOPARAMS  18
#define MPEGCMD_SETVIDEOPARAMS  19
#define MPEGCMD_SETAUDIOPARAMS  20
#define MPEGCMD_PLAYLSN         21
#define MPEGCMD_SEEKLSN         22
#define MPEGCMD_READFRAMEYUV    23

struct IOMPEGReq
{
    struct IOStdReq iomr_Req;
    WORD            iomr_MPEGError;
    UWORD           iomr_StreamType;
    ULONG           iomr_MPEGFlags;
    ULONG           iomr_Arg1;
    ULONG           iomr_Arg2;
    ULONG           iomr_Arg3;
    ULONG           iomr_Arg4;
    ULONG           iomr_Arg5;
    ULONG           iomr_Arg6;
    ULONG           iomr_Arg7;
    UWORD           iomr_Reserved;
};

#define MPEGDEVINFO_SIZE 264

struct MPEGDeviceInfo
{
    ULONG mdi_Reserved;
    ULONG mdi_DeviceType;
    UBYTE mdi_Name[256];
};

#endif /* DEVICES_CD32MPEG_H */
