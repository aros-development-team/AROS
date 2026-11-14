/*
 *----------------------------------------------------------------------------
 *                  uvc (USB Video Class) class for poseidon
 *----------------------------------------------------------------------------
 *
 * Poseidon USB Video Class capture. The class dispatcher exposes stable
 * camera IDs and RGB24 frames, e.g. to a VHI driver.
 */

#include "debug.h"

#include "uvc.class.h"
#include <devices/timer.h>
#include <devices/uvc_capture.h>
#include "uvc_payload.h"
#include "uvc_jpeg.h"

#define __LOG_LIBBASE (nh->nh_LogResBase)
#include <proto/log.h>
#include <resources/log.h>

/* Diagnostics go to log.resource (System/LogView, or LogDump to a file) as
 * well as the debug channel: a machine without a serial port has no other
 * way to show why a grab failed. */
#define UVCLOG(type, fmt, ...) do { \
    if (nh->nh_LogRHandle) \
        logAddEntry(((type) | 20), nh->nh_LogRHandle, "", __func__, 0, fmt, ##__VA_ARGS__); \
    bug("uvc.class: " fmt "\n", ##__VA_ARGS__); \
} while (0)

/* /// "Lib Stuff" */
static const STRPTR libname = MOD_NAME_STRING;

static int libInit(LIBBASETYPEPTR nh)
{
    struct NepUVCBase *ret = NULL;

    KPRINTF(10, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    NEWLIST(&nh->nh_Cameras);
    InitSemaphore(&nh->nh_Lock);
    InitSemaphore(&uvc_jpeg_lock);
    nh->nh_NextId = 0;
    nh->nh_UtilityBase = OpenLibrary("utility.library", 39);
    nh->nh_LogResBase = OpenResource("log.resource");
    if (nh->nh_LogResBase)
        nh->nh_LogRHandle = logInitialise(&nh->nh_Library.lib_Node);

#define	UtilityBase	nh->nh_UtilityBase

    if(UtilityBase)
    {
        ret = nh;
    } else {
        KPRINTF(20, ("libInit: OpenLibrary(\"utility.library\", 39) failed!\n"));
    }

    KPRINTF(10, ("libInit: Ok\n"));
    return(ret ? TRUE : FALSE);
}

static int libExpunge(LIBBASETYPEPTR nh)
{
    KPRINTF(10, ("libExpunge nh: 0x%08lx\n", nh));
    if (nh->nh_LogRHandle)
        logFinalise(nh->nh_LogRHandle);
    CloseLibrary((struct Library *) UtilityBase);
    return(TRUE);
}

ADD2INITLIB(libInit, 0)
ADD2EXPUNGELIB(libExpunge, 0)
/* \\\ */

/* Match exact uncompressed GUIDs and bit depths; MJPEG has its own descriptor.
 * Retain supported descriptors for enumeration; selection prefers a fitting
 * device default. The bDefaultFrameInterval belongs to that exact frame. */
struct uvc_frame_cand {
    UWORD w, h;
    UBYTE format, frame;
    ULONG interval;   /* slowest advertised interval of at least 10 fps */
    ULONG defint;     /* the frame descriptor's own default interval */
    ULONG maxbuf;     /* dwMaxVideoFrameBufferSize from the descriptor */
};

static BOOL uvc_parse_streaming(struct NepClassUVC *nch)
{
    struct NepUVCBase *nh = nch->ncu_ClsBase;
    struct Library *ps = OpenLibrary("poseidon.library", 4);
    APTR dd = NULL;
    ULONG alts_with_iso = 0;
    UBYTE format = 0, pixel_format = 0;
    struct uvc_frame_cand cand[UVC_MAX_FRAMES];
    ULONG ncand = 0;
    LONG chosen = -1;
    const char *reason = "";
    IPTR superspeed = FALSE;
    ULONG altcap;
    static const UBYTE yuy2[16] = {
        'Y','U','Y','2',0,0,0x10,0,0x80,0,0,0xaa,0,0x38,0x9b,0x71
    };
    static const UBYTE gray8[16] = {
        'Y','8','0','0',0,0,0x10,0,0x80,0,0,0xaa,0,0x38,0x9b,0x71
    };
    static const UBYTE y8[16] = {
        'Y','8',' ',' ',0,0,0x10,0,0x80,0,0,0xaa,0,0x38,0x9b,0x71
    };
    static const UBYTE gray16[16] = {
        'Y','1','6',' ',0,0,0x10,0,0x80,0,0,0xaa,0,0x38,0x9b,0x71
    };
    if (!ps) return FALSE;
    /* The VideoControl header (CS_INTERFACE subtype 1) names the streaming
     * interfaces that belong to this control interface (baInterfaceNr). A
     * device with two camera functions, such as an RGB and an IR module,
     * has two of each; take ours, not the first one on the device. */
    while ((dd = psdFindDescriptor(nch->ncu_Device, dd,
        DDA_DescriptorType, UDT_CS_INTERFACE,
        DDA_Interface, nch->ncu_Interface, TAG_END))) {
        UBYTE *p = NULL;
        IPTR len = 0;
        ULONG i;
        psdGetAttrs(PGA_DESCRIPTOR, dd, DDA_DescriptorData, &p,
                    DDA_DescriptorLength, &len, TAG_END);
        if (!p || len < 13 || p[0] > len || p[2] != 0x01) continue;
        for (i = 0; i < p[11] && 12 + i < p[0] && !nch->ncu_VSInterface; i++)
            nch->ncu_VSInterface = psdFindInterface(nch->ncu_Device, NULL,
                IFA_InterfaceNum, p[12 + i],
                IFA_Class, VIDEO_CLASSCODE, IFA_SubClass, VIDEO_STREAM_SUBCLASS,
                TAG_END);
        break;
    }
    dd = NULL;
    if (!nch->ncu_VSInterface)
        nch->ncu_VSInterface = psdFindInterface(nch->ncu_Device, NULL,
            IFA_Class, VIDEO_CLASSCODE, IFA_SubClass, VIDEO_STREAM_SUBCLASS,
            TAG_END);
    if (!nch->ncu_VSInterface) goto done;
    while ((dd = psdFindDescriptor(nch->ncu_Device, dd,
        DDA_DescriptorType, UDT_CS_INTERFACE,
        DDA_Interface, nch->ncu_VSInterface, TAG_END))) {
        UBYTE *p = NULL;
        IPTR len = 0;
        psdGetAttrs(PGA_DESCRIPTOR, dd, DDA_DescriptorData, &p,
                    DDA_DescriptorLength, &len, TAG_END);
        if (!p || len < 3 || p[0] > len || p[0] < 3) continue;
        if (p[2] == UDST_VIDEO_STREAM_FORMAT_UNCOMPRESSED) {
            if (p[0] >= 27) {
                char guid[48];
                IPTR vsnum = 0;
                ULONG k;
                /* Raw descriptor byte order avoids guessing GUID aliases or
                 * treating a monochrome stream as packed YUY2. */
                for (k = 0; k < 16; k++)
                    snprintf(guid + 3*k, sizeof(guid) - 3*k,
                             k == 15 ? "%02lx" : "%02lx:", (unsigned long)p[5+k]);
                psdGetAttrs(PGA_INTERFACE, nch->ncu_VSInterface,
                            IFA_InterfaceNum, &vsnum, TAG_END);
                UVCLOG(LOGF_Flag_Type_Information,
                       "'%s': VC %lu VS %lu uncompressed format %lu GUID bytes %s bpp %lu frames %lu default frame %lu",
                       nch->ncu_Name, (IPTR)nch->ncu_IfNum, vsnum,
                       (IPTR)p[3], guid, (IPTR)p[21], (IPTR)p[4], (IPTR)p[22]);
            }
            pixel_format = 0;
            if (p[0] >= 27) {
                if (!memcmp(p+5, yuy2, 16) && p[21] == 16) pixel_format = UVC_MODE_YUY2;
                else if ((!memcmp(p+5, gray8, 16) || !memcmp(p+5, y8, 16)) && p[21] == 8)
                    pixel_format = UVC_MODE_GRAY8;
                else if (!memcmp(p+5, gray16, 16) && p[21] == 16) pixel_format = UVC_MODE_GRAY16;
            }
            format = pixel_format ? p[3] : 0;
        } else if (p[2] == UDST_VIDEO_STREAM_FORMAT_MJPEG) {
            format = p[0] >= 11 ? p[3] : 0;
            pixel_format = UVC_MODE_MJPEG;
        } else if (((p[2] == UDST_VIDEO_STREAM_FRAME_UNCOMPRESSED && pixel_format != UVC_MODE_MJPEG) ||
                    (p[2] == UDST_VIDEO_STREAM_FRAME_MJPEG && pixel_format == UVC_MODE_MJPEG)) &&
                   format && p[0] >= 26) {
            ULONG w = p[5] | ((ULONG)p[6] << 8);
            ULONG h = p[7] | ((ULONG)p[8] << 8);
            ULONG interval = p[21] | ((ULONG)p[22] << 8) |
                             ((ULONG)p[23] << 16) | ((ULONG)p[24] << 24);
            /* Start with a conservative advertised rate (at least 10 fps).
             * Discrete intervals follow the fixed 26-byte frame descriptor. */
            if (p[25] && p[0] >= 26 + 4 * (ULONG)p[25]) {
                ULONG j;
                for (j = 0; j < p[25]; ++j) {
                    UBYTE *q = p + 26 + 4*j;
                    ULONG candidate = q[0] | ((ULONG)q[1]<<8) |
                        ((ULONG)q[2]<<16) | ((ULONG)q[3]<<24);
                    if (candidate > interval && candidate <= 1000000)
                        interval = candidate;
                }
            }
            if (w && h && (pixel_format != UVC_MODE_YUY2 || !(w & 1)) && w <= 3840 && h <= 2160 && interval &&
                ncand < UVC_MAX_FRAMES) {
                cand[ncand].w = w;
                cand[ncand].h = h;
                cand[ncand].format = format;
                cand[ncand].frame = p[3];
                cand[ncand].interval = interval;
                cand[ncand].defint = p[21] | ((ULONG)p[22] << 8) |
                                     ((ULONG)p[23] << 16) | ((ULONG)p[24] << 24);
                cand[ncand].maxbuf = p[17] | ((ULONG)p[18] << 8) |
                                     ((ULONG)p[19] << 16) | ((ULONG)p[20] << 24);
                {
                    struct UVCModeInfo *mode = &nch->ncu_Modes[ncand];
                    mode->width = w; mode->height = h;
                    mode->format_index = format; mode->frame_index = p[3];
                    mode->pixel_format = pixel_format;
                    mode->default_interval = cand[ncand].defint;
                    mode->capture_interval = interval;
                    mode->max_frame_bytes = cand[ncand].maxbuf;
                    {
                        struct UVCRates *rates = &nch->ncu_Rates[ncand];
                        ULONG j, entries = p[25] ? p[25] : 3;
                        if (p[0] >= 26 + 4 * entries) {
                            for (j = 0; j < entries; j++) {
                                const UBYTE *q = p + 26 + 4*j;
                                ULONG value = q[0] | ((ULONG)q[1]<<8) |
                                    ((ULONG)q[2]<<16) | ((ULONG)q[3]<<24);
                                if (p[25]) {
                                    if (value && rates->count < UVC_MAX_INTERVALS)
                                        rates->intervals[rates->count++] = value;
                                } else if (!j) rates->minimum = value;
                                else if (j == 1) rates->maximum = value;
                                else rates->step = value;
                            }
                        }
                    }
                }
                nch->ncu_ModeCount = ++ncand;
            }
        }
    }
    /* The stream endpoint: a bulk IN on the interface itself, or the
     * isochronous IN alternate with the most bandwidth the bus allows
     * (3 x 1024 per microframe at high speed, 48 x 1024 at SuperSpeed). */
    psdGetAttrs(PGA_DEVICE, nch->ncu_Device, DA_IsSuperspeed, &superspeed, TAG_END);
    altcap = superspeed ? 49152 : 3072;
    {
        struct List *alts = NULL;
        struct Node *node;
        nch->ncu_StreamEP = psdFindEndpoint(nch->ncu_VSInterface, NULL,
            EA_IsIn, TRUE, EA_TransferType, USEAF_BULK, TAG_END);
        nch->ncu_IsBulk = nch->ncu_StreamEP != NULL;
        if (!nch->ncu_IsBulk) {
            psdGetAttrs(PGA_INTERFACE, nch->ncu_VSInterface,
                        IFA_AlternateIfList, &alts, TAG_END);
            if (alts) for (node=alts->lh_Head; node->ln_Succ; node=node->ln_Succ) {
                APTR ep = psdFindEndpoint((APTR)node, NULL, EA_IsIn, TRUE,
                                          EA_TransferType, USEAF_ISOCHRONOUS, TAG_END);
                IPTR raw = 0, transactions = 1;
                ULONG bytes;
                if (!ep) continue;
                psdGetAttrs(PGA_ENDPOINT, ep, EA_MaxPktSize, &raw,
                            EA_NumTransMuFrame, &transactions, TAG_END);
                bytes = raw * transactions;
                alts_with_iso++;
                if (bytes > nch->ncu_StreamMaxPkt && bytes <= altcap) {
                    nch->ncu_StreamEP = ep;
                    nch->ncu_StreamAltIf = (APTR)node;
                    nch->ncu_StreamMaxPkt = bytes;
                }
            }
        }
    }
    if (ncand && nch->ncu_StreamEP) {
        /* Negotiate rather than assume: ask the device for its default
         * probe (GET_DEF) and take that frame when it is one of ours and the
         * stream endpoint can carry it; otherwise the largest frame that fits. */
        struct MsgPort *mp = CreateMsgPort();
        APTR pipe = mp ? psdAllocPipe(nch->ncu_Device, mp, NULL) : NULL;
        UBYTE probe[26];
        IPTR vsnum = 0;
        LONG ioerr = -1, dflt = -1;
        ULONG devinterval = 0, i;
        /* bytes per second the stream endpoint carries: 8000 service
         * intervals per second at high and super speed */
        ULONG budget = nch->ncu_IsBulk ? 0xFFFFFFFFUL : (ULONG)nch->ncu_StreamMaxPkt * 8000UL;
        psdGetAttrs(PGA_INTERFACE, nch->ncu_VSInterface, IFA_InterfaceNum, &vsnum, TAG_END);
        memset(probe, 0, sizeof(probe));
        if (pipe) {
            psdPipeSetup(pipe, URTF_IN|URTF_CLASS|URTF_INTERFACE,
                         UVUDR_GET_DEF, UVVSCS_PROBE_CONTROL << 8, vsnum);
            ioerr = psdDoPipe(pipe, probe, sizeof(probe));
            if (ioerr == 0 && psdGetPipeActual(pipe) < 8) ioerr = -2;
            psdFreePipe(pipe);
        }
        if (mp) DeleteMsgPort(mp);
        if (ioerr == 0) {
            devinterval = probe[4] | ((ULONG)probe[5] << 8) |
                          ((ULONG)probe[6] << 16) | ((ULONG)probe[7] << 24);
            for (i = 0; i < ncand; i++)
                if (cand[i].format == probe[2] && cand[i].frame == probe[3]) dflt = i;
        }
        if (dflt >= 0) {
            ULONG size = cand[dflt].maxbuf ? cand[dflt].maxbuf : (ULONG)cand[dflt].w * cand[dflt].h * 2;
            ULONG ival = devinterval ? devinterval : cand[dflt].defint;
            ULONG fps = ival ? 10000000UL / ival : 30;
            if (!fps) fps = 1;
            if (size <= budget / fps) {
                chosen = dflt;
                cand[dflt].interval = ival;
                reason = "device default";
            } else {
                UVCLOG(LOGF_Flag_Type_Information, "'%s': device default %lux%lu at %lu fps needs %lu bytes/s, stream endpoint carries %lu; choosing the largest frame that fits",
                       nch->ncu_Name, (IPTR)cand[dflt].w, (IPTR)cand[dflt].h, (IPTR)fps,
                       (IPTR)(size * fps), (IPTR)budget);
            }
        } else if (ioerr == 0) {
            UVCLOG(LOGF_Flag_Type_Information, "'%s': device default is format %lu frame %lu, not one of its supported frames; choosing the largest supported frame that fits",
                   nch->ncu_Name, (IPTR)probe[2], (IPTR)probe[3]);
        } else {
            UVCLOG(LOGF_Flag_Type_Warn, "'%s': GET_DEF probe failed (%ld); choosing the largest supported frame that fits",
                   nch->ncu_Name, (IPTR)ioerr);
        }
        if (chosen < 0) {
            ULONG bestarea = 0;
            for (i = 0; i < ncand; i++) {
                ULONG size = cand[i].maxbuf ? cand[i].maxbuf : (ULONG)cand[i].w * cand[i].h * 2;
                ULONG fps = cand[i].interval ? 10000000UL / cand[i].interval : 10;
                ULONG area = (ULONG)cand[i].w * cand[i].h;
                if (!fps) fps = 1;
                if (size <= budget / fps && area > bestarea) {
                    bestarea = area;
                    chosen = i;
                }
            }
            reason = "largest frame the stream endpoint carries";
        }
        if (chosen >= 0) {
            nch->ncu_PixelFormat = nch->ncu_Modes[chosen].pixel_format;
            nch->ncu_IsMJPEG = nch->ncu_PixelFormat == UVC_MODE_MJPEG;
            nch->ncu_Width = cand[chosen].w;
            nch->ncu_Height = cand[chosen].h;
            nch->ncu_FormatIndex = cand[chosen].format;
            nch->ncu_FrameIndex = cand[chosen].frame;
            nch->ncu_Interval = cand[chosen].interval;
            nch->ncu_MaxFrameBuf = (ULONG)cand[chosen].w * cand[chosen].h *
                (nch->ncu_PixelFormat == UVC_MODE_GRAY8 ? 1 : 2);
            if (cand[chosen].maxbuf > nch->ncu_MaxFrameBuf && cand[chosen].maxbuf <= 4 * nch->ncu_MaxFrameBuf)
                nch->ncu_MaxFrameBuf = cand[chosen].maxbuf;
        }
    }
    nch->ncu_HaveStream = chosen >= 0 && nch->ncu_StreamEP != NULL;
done:
    if (!nch->ncu_VSInterface)
        UVCLOG(LOGF_Flag_Type_Warn, "'%s': no VideoStreaming interface", nch->ncu_Name);
    else if (!ncand)
        UVCLOG(LOGF_Flag_Type_Warn, "'%s': no uncompressed supported frame up to 1920x1080", nch->ncu_Name);
    else if (!nch->ncu_StreamEP)
        UVCLOG(LOGF_Flag_Type_Warn, "'%s': %lu supported frames but no bulk IN or isochronous IN alternate up to %lu bytes (%lu isochronous alternates)",
               nch->ncu_Name, (IPTR)ncand, (IPTR)altcap, (IPTR)alts_with_iso);
    else if (!nch->ncu_HaveStream)
        UVCLOG(LOGF_Flag_Type_Warn, "'%s': none of %lu supported frames fits the %s stream endpoint (%lu bytes/interval)",
               nch->ncu_Name, (IPTR)ncand, nch->ncu_IsBulk ? "bulk" : "isochronous", (IPTR)nch->ncu_StreamMaxPkt);
    else
        UVCLOG(LOGF_Flag_Type_Information, "'%s': YUY2 %lux%lu format %lu frame %lu interval %lu (%s; %lu supported frames offered), %s, largest alternate %lu bytes/interval (%lu isochronous alternates), frame buffer %lu",
               nch->ncu_Name, (IPTR)nch->ncu_Width, (IPTR)nch->ncu_Height,
               (IPTR)nch->ncu_FormatIndex, (IPTR)nch->ncu_FrameIndex, (IPTR)nch->ncu_Interval, reason, (IPTR)ncand,
               nch->ncu_IsBulk ? "bulk" : "isochronous", (IPTR)nch->ncu_StreamMaxPkt, (IPTR)alts_with_iso,
               (IPTR)nch->ncu_MaxFrameBuf);
    CloseLibrary(ps);
    return nch->ncu_HaveStream;
}

/* Every transfer shares a deadline for the whole grab, including controls.
 * A submitted pipe is collected exactly once, also after abort. */
static LONG uvc_transfer(struct NepUVCBase *nh, struct Library *ps, APTR pipe,
                         APTR data, ULONG size, struct MsgPort *mp,
                         struct timerequest *timer, const char *what)
{
    LONG err;
    if (CheckIO((struct IORequest *)timer)) {
        UVCLOG(LOGF_Flag_Type_Warn, "%s: not sent, grab deadline already passed", what);
        return -1;
    }
    psdSendPipe(pipe, data, size);
    while (!psdCheckPipe(pipe) && !CheckIO((struct IORequest *)timer))
        Wait((1UL << mp->mp_SigBit) |
             (1UL << timer->tr_node.io_Message.mn_ReplyPort->mp_SigBit));
    if (!psdCheckPipe(pipe)) psdAbortPipe(pipe);
    err = psdWaitPipe(pipe);
    if (err)
        UVCLOG(LOGF_Flag_Type_Warn, "%s: transfer error %ld, actual %lu of %lu%s", what,
               (IPTR)err, (IPTR)psdGetPipeActual(pipe), (IPTR)size,
               CheckIO((struct IORequest *)timer) ? " (grab deadline passed)" : "");
    return CheckIO((struct IORequest *)timer) ? -1 : err;
}

static BOOL uvc_alt(struct NepUVCBase *nh, struct Library *ps, APTR control,
                    APTR interface, struct MsgPort *mp, struct timerequest *timer)
{
    IPTR number = 0, alternate = 0;
    UBYTE current[8] = {0};
    psdGetAttrs(PGA_INTERFACE, interface, IFA_InterfaceNum, &number,
                IFA_AlternateNum, &alternate, TAG_END);
    psdPipeSetup(control, URTF_STANDARD|URTF_INTERFACE,
                 USR_SET_INTERFACE, alternate, number);
    if (uvc_transfer(nh, ps, control, NULL, 0, mp, timer, "SET_INTERFACE")) return FALSE;
    psdPipeSetup(control, URTF_IN|URTF_STANDARD|URTF_INTERFACE,
                 USR_GET_INTERFACE, 0, number);
    if (uvc_transfer(nh, ps, control, current, 1, mp, timer, "GET_INTERFACE")) return FALSE;
    if (psdGetPipeActual(control) != 1 || current[0] != alternate) {
        UVCLOG(LOGF_Flag_Type_Warn, "interface %lu: asked for alternate %lu, device reports %lu (%lu bytes)",
               number, alternate, (IPTR)current[0], (IPTR)psdGetPipeActual(control));
        return FALSE;
    }
    if (!psdSetAltInterface(NULL, interface)) {
        UVCLOG(LOGF_Flag_Type_Warn, "interface %lu alternate %lu: psdSetAltInterface failed (host controller endpoint setup)",
               number, alternate);
        return FALSE;
    }
    return TRUE;
}

/* A camera's running stream: an isochronous one assembles frames in the
 * done hook of a real-time iso handler, a bulk one in its process. Grabs
 * copy the newest complete frame. It stops after UVC_IDLE s without a grab. */
#define UVC_IDLE 3
#define UVC_PKTBUFS 64
struct UVCStream {
    struct NepUVCBase *nh;
    struct List waiters;            /* grabs waiting for a frame (changed under Disable) */
    struct Process *proc;
    volatile BOOL quit, done;       /* asked to stop; process gone */
    volatile ULONG users;           /* grabs inside (under nh_Lock) */
    volatile ULONG lastgrab;        /* seconds, of the last grab */
    struct PsdDevice *device;
    APTR streamep;
    ULONG size;                     /* bytes per transfer */
    BOOL isbulk, ismjpeg;
    UBYTE format, frame, pixfmt;
    ULONG interval;
    UWORD width, height;
    ULONG expected;                 /* frame buffer bytes */
    ULONG maxpayload;
    /* frames: written at interrupt level, read by grabs (indexes swapped under Disable) */
    UBYTE *fbuf[3];
    ULONG flen[3];
    volatile LONG wi, li, ri;       /* being filled, newest (-1 none), being copied (-1 none) */
    volatile ULONG seq, given;      /* frames completed, last seq handed out */
    struct uvc_payload asm_;        /* frame assembly state (interrupt level) */
    /* the real-time iso handler and its packet buffers */
    APTR rtiso;
    struct Hook reqhook, donehook, relhook;
    UBYTE *pktraw, *pkt;
    ULONG pktstride, reqidx;
    volatile ULONG packets, frames, broken, errors;
    ULONG id;
};

struct UVCWaiter {
    struct MinNode node;
    struct Task *task;
    ULONG sigmask;
};

static ULONG uvc_seconds(void)
{
    struct DateStamp ds;
    DateStamp(&ds);
    return ds.ds_Days * 86400 + ds.ds_Minute * 60 + ds.ds_Tick / TICKS_PER_SECOND;
}

/* Negotiate nch's mode and select the streaming alternate setting (caller holds the device lock). */
static struct UVCStream *uvc_stream_setup(struct NepClassUVC *nch)
{
    struct NepUVCBase *nh = nch->ncu_ClsBase;
    struct Library *ps = NULL;
    struct MsgPort *mp = NULL, *tp = NULL;
    struct timerequest *timer = NULL;
    APTR control = NULL;
    APTR streamalt = nch->ncu_StreamAltIf, streamep = nch->ncu_StreamEP;
    ULONG streamsize = nch->ncu_StreamMaxPkt;
    UBYTE probe[26];
    IPTR ifnum = 0, altnum = 0;
    ULONG maxpayload = 0, maxvideoframe = 0, expected, i;
    const char *stage = "allocate";
    BOOL timeropen = FALSE, timersent = FALSE, ok = FALSE;
    struct UVCStream *st = NULL;
    if (!nch->ncu_HaveStream) return NULL;
    ps = OpenLibrary("poseidon.library", 4);
    if (!ps || !(mp = CreateMsgPort()) || !(tp = CreateMsgPort())) goto out;
    timer = (struct timerequest *)CreateIORequest(tp, sizeof(*timer));
    if (!timer || OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)timer, 0)) goto out;
    timeropen = TRUE;
    timer->tr_node.io_Command = TR_ADDREQUEST;
    timer->tr_time.tv_secs = 3;
    timer->tr_time.tv_micro = 0;
    SendIO((struct IORequest *)timer);
    timersent = TRUE;
    control = psdAllocPipe(nch->ncu_Device, mp, NULL);
    if (!control) goto out;
    psdGetAttrs(PGA_INTERFACE, nch->ncu_VSInterface, IFA_InterfaceNum, &ifnum, TAG_END);
    stage = "probe-default";
    memset(probe, 0, sizeof(probe));
    psdPipeSetup(control, URTF_IN|URTF_CLASS|URTF_INTERFACE,
                 UVUDR_GET_CUR, UVVSCS_PROBE_CONTROL << 8, ifnum);
    if (uvc_transfer(nh, ps, control, probe, sizeof(probe), mp, timer, "GET_CUR probe (default)")) goto out;
    if (psdGetPipeActual(control) != sizeof(probe)) {
        UVCLOG(LOGF_Flag_Type_Warn, "GET_CUR probe returned %lu bytes, expected %lu",
               (IPTR)psdGetPipeActual(control), (IPTR)sizeof(probe));
        goto out;
    }
    stage = "probe-set";
    probe[0] = 1; probe[1] = 0;
    probe[2] = nch->ncu_FormatIndex; probe[3] = nch->ncu_FrameIndex;
    for (i=0; i<4; ++i) probe[4+i] = nch->ncu_Interval >> (8*i);
    psdPipeSetup(control, URTF_OUT|URTF_CLASS|URTF_INTERFACE,
                 UVUDR_SET_CUR, UVVSCS_PROBE_CONTROL << 8, ifnum);
    if (uvc_transfer(nh, ps, control, probe, sizeof(probe), mp, timer, "SET_CUR probe")) goto out;
    psdPipeSetup(control, URTF_IN|URTF_CLASS|URTF_INTERFACE,
                 UVUDR_GET_CUR, UVVSCS_PROBE_CONTROL << 8, ifnum);
    if (uvc_transfer(nh, ps, control, probe, sizeof(probe), mp, timer, "GET_CUR probe (negotiated)")) goto out;
    if (psdGetPipeActual(control) != sizeof(probe) ||
        probe[2] != nch->ncu_FormatIndex || probe[3] != nch->ncu_FrameIndex) {
        UVCLOG(LOGF_Flag_Type_Warn, "probe came back as format %lu frame %lu (%lu bytes); asked for format %lu frame %lu",
               (IPTR)probe[2], (IPTR)probe[3], (IPTR)psdGetPipeActual(control),
               (IPTR)nch->ncu_FormatIndex, (IPTR)nch->ncu_FrameIndex);
        goto out;
    }
    if (nch->ncu_ExactInterval &&
        (probe[4] | ((ULONG)probe[5]<<8) | ((ULONG)probe[6]<<16) |
         ((ULONG)probe[7]<<24)) != nch->ncu_Interval) {
        UVCLOG(LOGF_Flag_Type_Warn, "[uvc:%lu] camera rejected requested frame interval %lu",
            (IPTR)nch->ncu_Id, (IPTR)nch->ncu_Interval);
        goto out;
    }
    stage = "probe-validate";
    maxvideoframe = probe[18] | ((ULONG)probe[19]<<8) |
                    ((ULONG)probe[20]<<16) | ((ULONG)probe[21]<<24);
    maxpayload = probe[22] | ((ULONG)probe[23]<<8) |
                 ((ULONG)probe[24]<<16) | ((ULONG)probe[25]<<24);
    if (maxpayload < 2 || maxpayload > 1024*1024 ||
        (!nch->ncu_IsBulk && maxpayload > nch->ncu_StreamMaxPkt)) {
        UVCLOG(LOGF_Flag_Type_Warn, "device wants %lu bytes per payload, more than the largest alternate (%lu bytes/interval)",
               (IPTR)maxpayload, (IPTR)nch->ncu_StreamMaxPkt);
        goto out;
    }
    psdPipeSetup(control, URTF_OUT|URTF_CLASS|URTF_INTERFACE,
                 UVUDR_SET_CUR, UVVSCS_COMMIT_CONTROL << 8, ifnum);
    if (uvc_transfer(nh, ps, control, probe, sizeof(probe), mp, timer, "SET_CUR commit")) goto out;
    if (!nch->ncu_IsBulk) {
        struct List *alts = NULL;
        struct Node *node;
        psdGetAttrs(PGA_INTERFACE, nch->ncu_VSInterface,
                    IFA_AlternateIfList, &alts, TAG_END);
        if (alts) for (node=alts->lh_Head; node->ln_Succ; node=node->ln_Succ) {
            APTR ep = psdFindEndpoint((APTR)node, NULL, EA_IsIn, TRUE,
                                      EA_TransferType, USEAF_ISOCHRONOUS, TAG_END);
            IPTR packet = 0, transactions = 1;
            ULONG bytes;
            if (!ep) continue;
            psdGetAttrs(PGA_ENDPOINT, ep, EA_MaxPktSize, &packet,
                        EA_NumTransMuFrame, &transactions, TAG_END);
            bytes = packet * transactions;
            if (bytes >= maxpayload && bytes < streamsize) {
                streamsize = bytes;
                streamalt = (APTR)node;
                streamep = ep;
            }
        }
        stage = "alternate";
        /* psdSetAltInterface updates Poseidon's selected-interface bookkeeping. */
        psdGetAttrs(PGA_INTERFACE, streamalt, IFA_AlternateNum, &altnum, TAG_END);
        if (!uvc_alt(nh, ps, control, streamalt, mp, timer)) goto out;
    }
    /* Publish only validated commit data, under the inventory lock. */
    ObtainSemaphore(&nh->nh_Lock);
    {
        struct NepClassUVC *bound;
        ForeachNode(&nh->nh_Cameras, bound) {
            if (bound->ncu_Id == nch->ncu_Id) {
                struct UVCDeviceInfo *info = &bound->ncu_Info;
                info->negotiated = TRUE;
                info->width = nch->ncu_Width;
                info->height = nch->ncu_Height;
                info->format_index = probe[2]; info->frame_index = probe[3];
                info->interval = probe[4] | ((ULONG)probe[5] << 8) |
                    ((ULONG)probe[6] << 16) | ((ULONG)probe[7] << 24);
                info->payload_bytes = maxpayload;
                info->frame_bytes = maxvideoframe;
                info->alternate = altnum;
                break;
            }
        }
    }
    ReleaseSemaphore(&nh->nh_Lock);
    /* The frame is as large as the device committed to, not as large as the
     * geometry implies; the picture is the first width*height*2 bytes. */
    expected = nch->ncu_MaxFrameBuf;
    if (maxvideoframe > expected && maxvideoframe <= 4 * (ULONG)nch->ncu_Width * nch->ncu_Height * 2)
        expected = maxvideoframe;
    stage = "stream";
    if (!(st = AllocVec(sizeof(*st), MEMF_PUBLIC | MEMF_CLEAR))) goto out;
    for (i = 0; i < 3; i++)
        if (!(st->fbuf[i] = AllocVec(expected, MEMF_PUBLIC))) goto out;
    st->nh = nh;
    NewList(&st->waiters);
    st->wi = 0;
    st->li = st->ri = -1;
    st->device = nch->ncu_Device;
    st->streamep = nch->ncu_IsBulk ? nch->ncu_StreamEP : streamep;
    st->size = nch->ncu_IsBulk ? maxpayload : streamsize;
    st->isbulk = nch->ncu_IsBulk;
    st->ismjpeg = nch->ncu_IsMJPEG;
    st->format = nch->ncu_FormatIndex;
    st->frame = nch->ncu_FrameIndex;
    st->pixfmt = nch->ncu_PixelFormat;
    st->interval = nch->ncu_Interval;
    st->width = nch->ncu_Width;
    st->height = nch->ncu_Height;
    st->expected = expected;
    st->maxpayload = maxpayload;
    st->id = nch->ncu_Id;
    st->lastgrab = uvc_seconds();
    ok = TRUE;
    UVCLOG(LOGF_Flag_Type_Information, "[uvc:%lu] streaming %lux%lu format %lu/frame %lu via %s alt %lu, %lu bytes/transfer",
           (IPTR)nch->ncu_Id, (IPTR)nch->ncu_Width, (IPTR)nch->ncu_Height, (IPTR)probe[2], (IPTR)probe[3],
           nch->ncu_IsBulk ? "bulk" : "isochronous", altnum, (IPTR)st->size);
out:
    if (!ok) {
        UVCLOG(LOGF_Flag_Type_Error, "[uvc:%lu] '%s' could not start streaming at %s",
               (IPTR)nch->ncu_Id, nch->ncu_Name, stage);
        if (st) { for (i = 0; i < 3; i++) FreeVec(st->fbuf[i]); FreeVec(st); st = NULL; }
    }
    if (control) psdFreePipe(control);
    if (timersent) {
        if (!CheckIO((struct IORequest *)timer)) AbortIO((struct IORequest *)timer);
        WaitIO((struct IORequest *)timer);
    }
    if (timeropen) CloseDevice((struct IORequest *)timer);
    if (timer) DeleteIORequest((struct IORequest *)timer);
    if (tp) DeleteMsgPort(tp);
    if (mp) DeleteMsgPort(mp);
    if (ps) CloseLibrary(ps);
    return st;
}

/* A complete frame in fbuf[wi]: it becomes the newest, and filling moves to the buffer that is neither the newest
 * nor being copied. Interrupt level, or a task under Disable. */
static void uvc_publish(struct UVCStream *st, ULONG len)
{
    struct UVCWaiter *w;
    LONG next;
    st->flen[st->wi] = len;
    st->li = st->wi;
    for (next = 0; next == st->li || next == st->ri; next++)
        ;
    st->wi = next;
    st->asm_.data = st->fbuf[next];
    st->asm_.used = 0;
    st->seq++;
    st->frames++;
    ForeachNode(&st->waiters, w) Signal(w->task, w->sigmask);
}

AROS_UFH3(void, uvc_iso_request,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(struct IOUsbHWRTIso *, urti, A2),
          AROS_UFHA(struct IOUsbHWBufferReq *, ubr, A1))
{
    AROS_USERFUNC_INIT
    struct UVCStream *st = hook->h_Data;
    ubr->ubr_Buffer = st->pkt + (st->reqidx++ % UVC_PKTBUFS) * st->pktstride;
    ubr->ubr_Length = st->size;
    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, uvc_iso_done,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(struct IOUsbHWRTIso *, urti, A2),
          AROS_UFHA(struct IOUsbHWBufferReq *, ubr, A1))
{
    AROS_USERFUNC_INIT
    struct UVCStream *st = hook->h_Data;
    ULONG len = ubr->ubr_Length;
    st->packets++;
    if (len && len <= st->size && ubr->ubr_Buffer) {
        if (uvc_payload_feed(&st->asm_, ubr->ubr_Buffer, len)) uvc_publish(st, st->asm_.used);
        else if (st->asm_.saw_eof && st->asm_.damaged && (ubr->ubr_Buffer[1] & 2)) st->broken++;
    }
    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, uvc_iso_release,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(APTR, prt, A2),
          AROS_UFHA(APTR, unused, A1))
{
    AROS_USERFUNC_INIT
    struct UVCStream *st = hook->h_Data;
    st->errors++;           /* the device went away: the process stops the stream */
    st->quit = TRUE;
    Signal(&st->proc->pr_Task, SIGBREAKF_CTRL_C);
    AROS_USERFUNC_EXIT
}

/* The stream's process: starts the real-time iso handler (or runs the bulk transfers), stops it when idle. */
static void uvc_stream_proc(void)
{
    struct UVCStream *st = (struct UVCStream *)FindTask(NULL)->tc_UserData;
    struct NepUVCBase *nh = st->nh;
    struct Library *ps = OpenLibrary("poseidon.library", 4);
    struct MsgPort *mp = CreateMsgPort(), *tp = CreateMsgPort();
    struct timerequest *timer = tp ? (struct timerequest *)CreateIORequest(tp, sizeof(*timer)) : NULL;
    APTR pipes[2] = {0};
    BOOL pending[2] = {0}, timeropen = FALSE, timersent = FALSE, started = FALSE;
    struct uvc_bulk_payload bulk = {0};
    UBYTE *bulkbuf[2] = {0};
    ULONG slot = 0, i;
    if (!ps || !mp || !timer || OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)timer, 0)) goto out;
    timeropen = TRUE;
    st->asm_.data = st->fbuf[st->wi];
    st->asm_.capacity = st->expected;
    st->asm_.variable_length = st->ismjpeg;
    if (!st->isbulk) {
        /* packet buffers: a power-of-two stride from a 64 KB aligned base, so no transfer crosses 64 KB */
        st->pktstride = 1024;
        while (st->pktstride < st->size) st->pktstride <<= 1;
        if (!(st->pktraw = AllocVec(st->pktstride * UVC_PKTBUFS + 65536, MEMF_PUBLIC))) goto out;
        st->pkt = (UBYTE *)(((IPTR)st->pktraw + 65535) & ~(IPTR)65535);
        st->reqhook.h_Entry = (HOOKFUNC)uvc_iso_request;
        st->reqhook.h_Data = st;
        st->donehook.h_Entry = (HOOKFUNC)uvc_iso_done;
        st->donehook.h_Data = st;
        st->relhook.h_Entry = (HOOKFUNC)uvc_iso_release;
        st->relhook.h_Data = st;
        st->rtiso = psdAllocRTIsoHandler(st->streamep, RTA_InRequestHook, (IPTR)&st->reqhook,
                                         RTA_InDoneHook, (IPTR)&st->donehook, RTA_ReleaseHook, (IPTR)&st->relhook,
                                         TAG_END);
        if (!st->rtiso || psdStartRTIso(st->rtiso)) {
            UVCLOG(LOGF_Flag_Type_Error, "[uvc:%lu] could not start the real-time iso handler", (IPTR)st->id);
            goto out;
        }
        started = TRUE;
    } else {
        bulk.capacity = st->maxpayload;
        if (!(bulk.data = AllocVec(st->maxpayload, MEMF_PUBLIC))) goto out;
        for (i = 0; i < 2; i++) {
            if (!(bulkbuf[i] = AllocVec(st->size, MEMF_PUBLIC)) ||
                !(pipes[i] = psdAllocPipe(st->device, mp, st->streamep))) goto out;
            psdSetAttrs(PGA_PIPE, pipes[i], PPA_AllowRuntPackets, TRUE, PPA_NakTimeout, TRUE,
                        PPA_NakTimeoutTime, 1000, TAG_END);
            psdSendPipe(pipes[i], bulkbuf[i], st->size);
            pending[i] = TRUE;
        }
    }
    timer->tr_node.io_Command = TR_ADDREQUEST;
    while (!st->quit) {
        if (!timersent) {
            timer->tr_time.tv_secs = 0;
            timer->tr_time.tv_micro = 200000;
            SendIO((struct IORequest *)timer);
            timersent = TRUE;
        }
        if (st->isbulk) {
            while (!psdCheckPipe(pipes[slot]) && !CheckIO((struct IORequest *)timer))
                Wait((1UL << mp->mp_SigBit) | (1UL << tp->mp_SigBit) | SIGBREAKF_CTRL_C);
            if (psdCheckPipe(pipes[slot])) {
                LONG error = psdWaitPipe(pipes[slot]);
                pending[slot] = FALSE;
                if (error) st->errors++;
                else {
                    ULONG actual = psdGetPipeActual(pipes[slot]);
                    st->packets++;
                    if (uvc_bulk_feed(&bulk, &st->asm_, bulkbuf[slot], actual, st->size)) {
                        Disable();
                        uvc_publish(st, st->asm_.used);
                        Enable();
                    }
                }
                psdSendPipe(pipes[slot], bulkbuf[slot], st->size);
                pending[slot] = TRUE;
                slot ^= 1;
            }
        } else {
            Wait((1UL << tp->mp_SigBit) | SIGBREAKF_CTRL_C);
        }
        if (CheckIO((struct IORequest *)timer)) {
            WaitIO((struct IORequest *)timer);
            timersent = FALSE;
            if (uvc_seconds() - st->lastgrab > UVC_IDLE) break;
        }
    }
out:
    if (started) psdStopRTIso(st->rtiso);
    if (st->rtiso) psdFreeRTIsoHandler(st->rtiso);
    UVCLOG(LOGF_Flag_Type_Information, "[uvc:%lu] stream stopped: %lu packets, %lu frames, %lu broken, %lu errors%s",
           (IPTR)st->id, (IPTR)st->packets, (IPTR)st->frames, (IPTR)st->broken, (IPTR)st->errors,
           st->quit ? " (asked)" : "");
    for (i = 0; i < 2; i++) {
        if (pending[i]) { psdAbortPipe(pipes[i]); psdWaitPipe(pipes[i]); }
        if (pipes[i]) psdFreePipe(pipes[i]);
        FreeVec(bulkbuf[i]);
    }
    if (timersent) { if (!CheckIO((struct IORequest *)timer)) AbortIO((struct IORequest *)timer); WaitIO((struct IORequest *)timer); }
    if (timeropen) CloseDevice((struct IORequest *)timer);
    if (timer) DeleteIORequest((struct IORequest *)timer);
    if (tp) DeleteMsgPort(tp);
    if (mp) DeleteMsgPort(mp);
    FreeVec(bulk.data);
    if (ps) CloseLibrary(ps);
    Forbid();                           /* done is seen only after this process can no longer run */
    st->done = TRUE;
    {
        struct UVCWaiter *w;
        Disable();
        ForeachNode(&st->waiters, w) Signal(w->task, w->sigmask);
        Enable();
    }
}

static BOOL uvc_stream_start(struct UVCStream *st)
{
    st->proc = CreateNewProcTags(NP_Entry, (IPTR)uvc_stream_proc, NP_Name, (IPTR)"uvc.class stream",
                                 NP_Priority, 1, NP_StackSize, 32768, NP_UserData, (IPTR)st, TAG_END);
    return st->proc != NULL;
}

static void uvc_stream_free(struct UVCStream *st)
{
    int i;
    for (i = 0; i < 3; i++) FreeVec(st->fbuf[i]);
    FreeVec(st->pktraw);
    FreeVec(st);
}

/* Stop st and free it once its process and its last grab are gone. */
static void uvc_stream_stop(struct NepUVCBase *nh, struct UVCStream *st)
{
    BOOL idle;
    st->quit = TRUE;
    for (;;) {
        Disable();
        {
            struct UVCWaiter *w;
            ForeachNode(&st->waiters, w) Signal(w->task, w->sigmask);
        }
        if (st->proc && !st->done) Signal(&st->proc->pr_Task, SIGBREAKF_CTRL_C);
        Enable();
        ObtainSemaphore(&nh->nh_Lock);
        idle = (st->done || !st->proc) && !st->users;
        ReleaseSemaphore(&nh->nh_Lock);
        if (idle) break;
        Delay(1);
    }
    uvc_stream_free(st);
}

/* A frame newer than the last one handed out, decoded into rgb; 0 if none came within 3 s. */
static ULONG uvc_stream_frame(struct NepUVCBase *nh, struct UVCStream *st, UBYTE *rgb, ULONG capacity)
{
    struct UVCWaiter w;
    BYTE sig = AllocSignal(-1);
    ULONG start = uvc_seconds(), len = 0, result = 0, pixels = (ULONG)st->width * st->height;
    LONG r = -1;
    UBYTE *raw;
    if (sig < 0 || capacity < pixels * 3 || !(raw = AllocVec(st->expected, MEMF_PUBLIC))) {
        if (sig >= 0) FreeSignal(sig);
        return 0;
    }
    w.task = FindTask(NULL);
    w.sigmask = 1UL << sig;
    Disable();
    AddTail(&st->waiters, (struct Node *)&w.node);
    while (st->seq == st->given && !st->quit && !st->done && uvc_seconds() - start < 3) {
        Enable();
        st->lastgrab = uvc_seconds();
        Wait(w.sigmask | SIGBREAKF_CTRL_C);
        Disable();
    }
    if (st->seq != st->given && st->li >= 0) {
        r = st->ri = st->li;            /* the interrupt now fills neither this one nor the newest */
        len = st->flen[r];
        st->given = st->seq;
    }
    Remove((struct Node *)&w.node);
    Enable();
    if (r >= 0) {
        memcpy(raw, st->fbuf[r], len);
        Disable();
        st->ri = -1;
        Enable();
    }
    st->lastgrab = uvc_seconds();
    FreeSignal(sig);
    if (len) {
        if (st->ismjpeg) {
            int reason = 0;
            if (uvc_jpeg_rgb(raw, len, rgb, capacity, st->width, st->height, &reason)) result = pixels * 3;
            else UVCLOG(LOGF_Flag_Type_Warn, "[uvc:%lu] JPEG rejected %lu bytes: reason %ld",
                        (IPTR)st->id, (IPTR)len, (IPTR)reason);
        } else if (st->pixfmt == UVC_MODE_GRAY8 || st->pixfmt == UVC_MODE_GRAY16) {
            uvc_gray_rgb(raw, rgb, pixels, st->pixfmt == UVC_MODE_GRAY16);
            result = pixels * 3;
        } else {
            uvc_yuy2_rgb(raw, rgb, pixels * 2);
            result = pixels * 3;
        }
    }
    FreeVec(raw);
    if (result) nh->nh_FramesOK++;
    else nh->nh_FramesFailed++;
    return result;
}

static IPTR uvc_method(struct NepUVCBase *nh, ULONG method, IPTR *args)
{
    struct NepClassUVC *nch, snapshot;
    struct Library *ps = NULL;
    ULONG index = 0;
    IPTR result = 0;
    BOOL found = FALSE;
    if (method == UVCM_GRAB || method == UVCM_GRABMODE || method == UVCM_GRABRATE) {
        ps = OpenLibrary("poseidon.library", 4);
        if (!ps) return 0;
        psdLockReadPBase();
    }
    ObtainSemaphore(&nh->nh_Lock);
    ForeachNode(&nh->nh_Cameras, nch) {
        if (method == UVCM_COUNT) { result++; continue; }
        if (!args) break;
        if (method == UVCM_INFO && index++ == args[0]) {
            struct UVCCameraInfo *info = (APTR)args[1];
            if (info) {
                memset(info, 0, sizeof(*info));
                info->id = nch->ncu_Id;
                info->width = nch->ncu_Width; info->height = nch->ncu_Height;
                memcpy(info->name, nch->ncu_Name, sizeof(info->name));
                result = TRUE;
            }
            break;
        }
        if (method == UVCM_DEVICEINFO && nch->ncu_Id == args[0]) {
            if (args[1] && args[2] >= sizeof(nch->ncu_Info)) {
                memcpy((APTR)args[1], &nch->ncu_Info, sizeof(nch->ncu_Info));
                result = TRUE;
            }
            break;
        }
        if (method == UVCM_RATES && nch->ncu_Id == args[0]) {
            if (args[1] < nch->ncu_ModeCount && args[2] && args[3] >= sizeof(struct UVCRates)) {
                memcpy((APTR)args[2], &nch->ncu_Rates[args[1]], sizeof(struct UVCRates));
                result = TRUE;
            }
            break;
        }
        if (method == UVCM_MODECOUNT && nch->ncu_Id == args[0]) {
            result = nch->ncu_ModeCount;
            break;
        }
        if (method == UVCM_MODEINFO && nch->ncu_Id == args[0]) {
            if (args[1] < nch->ncu_ModeCount && args[2] &&
                args[3] >= sizeof(struct UVCModeInfo)) {
                memcpy((APTR)args[2], &nch->ncu_Modes[args[1]],
                       sizeof(struct UVCModeInfo));
                result = TRUE;
            }
            break;
        }
        if ((method == UVCM_GRAB || method == UVCM_GRABMODE || method == UVCM_GRABRATE) &&
            nch->ncu_Id == args[0]) {
            snapshot = *nch;
            if (method == UVCM_GRABMODE || method == UVCM_GRABRATE) {
                const struct UVCModeInfo *mode;
                ULONG geometry;
                if (args[3] >= nch->ncu_ModeCount) break;
                mode = &nch->ncu_Modes[args[3]];
                if (mode->pixel_format < UVC_MODE_YUY2 || mode->pixel_format > UVC_MODE_GRAY16) break;
                snapshot.ncu_PixelFormat = mode->pixel_format;
                snapshot.ncu_IsMJPEG = mode->pixel_format == UVC_MODE_MJPEG;
                snapshot.ncu_Width = mode->width;
                snapshot.ncu_Height = mode->height;
                snapshot.ncu_FormatIndex = mode->format_index;
                snapshot.ncu_FrameIndex = mode->frame_index;
                snapshot.ncu_Interval = mode->capture_interval;
                if (method == UVCM_GRABRATE && args[4]) {
                    const struct UVCRates *rates = &nch->ncu_Rates[args[3]];
                    ULONG j, wanted = args[4];
                    BOOL valid = FALSE;
                    for (j = 0; j < rates->count; j++)
                        if (wanted == rates->intervals[j]) valid = TRUE;
                    if (!rates->count && rates->minimum && rates->step &&
                        wanted >= rates->minimum && wanted <= rates->maximum &&
                        (wanted-rates->minimum) % rates->step == 0) valid = TRUE;
                    if (!valid) break;
                    snapshot.ncu_Interval = wanted;
                    snapshot.ncu_ExactInterval = TRUE;
                }
                geometry = mode->width * mode->height * (mode->pixel_format == UVC_MODE_GRAY8 ? 1 : 2);
                snapshot.ncu_MaxFrameBuf = geometry;
                if (mode->max_frame_bytes > geometry &&
                    mode->max_frame_bytes <= 4 * geometry)
                    snapshot.ncu_MaxFrameBuf = mode->max_frame_bytes;
            }
            found = TRUE;
            break;
        }
    }
    ReleaseSemaphore(&nh->nh_Lock);
    if (ps) {
        APTR pd = NULL;
        /* The base read lock pins the active inventory while acquiring the
         * device lock. Never wait for a device lock with the class lock held:
         * binding release runs under the device lock and takes the class lock. */
        if (found) {
            while ((pd = psdGetNextDevice(pd)))
                if (pd == snapshot.ncu_Device) break;
        }
        if (pd) psdLockWriteDevice(pd);
        psdUnlockPBase();
        if (pd) {
            /* A manual unbind or configuration switch may have won while
             * acquiring the device lock. Revalidate before using descriptors. */
            found = FALSE;
            ObtainSemaphore(&nh->nh_Lock);
            ForeachNode(&nh->nh_Cameras, nch) {
                if (nch->ncu_Id == args[0]) { found = TRUE; break; }
            }
            ReleaseSemaphore(&nh->nh_Lock);
            if (found) {
                /* The stream of this binding: kept when it runs in the mode
                 * asked for, otherwise replaced; started under the device lock. */
                struct UVCStream *st = nch->ncu_Stream, *old = NULL;
                if (st && (st->done || st->format != snapshot.ncu_FormatIndex ||
                           st->frame != snapshot.ncu_FrameIndex || st->interval != snapshot.ncu_Interval ||
                           st->pixfmt != snapshot.ncu_PixelFormat)) {
                    old = st;
                    st = NULL;
                    nch->ncu_Stream = NULL;
                }
                if (old) uvc_stream_stop(nh, old);
                if (!st && (st = uvc_stream_setup(&snapshot))) {
                    if (uvc_stream_start(st)) nch->ncu_Stream = st;
                    else { uvc_stream_free(st); st = NULL; }
                }
                if (st) {
                    ObtainSemaphore(&nh->nh_Lock);
                    st->users++;
                    ReleaseSemaphore(&nh->nh_Lock);
                }
                psdUnlockDevice(pd);
                pd = NULL;
                if (st) {
                    result = uvc_stream_frame(nh, st, (APTR)args[1], args[2]);
                    ObtainSemaphore(&nh->nh_Lock);
                    st->users--;
                    ReleaseSemaphore(&nh->nh_Lock);
                }
            }
            if (pd) psdUnlockDevice(pd);
        }
        CloseLibrary(ps);
    }
    return result;
}

/*
 * ***********************************************************************
 * * Library functions                                                   *
 * ***********************************************************************
 */

/* /// "usbAttemptInterfaceBinding()" */
struct NepClassUVC * usbAttemptInterfaceBinding(struct NepUVCBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    IPTR ifclass;
    IPTR subclass;

    KPRINTF(1, ("nepUVCAttemptInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Class, &ifclass,
                    IFA_SubClass, &subclass,
                    TAG_DONE);
        CloseLibrary(ps);

        /* Bind on the VideoControl interface; the associated VideoStreaming
         * interface(s) are claimed via the interface association once streaming
         * is implemented. */
        if((ifclass == VIDEO_CLASSCODE) && (subclass == VIDEO_CTRL_SUBCLASS))
        {
            return(usbForceInterfaceBinding(nh, pif));
        }
    }
    return(NULL);
}
/* \\\ */

/* /// "usbForceInterfaceBinding()" */
struct NepClassUVC * usbForceInterfaceBinding(struct NepUVCBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;
    struct NepClassUVC *nch;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    struct PsdDevice *hubpd;
    STRPTR devname;
    STRPTR ifidstr;
    STRPTR devidstr;
    IPTR ifnum;
    IPTR hubport;

    KPRINTF(1, ("nepUVCForceInterfaceBinding(%08lx)\n", pif));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Config, &pc,
                    IFA_IDString, &ifidstr,
                    IFA_InterfaceNum, &ifnum,
                    TAG_END);
        psdGetAttrs(PGA_CONFIG, pc,
                    CA_Device, &pd,
                    TAG_END);
        psdGetAttrs(PGA_DEVICE, pd,
                    DA_ProductName, &devname,
                    DA_IDString, &devidstr,
                    DA_HubDevice, &hubpd,
                    DA_AtHubPortNumber, &hubport,
                    TAG_END);

        if((nch = psdAllocVec(sizeof(struct NepClassUVC))))
        {
            memset(nch, 0, sizeof(*nch));
            if (devname) strncpy(nch->ncu_Name, devname, sizeof(nch->ncu_Name)-1);
            nch->ncu_ClsBase = nh;
            nch->ncu_Device = pd;
            nch->ncu_Hub = hubpd;
            nch->ncu_Config = pc;
            nch->ncu_Interface = pif;
            nch->ncu_HubPort = hubport;
            nch->ncu_IfNum = ifnum;
            nch->ncu_DevIDString = devidstr;
            nch->ncu_IfIDString = ifidstr;

            if (!uvc_parse_streaming(nch)) {
                psdAddErrorMsg(RETURN_WARN, (STRPTR)libname,
                    "No supported video mode (up to 3840x2160) on '%s'", devname);
                psdFreeVec(nch);
                CloseLibrary(ps);
                return NULL;
            }
            {
                IPTR vid = 0, pid = 0, low = 0, high = 0, super = 0, vs = 0;
                psdGetAttrs(PGA_DEVICE, pd, DA_VendorID, &vid,
                    DA_ProductID, &pid, DA_IsLowspeed, &low,
                    DA_IsHighspeed, &high, DA_IsSuperspeed, &super, TAG_END);
                psdGetAttrs(PGA_INTERFACE, nch->ncu_VSInterface,
                    IFA_InterfaceNum, &vs, TAG_END);
                nch->ncu_Info.size = sizeof(nch->ncu_Info);
                nch->ncu_Info.vendor_id = vid;
                nch->ncu_Info.product_id = pid;
                nch->ncu_Info.speed = super ? UVC_SPEED_SUPER : high ?
                    UVC_SPEED_HIGH : low ? UVC_SPEED_LOW : UVC_SPEED_FULL;
                nch->ncu_Info.control_interface = ifnum;
                nch->ncu_Info.streaming_interface = vs;
                nch->ncu_Info.bulk = nch->ncu_IsBulk;
                nch->ncu_Info.mode_count = nch->ncu_ModeCount;
            }
            ObtainSemaphore(&nh->nh_Lock);
            nch->ncu_Id = ++nh->nh_NextId;
            nch->ncu_Info.binding_id = nch->ncu_Id;
            AddTail(&nh->nh_Cameras, &nch->ncu_Node);
            ReleaseSemaphore(&nh->nh_Lock);
            UVCLOG(LOGF_Flag_Type_Information, "bound id=%lu '%s' (interface %lu, hub port %lu) video %lux%lu",
                   (IPTR)nch->ncu_Id, devname, (IPTR)ifnum, (IPTR)hubport,
                   (IPTR)nch->ncu_Width, (IPTR)nch->ncu_Height);
            CloseLibrary(ps);
            return(nch);
        }
        CloseLibrary(ps);
    }
    return(NULL);
}
/* \\\ */

/* /// "usbReleaseInterfaceBinding()" */
void usbReleaseInterfaceBinding(struct NepUVCBase *nh, struct NepClassUVC *nch)
{
    struct Library *ps;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    STRPTR devname;

    KPRINTF(1, ("nepUVCReleaseInterfaceBinding(%08lx)\n", nch));
    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        psdGetAttrs(PGA_INTERFACE, nch->ncu_Interface, IFA_Config, &pc, TAG_END);
        psdGetAttrs(PGA_CONFIG, pc, CA_Device, &pd, TAG_END);
        psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname,
                       "Released USB Video device '%s'!", devname);
        ObtainSemaphore(&nh->nh_Lock);
        Remove(&nch->ncu_Node);
        ReleaseSemaphore(&nh->nh_Lock);
        if (nch->ncu_Stream) uvc_stream_stop(nh, nch->ncu_Stream);
        psdFreeVec(nch);
        CloseLibrary(ps);
    }
}
/* \\\ */

/* /// "usbGetAttrsA()" */
AROS_LH3(LONG, usbGetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, usbstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 5, nep)
{
    AROS_LIBFUNC_INIT

    struct TagItem *ti;
    LONG count = 0;

    KPRINTF(1, ("nepUVCGetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
    switch(type)
    {
        case UGA_CLASS:
            if((ti = FindTagItem(UCCA_Priority, tags)))
            {
                *((SIPTR *) ti->ti_Data) = -100;
                count++;
            }
            if((ti = FindTagItem(UCCA_Description, tags)))
            {
                *((STRPTR *) ti->ti_Data) = "USB Video Class (webcams, capture)";
                count++;
            }
            if((ti = FindTagItem(UCCA_HasClassCfgGUI, tags)))
            {
                *((IPTR *) ti->ti_Data) = FALSE;
                count++;
            }
            if((ti = FindTagItem(UCCA_HasBindingCfgGUI, tags)))
            {
                *((IPTR *) ti->ti_Data) = FALSE;
                count++;
            }
            if((ti = FindTagItem(UCCA_AfterDOSRestart, tags)))
            {
                *((IPTR *) ti->ti_Data) = FALSE;
                count++;
            }
            if((ti = FindTagItem(UCCA_UsingDefaultCfg, tags)))
            {
                *((IPTR *) ti->ti_Data) = TRUE;
                count++;
            }
            break;

        case UGA_BINDING:
            if((ti = FindTagItem(UCBA_UsingDefaultCfg, tags)))
            {
                *((IPTR *) ti->ti_Data) = TRUE;
                count++;
            }
            break;
    }
    return(count);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "usbSetAttrsA()" */
AROS_LH3(LONG, usbSetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, usbstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 6, nep)
{
    AROS_LIBFUNC_INIT
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "usbDoMethodA()" */
AROS_LH2(IPTR, usbDoMethodA,
         AROS_LHA(ULONG, methodid, D0),
         AROS_LHA(IPTR *, methoddata, A1),
         LIBBASETYPEPTR, nh, 7, nep)
{
    AROS_LIBFUNC_INIT

    KPRINTF(10, ("Do Method %ld\n", methodid));
    if (methodid >= UVCM_COUNT && methodid <= UVCM_GRABRATE)
        return uvc_method(nh, methodid, methoddata);
    switch(methodid)
    {
        case UCM_AttemptInterfaceBinding:
            return((IPTR) usbAttemptInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ForceInterfaceBinding:
            return((IPTR) usbForceInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ReleaseInterfaceBinding:
            usbReleaseInterfaceBinding(nh, (struct NepClassUVC *) methoddata[0]);
            return(TRUE);

        default:
            return(0);
    }
    AROS_LIBFUNC_EXIT
}
/* \\\ */
