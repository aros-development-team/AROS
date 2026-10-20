#ifndef BTAUDIO_H
#define BTAUDIO_H

#include <exec/libraries.h>
#include <exec/rawfmt.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include <btcore/sbc.h>
#include <btcore/a2dp_source.h>

#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <devices/ahi.h>
#include <libraries/ahi_sub.h>
#include <libraries/bluetooth.h>
#include <libraries/btclass.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/bluetooth.h>
#include <string.h>

struct BtaLink;
struct BTAudioClass;
struct BluetoothAudioBase {
    struct Library library;
    struct BTAudioClass *cls;
};
struct BTAudioClass {
    struct Library library;
    struct Library *utility;
    struct DosLibrary *dos;
    struct BluetoothAudioBase *audio;
    struct SignalSemaphore lock;
    struct List units;
};
#define UtilityBase (AHIsubBase->cls->utility)
#define DOSBase (AHIsubBase->cls->dos)
#define LibName "btaudio.audio"
#define LibIDString "btaudio.audio 6.1 (19.10.2026)"


/* PCM waiting to be encoded: interleaved stereo, 16 bit. */
#define BTA_FIFO_FRAMES   8192
/* SBC frames per media packet; 5 frames of 119 bytes plus headers stay
   below the default 672 byte L2CAP MTU. */
#define BTA_MAX_FRAMES    15

struct BluetoothAudioData {
    struct BluetoothAudioBase *ahisubbase;
    APTR                mixbuffer;
    struct BtaLink     *unit;
    ULONG               mixoffset;
    ULONG               mixleft;
    ULONG               volume;        /* output volume, Fixed (0x10000 = 0 dB) */
};

/* Each AudioSink service binding owns a transport and an AHI mode.
   Detached units remain until class expunge so open AHI clients stay valid. */
struct BtaLink {
    struct Node node;
    APTR service;
    ULONG modeid;
    BOOL attached;
    BOOL binding;
    ULONG mixfreq;
    struct MsgPort ahiport;
    struct AHIRequest ahireq;
    BOOL ahiopen;
    struct SignalSemaphore modelock;
    char label[96];
    ULONG modefreq;
    ULONG generation;
    ULONG setupseq;
    ULONG playseq;
    struct AHIAudioCtrlDrv *owner;
    struct Task *stopper;
    BYTE stopsig;
    struct SignalSemaphore lock;       /* held while ctrl is used or changed */
    struct Task        *task;
    struct Task        *creator;
    BYTE                creatorsig;
    BYTE                wakesig;
    struct AHIAudioCtrlDrv *ctrl;      /* the one playing, NULL when stopped */
    UBYTE               want_address[18];
    ULONG               want_freq;
    BOOL                quit;
    BOOL                transport_stopped;
    BOOL                preparing;
    BOOL                ready;

    /* owned by the link process */
    struct BluetoothAudioBase *ahisubbase;
    struct Library     *btbase;
    struct MsgPort     *port;          /* channel replies */
    struct MsgPort     *timerport;
    struct timerequest *timereq;
    BOOL                timeropen;
    UBYTE               address[18];   /* sink of the current session */
    ULONG               freq;          /* mixing frequency of the current session */
    BOOL                session;       /* signalling channels up and A2DP started */
    BOOL                suspendpending;
    BOOL                broken;        /* the sink went away: close and reconnect */
    APTR                device;
    APTR                sigep;
    APTR                mediaep;
    APTR                sigread;
    APTR                sigwrite;
    APTR                mediaread;
    APTR                mediawrite;
    BOOL                sigreadpending;
    BOOL                mediareadpending;
    BOOL                mediawritepending;
    BOOL                wantmedia;     /* OPEN accepted: bring the transport channel up */
    ULONG               openticks;     /* ticks the transport channel has been open */
    UBYTE               sigbuf[1024];
    UBYTE               mediarxbuf[64];
    struct bt_a2dp_source source;
    struct bt_sbc_encoder encoder;
    struct bt_a2dp_packetizer packetizer;
    BOOL                encoderready;
    ULONG               framesperpacket;
    ULONG               samplesperframe;
    ULONG               framelen;
    UBYTE               packet[8][BTA_MAX_FRAMES * 512 + 16];
    ULONG               packetlen[8];
    ULONG               queuehead;
    ULONG               queuecount;
    UBYTE               sbcframes[BTA_MAX_FRAMES * 512];
    WORD                fifo[BTA_FIFO_FRAMES * 2];
    ULONG               fifofill;      /* in stereo frames */
    WORD                framepcm[16 * 8 * 2];
    ULONG               packetssent;
    ULONG               packetsdropped;
};

BOOL bLinkPlay(struct BluetoothAudioBase *BluetoothAudioBase, struct AHIAudioCtrlDrv *AudioCtrl);
void bLinkRelease(struct BluetoothAudioBase *BluetoothAudioBase, struct AHIAudioCtrlDrv *AudioCtrl);

BOOL bLinkStart(struct BtaLink *lk);
void bRefreshMode(struct BtaLink *unit);
void bLinkStop(struct BtaLink *lk);
struct BluetoothAudioBase *bAudioCreate(struct BTAudioClass *cls);
void bAudioDelete(struct BluetoothAudioBase *base);

/* The mode carries an ID, never a borrowed binding pointer. */
struct BTAudioModeData
{
    ULONG bmd_Size;
    ULONG bmd_ID;
    ULONG bmd_Frequency;
};

#endif /* BTAUDIO_H */
