/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Link process of the AHI interface of the Bluetooth audio class: keeps the audio sink
    connected, runs AVDTP signalling, SBC encodes the output of the playing
    AudioCtrl and sends it as RTP packets on the AVDTP transport channel,
    paced by timer.device.
*/

#include <devices/ahi.h>
#include <devices/timer.h>
#include <dos/dostags.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <libraries/ahi_sub.h>
#include <libraries/bluetooth.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bluetooth.h>
#include <proto/timer.h>

#include <string.h>

#include "btaudio.h"
#include "clock.h"
#include "stream_state.h"

#define dd(ctrl) ((struct BluetoothAudioData *) (ctrl)->ahiac_DriverData)

/* Our L2CAP receive MTU on the AVDTP channels; the peer's is not known,
   so media packets are kept within the default size. */
#define BTA_MEDIA_MTU       672
#define BTA_MEDIA_SETTLE_TICKS 10   /* ~145 ms at 640 samples per tick */
#define BTA_IDLE_SECS       300     /* close the link after this long without playback */
#define BTA_SETUP_PERIOD_US 10000
#define BTA_RETRY_US 2000000

#define LINK_BASES(lk) \
    struct BluetoothAudioBase *AHIsubBase = (lk)->ahisubbase; \
    struct BluetoothAudioBase *BluetoothAudioBase = (struct BluetoothAudioBase *) AHIsubBase; \
    struct Library *BluetoothBase = (lk)->btbase; \
    (void) AHIsubBase; (void) BluetoothAudioBase; (void) BluetoothBase

#include <aros/asmcall.h>

AROS_UFP0(void, bAudioTask);

/******************************************************************************
** A2DP source callbacks ******************************************************
******************************************************************************/

static bt_status_t bSendSignal(void *context, const uint8_t *pdu, size_t len)
{
    struct BtaLink *lk = context;
    LINK_BASES(lk);

    return btDoChannel(lk->sigwrite, (APTR) pdu, len) ? BT_ERR_IO : BT_OK;
}

static void bOpenMedia(void *context)
{
    struct BtaLink *lk = context;

    lk->wantmedia = TRUE;
}

/* Called with the mixer lock held, once the negotiated media is usable. */
static void bPrepareEncoder(struct BtaLink *lk)
{
    LINK_BASES(lk);
    struct bt_sbc_config *c = &lk->source.config;
    size_t mtu = BTA_MEDIA_MTU;
    static const ULONG rates[] = {16000, 32000, 44100, 48000};
    ULONG rate = rates[c->frequency & 3];
    /* An open AHI allocation cannot change its mixer rate. Never send
       its samples at a different rate after reconnecting. */
    if(lk->owner && rate != lk->mixfreq) {
        btAddErrorMsg(RETURN_ERROR, "btaudio.class", "Sink changed rate from %lu to %lu Hz.", lk->mixfreq, rate);
        lk->broken = TRUE;
        return;
    }
    lk->mixfreq = lk->freq = rate;

    if(bt_sbc_encoder_init(&lk->encoder, c)) {
        lk->framelen = bt_sbc_frame_length(c);
        lk->samplesperframe = (ULONG) c->blocks * c->subbands;
        lk->framesperpacket = bt_a2dp_frames_per_packet(mtu, lk->framelen);
        if(lk->framesperpacket > BTA_MAX_FRAMES) {
            lk->framesperpacket = BTA_MAX_FRAMES;
        }
        lk->encoderready = (lk->framesperpacket > 0);
        btAddErrorMsg(RETURN_OK, "btaudio.class", "Audio format ready at %lu Hz.", rate);
    }
}

static void bStateChanged(void *context, enum bt_a2dp_source_state state)
{
    struct BtaLink *lk = context;
    LINK_BASES(lk);


    ObtainSemaphore(&lk->lock);
    lk->suspendpending = FALSE;
    if(state == BT_A2DP_DISCOVERING || state == BT_A2DP_OPENING ||
       state == BT_A2DP_MEDIA_PENDING || state == BT_A2DP_STARTING) lk->setupseq++;
    if(state == BT_A2DP_STREAMING) {
        bPrepareEncoder(lk);
    } else if(state == BT_A2DP_FAILED) {
        btAddErrorMsg(RETURN_ERROR, "btaudio.class", "A2DP setup failed: signal %lu, error %lu.",
            (ULONG)lk->source.fail_signal, (ULONG)lk->source.fail_error);
        lk->encoderready = FALSE;
        lk->broken = TRUE;
    } else {
        lk->encoderready = FALSE;
    }
    if(state == BT_A2DP_MEDIA_PENDING && !lk->source.initiator) lk->wantmedia = TRUE;
    ReleaseSemaphore(&lk->lock);
}

static const struct bt_a2dp_source_ops bOps = {
    bSendSignal,
    bOpenMedia,
    bStateChanged
};

/******************************************************************************
** PCM ************************************************************************
******************************************************************************/

/* Append one mixer buffer to the FIFO as interleaved stereo 16 bit. */
static void bTakeMix(struct BtaLink *lk, struct AHIAudioCtrlDrv *AudioCtrl)
{
    ULONG n = dd(AudioCtrl)->mixleft;
    ULONG room = BTA_FIFO_FRAMES - lk->fifofill;
    WORD *out = &lk->fifo[lk->fifofill * 2];
    ULONG i;
    ULONG offset = dd(AudioCtrl)->mixoffset;

    if(n > room) {
        n = room;
    }
    switch(AudioCtrl->ahiac_BuffType) {
    case AHIST_S16S: {
        const WORD *in = (const WORD *)dd(AudioCtrl)->mixbuffer + offset * 2;
        memcpy(out, in, n * 2 * sizeof(WORD));
        break;
    }
    case AHIST_M16S: {
        const WORD *in = (const WORD *)dd(AudioCtrl)->mixbuffer + offset;
        for(i = 0; i < n; i++) {
            out[2 * i] = out[2 * i + 1] = in[i];
        }
        break;
    }
    case AHIST_S32S: {
        const LONG *in = (const LONG *)dd(AudioCtrl)->mixbuffer + offset * 2;
        for(i = 0; i < n * 2; i++) {
            out[i] = (WORD) (in[i] >> 16);
        }
        break;
    }
    case AHIST_M32S: {
        const LONG *in = (const LONG *)dd(AudioCtrl)->mixbuffer + offset;
        for(i = 0; i < n; i++) {
            out[2 * i] = out[2 * i + 1] = (WORD) (in[i] >> 16);
        }
        break;
    }
    default:
        memset(out, 0, n * 2 * sizeof(WORD));
        break;
    }
    if(dd(AudioCtrl)->volume < 0x10000) {
        for(i = 0; i < n * 2; i++) {
            out[i] = (WORD) (((LONG) out[i] * (LONG) dd(AudioCtrl)->volume) >> 16);
        }
    }
    lk->fifofill += n;
    dd(AudioCtrl)->mixoffset += n;
    dd(AudioCtrl)->mixleft -= n;
}

/* Run the AHI mixer until the FIFO holds at least frames stereo frames. */
static void bFill(struct BtaLink *lk, struct AHIAudioCtrlDrv *AudioCtrl, ULONG frames)
{
    LINK_BASES(lk);

    while(lk->fifofill < frames && lk->fifofill < BTA_FIFO_FRAMES) {
        if(!dd(AudioCtrl)->mixleft) {
            ULONG samplebytes;
            CallHookPkt(AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL);
            samplebytes = AudioCtrl->ahiac_BuffType == AHIST_S32S ? 8 :
                (AudioCtrl->ahiac_BuffType == AHIST_M16S ? 2 : 4);
            if(!AudioCtrl->ahiac_BuffSamples ||
                AudioCtrl->ahiac_BuffSamples > AudioCtrl->ahiac_BuffSize / samplebytes)
                break;
            CallHookPkt(AudioCtrl->ahiac_MixerFunc, AudioCtrl, dd(AudioCtrl)->mixbuffer);
            dd(AudioCtrl)->mixoffset = 0;
            dd(AudioCtrl)->mixleft = AudioCtrl->ahiac_BuffSamples;
        }
        bTakeMix(lk, AudioCtrl);
    }
}

static void bConsume(struct BtaLink *lk, ULONG frames)
{
    if(frames > lk->fifofill) {
        frames = lk->fifofill;
    }
    memmove(lk->fifo, &lk->fifo[frames * 2], (lk->fifofill - frames) * 2 * sizeof(WORD));
    lk->fifofill -= frames;
}

/*
 * Produce one packet's worth of audio. When streaming it is encoded and
 * sent; otherwise it is discarded at the same pace so applications keep
 * running while the sink is (re)connecting.
 */
/* The front packet remains owned by the channel until its reply. Other
   packets accumulate behind it; neither PCM nor an in-flight buffer is reused. */
static void bSendQueued(struct BtaLink *lk)
{
    LINK_BASES(lk);
    if(!lk->mediawritepending && lk->queuecount && lk->mediawrite) {
        btSendChannel(lk->mediawrite, lk->packet[lk->queuehead], lk->packetlen[lk->queuehead]);
        lk->mediawritepending = TRUE;
        lk->packetssent++;
    }
}

static BOOL bTick(struct BtaLink *lk, struct AHIAudioCtrlDrv *AudioCtrl, ULONG packet_frames)
{
    LINK_BASES(lk);
    ULONG f, i, tail;
    if(lk->encoderready) packet_frames = lk->framesperpacket * lk->samplesperframe;

    /* Back-pressure before running the mixer: a slow write must not consume
       audio which we cannot retain. */
    if(lk->encoderready && lk->queuecount == 8) return FALSE;
    bFill(lk, AudioCtrl, packet_frames);
    if(lk->fifofill < packet_frames) return FALSE;
    if(!lk->encoderready || lk->source.state != BT_A2DP_STREAMING) {
        bConsume(lk, packet_frames);
        return TRUE;
    }
    for(f = 0; f < lk->framesperpacket; f++) {
        const WORD *pcm = &lk->fifo[f * lk->samplesperframe * 2];
        const int16_t *in = (const int16_t *) pcm;

        if(lk->encoder.channels == 1) {
            for(i = 0; i < lk->samplesperframe; i++) {
                lk->framepcm[i] = (WORD) (((LONG) pcm[2 * i] + pcm[2 * i + 1]) / 2);
            }
            in = (const int16_t *) lk->framepcm;
        }
        if(bt_sbc_encode(&lk->encoder, in, &lk->sbcframes[f * lk->framelen], lk->framelen) != lk->framelen) {
            bConsume(lk, packet_frames);
            lk->broken = TRUE;
            lk->encoderready = FALSE;
            return TRUE;
        }
    }
    bConsume(lk, packet_frames);
    {
        tail = (lk->queuehead + lk->queuecount) % 8;
        size_t len = bt_a2dp_build_media_packet(&lk->packetizer, lk->sbcframes, lk->framelen,
                                                lk->framesperpacket, lk->samplesperframe,
                                                lk->packet[tail], sizeof(lk->packet[tail]));
        if(len) {
            lk->packetlen[tail] = len;
            lk->queuecount++;
            bSendQueued(lk);
        }
    }
    return TRUE;
}

/******************************************************************************
** Connection *****************************************************************
******************************************************************************/

static APTR bChannel(struct BtaLink *lk, APTR ep, UWORD request)
{
    LINK_BASES(lk);
    APTR ch = btAllocChannel(lk->device, lk->port, ep);

    if(ch) {
        btSetAttrs(BGA_CHANNEL, ch, BCHA_AutoConnect, TRUE, TAG_END);
        btChannelSetup(ch, request, 0, 0);
    }
    return ch;
}

static void bPostSigread(struct BtaLink *lk)
{
    LINK_BASES(lk);

    if(lk->sigread && !lk->sigreadpending) {
        btSendChannel(lk->sigread, lk->sigbuf, sizeof(lk->sigbuf));
        lk->sigreadpending = TRUE;
    }
}

static void bHandlePort(struct BtaLink *lk)
{
    LINK_BASES(lk);
    struct Message *msg;

    while((msg = GetMsg(lk->port))) {
        APTR ch = (APTR) msg;

        if(ch == lk->sigread) {
            LONG err = btGetChannelError(ch);
            ULONG actual = btGetChannelActual(ch);

            lk->sigreadpending = FALSE;
            if(!err && actual) {
                bt_a2dp_source_on_signal(&lk->source, lk->sigbuf, actual);
                bPostSigread(lk);
            } else if(err) {
                /* the sink went away: the link reconnects on demand */
                btAddErrorMsg(RETURN_WARN, "btaudio.class", "Signalling read failed (%ld); reconnecting.", err);
                lk->broken = TRUE;
            } else {
                bPostSigread(lk);
            }
        } else if(ch == lk->mediawrite) {
            lk->mediawritepending = FALSE;
            if(btGetChannelError(ch)) lk->broken = TRUE;
            if(lk->queuecount) {
                lk->queuehead = (lk->queuehead + 1) % 8;
                lk->queuecount--;
            }
            if(!lk->broken) bSendQueued(lk);
        } else if(ch == lk->mediaread) {
            /* nothing is expected on the transport channel; keep a read
               posted so the channel stays open */
            lk->mediareadpending = FALSE;
            if(btGetChannelError(ch) == 0) {
                btSendChannel(lk->mediaread, lk->mediarxbuf, sizeof(lk->mediarxbuf));
                lk->mediareadpending = TRUE;
            } else {
                lk->broken = TRUE;
            }
        }
    }
}

/* OPEN was accepted: bring the transport channel up and tell the source
   once it is open. */
static void bServiceMedia(struct BtaLink *lk)
{
    LINK_BASES(lk);
    IPTR isopen = FALSE;

    if(!lk->wantmedia) {
        return;
    }
    btGetAttrs(BGA_ENDPOINT, lk->mediaep, BEA_IsOpen, (IPTR) &isopen, TAG_END);
    if(!lk->source.initiator && !isopen) return;
    if(!lk->mediaread) {
        lk->mediaread = bChannel(lk, lk->mediaep, BTPR_READ);
        lk->mediawrite = bChannel(lk, lk->mediaep, BTPR_WRITE);
        if(!lk->mediaread || !lk->mediawrite) {
            btAddErrorMsg(RETURN_ERROR, "btaudio.class", "Cannot allocate transport channels.");
            lk->wantmedia = FALSE;
            return;
        }
        if(!lk->source.initiator) {
            btSetAttrs(BGA_CHANNEL, lk->mediaread, BCHA_AutoConnect, FALSE, TAG_END);
            btSetAttrs(BGA_CHANNEL, lk->mediawrite, BCHA_AutoConnect, FALSE, TAG_END);
        }
        /* For a local OPEN the read connects; for a remote OPEN it uses
           the channel already accepted by the secondary endpoint. */
        btSendChannel(lk->mediaread, lk->mediarxbuf, sizeof(lk->mediarxbuf));
        lk->mediareadpending = TRUE;
    }
    btGetAttrs(BGA_ENDPOINT, lk->mediaep, BEA_IsOpen, (IPTR) &isopen, TAG_END);
    if(!isopen) {
        lk->openticks = 0;
        return;
    }
    /* The endpoint reports open once the connection is accepted, which can
       be before the sink has finished configuring the channel; a START
       sent then is rejected with BAD_STATE. Give it a moment. */
    if(lk->source.initiator && ++lk->openticks < BTA_MEDIA_SETTLE_TICKS) {
        return;
    }
    lk->wantmedia = FALSE;
    bt_a2dp_source_media_open(&lk->source);
    /* A sink may leave START to us. Publish its negotiated format without
       requiring playback, and keep servicing commands while it is idle. */
    if(lk->source.state == BT_A2DP_MEDIA_PENDING) {
        ObtainSemaphore(&lk->lock);
        bPrepareEncoder(lk);
        ReleaseSemaphore(&lk->lock);
    }
}

static void bClose(struct BtaLink *lk)
{
    LINK_BASES(lk);
    APTR *chans[4];
    ULONG i;

    chans[0] = &lk->sigread;
    chans[1] = &lk->sigwrite;
    chans[2] = &lk->mediaread;
    chans[3] = &lk->mediawrite;
    for(i = 0; i < 4; i++) {
        if(*chans[i]) {
            btAbortChannel(*chans[i]);
            btWaitChannel(*chans[i]);
            btFreeChannel(*chans[i]);
            *chans[i] = NULL;
        }
    }
    lk->sigreadpending = lk->mediareadpending = lk->mediawritepending = FALSE;
    lk->queuehead = lk->queuecount = 0;
}

/* Connect to the sink and start A2DP signalling. */
static BOOL bBeginSession(struct BtaLink *lk, const UBYTE *address, ULONG freq)
{
    LINK_BASES(lk);

    CopyMem((APTR) address, lk->address, sizeof(lk->address));
    lk->freq = freq;
    lk->broken = FALSE;
    lk->fifofill = 0;
    lk->packetssent = lk->packetsdropped = 0;

    bt_a2dp_source_init(&lk->source, &bOps, lk);
    bt_a2dp_packetizer_init(&lk->packetizer, 0x41524f53);
    switch(freq) {
    case 16000: lk->source.prefer_frequency = BT_SBC_FREQ_16000; break;
    case 32000: lk->source.prefer_frequency = BT_SBC_FREQ_32000; break;
    case 48000: lk->source.prefer_frequency = BT_SBC_FREQ_48000; break;
    default:    lk->source.prefer_frequency = BT_SBC_FREQ_44100; break;
    }

    lk->sigread = bChannel(lk, lk->sigep, BTPR_READ);
    lk->sigwrite = bChannel(lk, lk->sigep, BTPR_WRITE);
    if(!lk->sigread || !lk->sigwrite) {
        bClose(lk);
        return FALSE;
    }
    bPostSigread(lk);
    bt_a2dp_source_connect(&lk->source);
    lk->session = TRUE;
    return TRUE;
}

static void bEndSession(struct BtaLink *lk)
{
    LINK_BASES(lk);

    if(!lk->broken && !lk->suspendpending && lk->source.state == BT_A2DP_STREAMING) {
        bt_a2dp_source_suspend(&lk->source);
    }
    btAddErrorMsg(RETURN_OK, "btaudio.class", "Audio session closed (%lu packets sent).", lk->packetssent);
    bClose(lk);
    lk->session = FALSE;
    lk->encoderready = FALSE;
    lk->wantmedia = FALSE;
    lk->openticks = 0;
    lk->fifofill = 0;

}

/******************************************************************************
** Called by Start() and Stop() ***********************************************
******************************************************************************/

/* AHI submits PCM to an already bound and running transport. */
BOOL bLinkPlay(struct BluetoothAudioBase *AHIsubBase, struct AHIAudioCtrlDrv *AudioCtrl)
{
    struct BtaLink *lk = dd(AudioCtrl)->unit;
    BOOL ok;
    ObtainSemaphore(&lk->lock);
    ok = lk->task != NULL;
    if(ok) {
        lk->playseq++;
        dd(AudioCtrl)->mixleft = dd(AudioCtrl)->mixoffset = 0;
        lk->fifofill = 0;
        lk->want_freq = AudioCtrl->ahiac_MixFreq;
        lk->ctrl = AudioCtrl;
        Signal((struct Task *)lk->task, 1UL << lk->wakesig);
    }
    ReleaseSemaphore(&lk->lock);
    return ok;
}

/* The binding starts the transport task before publishing its mode. */
BOOL bLinkStart(struct BtaLink *lk)
{
    struct Library *BluetoothBase = OpenLibrary("bluetooth.library", 1);
    struct Task *task;
    BYTE sig = AllocSignal(-1);
    BOOL ok = FALSE;
    if(sig == -1 || !BluetoothBase) goto out;
    ObtainSemaphore(&lk->lock);
    lk->creator = FindTask(NULL);
    lk->creatorsig = sig;
    lk->ready = FALSE;
    lk->generation++;
    lk->preparing = TRUE;
    lk->quit = FALSE;
    lk->transport_stopped = FALSE;
    SetSignal(0, 1UL << sig);
    task = lk->task;
    if(task) Signal(task, 1UL << lk->wakesig);
    else {
        lk->wakesig = -1;
        task = btSpawnSubTask("Bluetooth audio", (APTR)bAudioTask, lk);
    }
    ReleaseSemaphore(&lk->lock);
    if(task) {
        btBorrowLocksWait(task, 1UL << sig);
        ObtainSemaphore(&lk->lock);
        ok = lk->ready;
        lk->creator = NULL;
        ReleaseSemaphore(&lk->lock);
    }
out:
    if(sig != -1) FreeSignal(sig);
    if(BluetoothBase) CloseLibrary(BluetoothBase);
    return ok;
}

void bLinkStop(struct BtaLink *lk)
{
    struct Library *BluetoothBase = OpenLibrary("bluetooth.library", 1);
    struct Task *task;
    /* The binder owns the device lock. Do not first wait for lk->lock:
       the worker may hold it while waiting for that same device lock. */
    Forbid();
    lk->attached = FALSE;
    lk->quit = TRUE;
    task = lk->task;
    if(task) {
        lk->stopper = FindTask(NULL);
        lk->stopsig = SIGB_SINGLE;
        SetSignal(0, SIGF_SINGLE);
        Signal(task, SIGBREAKF_CTRL_C);
    }
    Permit();
    if(task) {
        if(BluetoothBase) btBorrowLocksWait(task, SIGF_SINGLE);
        else Wait(SIGF_SINGLE);
    }
    if(BluetoothBase) CloseLibrary(BluetoothBase);
}

/* AudioCtrl stops playing. On return the link process no longer uses it,
   so its mix buffer may be freed. The sink stays connected. */
void bLinkRelease(struct BluetoothAudioBase *BluetoothAudioBase, struct AHIAudioCtrlDrv *AudioCtrl)
{
    struct BluetoothAudioBase *AHIsubBase = (struct BluetoothAudioBase *) BluetoothAudioBase;
    struct BtaLink *lk = dd(AudioCtrl)->unit;

    if(lk == NULL) {
        return;
    }
    ObtainSemaphore(&lk->lock);
    if(lk->ctrl == AudioCtrl) {
        lk->ctrl = NULL;
        if(lk->task != NULL) {
            Signal((struct Task *) lk->task, 1L << lk->wakesig);
        }
    }
    ReleaseSemaphore(&lk->lock);
}

/******************************************************************************
** The link process ***********************************************************
******************************************************************************/

static UQUAD bNow(struct Device *TimerBase)
{
    struct timeval now;
    GetUpTime(&now);
    return (UQUAD)now.tv_secs * 1000000 + now.tv_micro;
}

AROS_UFH0(void, bAudioTask)
{
    AROS_USERFUNC_INIT
    struct BtaLink *lk = FindTask(NULL)->tc_UserData;
    struct BluetoothAudioBase *AHIsubBase = lk->ahisubbase;
    struct Library *BluetoothBase = OpenLibrary("bluetooth.library", 1);
    struct Device *TimerBase;
    BOOL timerpending = FALSE, running;
    ULONG generation = 0, playseq = 0, setupseq = 0, signals = 0;
    struct AHIAudioCtrlDrv *lastctrl = NULL;
    struct bAudioClock clock = {0, 0};
    UQUAD setup_until = 0, retry_at = 0, idle_until = 0, service_at = 0, mode_at = 0;

    Forbid();
    AHIsubBase->library.lib_OpenCnt++;
    Permit();
    SetTaskPri(FindTask(NULL), 50);
    ObtainSemaphore(&lk->lock);
    lk->task = FindTask(NULL);
    lk->btbase = BluetoothBase;
    lk->wakesig = AllocSignal(-1);
    lk->port = CreateMsgPort();
    lk->timerport = CreateMsgPort();
    lk->timereq = lk->timerport ? (APTR)CreateIORequest(lk->timerport, sizeof(*lk->timereq)) : NULL;
    if(lk->timereq && !OpenDevice(TIMERNAME, UNIT_MICROHZ, (APTR)lk->timereq, 0)) lk->timeropen = TRUE;
    TimerBase = lk->timeropen ? lk->timereq->tr_node.io_Device : NULL;
    running = lk->wakesig != -1 && lk->port && lk->timeropen && BluetoothBase;
    ReleaseSemaphore(&lk->lock);

    while(running) {
        UQUAD now = bNow(TimerBase), next = 0;
        ULONG packets = 0, frames;
        BOOL active, blocked = FALSE, checkmode;
        ObtainSemaphore(&lk->lock);
        if(generation != lk->generation) {
            generation = lk->generation;
            /* A retained worker must not inherit the previous binding's
               timeout or retry delay. */
            setup_until = now + 10000000;
            retry_at = service_at = 0;
            lk->broken = FALSE;
        }
        active = lk->ctrl != NULL;
        if(lk->ctrl != lastctrl || playseq != lk->playseq) {
            lk->fifofill = 0;
            lastctrl = lk->ctrl;
            playseq = lk->playseq;
            clock.next = now;
            clock.remainder = 0;
            idle_until = active ? 0 : now + (UQUAD)BTA_IDLE_SECS * 1000000;
        }
        ReleaseSemaphore(&lk->lock);

        /* Transport operations can wait on the binder's device lock. Never
           hold the mixer lock across them: the binder needs it to finish
           publication and return its borrowed locks. Channel replies remain
           serviced during that handoff rather than waiting only for a wake. */
        if(lk->quit) {
            if(!lk->transport_stopped) {
                if(lk->session) bEndSession(lk);
                else bClose(lk);
                ObtainSemaphore(&lk->lock);
                lk->transport_stopped = TRUE;
                if(lk->stopper) {
                    Signal(lk->stopper, 1UL << lk->stopsig);
                    lk->stopper = NULL;
                }
                ReleaseSemaphore(&lk->lock);
            }
            ObtainSemaphore(&lk->lock);
            if(!lk->owner && !lk->binding) goto cleanup;
            ReleaseSemaphore(&lk->lock);
        } else {
            if(signals & (1UL << lk->port->mp_SigBit)) bHandlePort(lk);
            if(setupseq != lk->setupseq) {
                setupseq = lk->setupseq;
                setup_until = now + 10000000;
            }
            if(lk->session && !lk->encoderready &&
               !bStreamWaiting(lk->source.state, lk->mediawrite && !lk->wantmedia) && now >= setup_until)
                lk->broken = TRUE;
            if(lk->broken && lk->session) {
                bEndSession(lk);
                retry_at = now + BTA_RETRY_US;
            }
            if(!lk->session && now >= retry_at && (lk->preparing || active)) {
                setup_until = now + 10000000;
                if(!bBeginSession(lk, lk->want_address, lk->mixfreq ? lk->mixfreq : 44100))
                    retry_at = now + BTA_RETRY_US;
                else {
                    service_at = now;
                    if(!idle_until) idle_until = now + (UQUAD)BTA_IDLE_SECS * 1000000;
                }
            }
            if(lk->session && now >= service_at) {
                bServiceMedia(lk);
                service_at = now + BTA_SETUP_PERIOD_US;
            }
            if(active && lk->session &&
               bStreamWaiting(lk->source.state, lk->mediawrite && !lk->wantmedia)) {
                setup_until = now + 10000000;
                bt_a2dp_source_start(&lk->source);
            }
            if(!active && !lk->preparing && !lk->queuecount && !lk->suspendpending &&
               lk->session && lk->source.state == BT_A2DP_STREAMING) {
                lk->suspendpending = TRUE;
                bt_a2dp_source_suspend(&lk->source);
            }
            if(!active && !lk->preparing && lk->session && idle_until && now >= idle_until)
                bEndSession(lk);
        }

        ObtainSemaphore(&lk->lock);
        if(lk->preparing && (lk->encoderready || lk->broken || now >= setup_until)) {
            lk->ready = lk->encoderready && !lk->broken;
            lk->preparing = FALSE;
            if(lk->creator) Signal(lk->creator, 1UL << lk->creatorsig);
            if(!lk->ready)
                btAddErrorMsg(RETURN_ERROR, "btaudio.class", "Cannot negotiate audio with %s.", lk->want_address);
        }
        /* Keep AHI's clock running during setup, failure and reconnect as
           well as unbind. Catch up from the last deadline after slow sends;
           retain at most one queue of debt after a prolonged stall. */
        now = bNow(TimerBase);
        frames = lk->encoderready ? lk->framesperpacket * lk->samplesperframe : 640;
        if(lk->ctrl) bClockLimitDebt(&clock, now, frames, lk->mixfreq ? lk->mixfreq : 44100, 8);
        while(lk->ctrl && now >= clock.next && packets < 8) {
            if(!bTick(lk, lk->ctrl, frames)) {
                blocked = TRUE;
                break;
            }
            bClockAdvance(&clock, frames, lk->mixfreq ? lk->mixfreq : 44100);
            packets++;
        }
        if(lk->ctrl) next = blocked ? now + BTA_SETUP_PERIOD_US : clock.next;
        checkmode = lk->attached && !lk->binding &&
            (now >= mode_at || lk->modefreq != lk->mixfreq);
        ReleaseSemaphore(&lk->lock);
        if(checkmode) {
            bRefreshMode(lk);
            mode_at = now + 30000000;
        }

#define BTA_DEADLINE(value) do { UQUAD deadline = (value); if(deadline && (!next || deadline < next)) next = deadline; } while(0)
        if(!lk->quit) {
            if(lk->preparing || (lk->session && !lk->encoderready && !bStreamWaiting(lk->source.state, lk->mediawrite && !lk->wantmedia)))
                BTA_DEADLINE(service_at ? service_at : now + BTA_SETUP_PERIOD_US);
            if(lk->wantmedia) BTA_DEADLINE(service_at);
            if(!lk->session && (active || lk->preparing)) BTA_DEADLINE(retry_at);
            if(!active && lk->session) BTA_DEADLINE(idle_until);
            if(lk->attached && !lk->binding) BTA_DEADLINE(mode_at);
        }
#undef BTA_DEADLINE
        if(next) {
            UQUAD current = bNow(TimerBase);
            UQUAD delay = next > current ? next - current : 1;
            lk->timereq->tr_node.io_Command = TR_ADDREQUEST;
            lk->timereq->tr_time.tv_secs = delay / 1000000;
            lk->timereq->tr_time.tv_micro = delay % 1000000;
            SendIO((APTR)lk->timereq);
            timerpending = TRUE;
        }
        signals = Wait((1UL << lk->wakesig) | (1UL << lk->port->mp_SigBit) |
            (1UL << lk->timerport->mp_SigBit) | SIGBREAKF_CTRL_C);
        if(timerpending) {
            if(!CheckIO((APTR)lk->timereq)) AbortIO((APTR)lk->timereq);
            WaitIO((APTR)lk->timereq);
            timerpending = FALSE;
            /* AbortIO also signals this port. Its reply has been removed by
               WaitIO; do not let that stale signal abort the next timer. */
            SetSignal(0, 1UL << lk->timerport->mp_SigBit);
        }
    }
    ObtainSemaphore(&lk->lock);
cleanup:
    if(timerpending) {
        AbortIO((APTR)lk->timereq);
        WaitIO((APTR)lk->timereq);
    }
    if(lk->session) bEndSession(lk);
    else if(BluetoothBase) bClose(lk);
    if(lk->timeropen) CloseDevice((APTR)lk->timereq);
    if(lk->timereq) DeleteIORequest((APTR)lk->timereq);
    if(lk->timerport) DeleteMsgPort(lk->timerport);
    if(lk->port) DeleteMsgPort(lk->port);
    if(lk->wakesig != -1) FreeSignal(lk->wakesig);
    lk->timeropen = FALSE;
    lk->timereq = NULL;
    lk->timerport = lk->port = NULL;
    lk->btbase = NULL;
    if(BluetoothBase) CloseLibrary(BluetoothBase);
    Forbid();
    lk->task = NULL;
    lk->transport_stopped = TRUE;
    if(lk->creator) Signal(lk->creator, 1UL << lk->creatorsig);
    if(lk->stopper) Signal(lk->stopper, 1UL << lk->stopsig);
    ReleaseSemaphore(&lk->lock);
    AHIsubBase->library.lib_OpenCnt--;
    AROS_USERFUNC_EXIT
}
