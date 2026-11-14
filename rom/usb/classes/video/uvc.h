#ifndef UVC_H
#define UVC_H

/*
 *----------------------------------------------------------------------------
 *                  uvc (USB Video Class) class for poseidon
 *----------------------------------------------------------------------------
 */

#include <exec/types.h>
#include <devices/uvc_capture.h>

#define UVC_MAX_FRAMES 32
#include <exec/nodes.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/lists.h>

/* struct Psd* are used only through pointers here, so forward pointers to the
 * incomplete poseidon types are sufficient (the full definitions come from
 * <proto/poseidon.h> in the .c). */

/* UVC 1.0 Video Probe/Commit control — byte-exact wire format (26 bytes). Used
 * with SET_CUR/GET_CUR on UVVSCS_PROBE_CONTROL then UVVSCS_COMMIT_CONTROL over
 * EP0 to negotiate format/frame/interval before streaming. Multi-byte fields are
 * UBYTE arrays (little-endian on the wire) so the layout is packing-independent. */
struct uvc_streaming_control
{
    UBYTE bmHint[2];
    UBYTE bFormatIndex;
    UBYTE bFrameIndex;
    UBYTE dwFrameInterval[4];
    UBYTE wKeyFrameRate[2];
    UBYTE wPFrameRate[2];
    UBYTE wCompQuality[2];
    UBYTE wCompWindowSize[2];
    UBYTE wDelay[2];
    UBYTE dwMaxVideoFrameSize[4];
    UBYTE dwMaxPayloadTransferSize[4];
};

/* One bound UVC camera. Descriptor pointers remain owned by Poseidon. */
struct NepClassUVC
{
    struct Node          ncu_Node;         /* Node linkage */
    struct NepUVCBase   *ncu_ClsBase;      /* Up linkage (class base) */
    struct PsdDevice    *ncu_Device;       /* Up linkage */
    struct PsdDevice    *ncu_Hub;          /* Up linkage */
    struct PsdConfig    *ncu_Config;       /* Up linkage */
    struct PsdInterface *ncu_Interface;    /* the VideoControl interface */
    ULONG                ncu_HubPort;
    ULONG                ncu_IfNum;        /* VideoControl interface number */
    STRPTR               ncu_DevIDString;  /* Device ID String */
    STRPTR               ncu_IfIDString;   /* Interface ID String */

    /* Selected YUY2 mode and transport, parsed at bind time. */
    struct PsdInterface *ncu_VSInterface;  /* the VideoStreaming interface (alt 0) */
    APTR                 ncu_StreamEP;     /* isoc IN endpoint (found, unopened) */
    APTR                 ncu_StreamAltIf;  /* alt-setting interface owning ncu_StreamEP */
    UWORD                ncu_StreamMaxPkt; /* isoc endpoint wMaxPacketSize (bandwidth) */
    UBYTE                ncu_FormatIndex;  /* chosen VS format index */
    UBYTE                ncu_FrameIndex;   /* chosen VS frame index */
    UBYTE                ncu_PixelFormat;
    UBYTE                ncu_IsMJPEG;      /* TRUE=MJPEG, FALSE=uncompressed/YUYV */
    UBYTE                ncu_IsBulk;       /* TRUE=bulk streaming EP, FALSE=isoc */
    UBYTE                ncu_HaveStream;   /* TRUE once a format+frame was parsed */
    UWORD                ncu_Width;        /* negotiated frame width */
    UWORD                ncu_Height;       /* negotiated frame height */
    char                 ncu_Name[128];
    struct UVCDeviceInfo ncu_Info;
    ULONG                ncu_Id;
    ULONG                ncu_Interval;
    BOOL                 ncu_ExactInterval;
    ULONG                ncu_MaxPayload;
    ULONG                ncu_ModeCount;
    struct UVCModeInfo   ncu_Modes[UVC_MAX_FRAMES];
    struct UVCRates      ncu_Rates[UVC_MAX_FRAMES];
    ULONG                ncu_MaxFrameBuf;  /* dwMaxVideoFrameBuf from the frame desc */
    struct UVCStream    *ncu_Stream;       /* the running stream, NULL when idle */
};

struct NepUVCBase
{
    struct Library       nh_Library;       /* standard */
    struct List          nh_Cameras;
    struct SignalSemaphore nh_Lock;
    ULONG                nh_NextId;
    UWORD                nh_Flags;         /* various flags */

    struct Library      *nh_UtilityBase;   /* Utility base */

    APTR                 nh_LogResBase;    /* log.resource, NULL if absent */
    APTR                 nh_LogRHandle;    /* our log provider handle */
    ULONG                nh_FramesOK;      /* grabs that produced a frame */
    ULONG                nh_FramesFailed;  /* grabs that did not */
};

#endif /* UVC_H */
