/*
 * A2DP source (audio sender) over AVDTP. Transport agnostic: the owner
 * carries signalling PDUs over the first L2CAP channel to PSM 0x0019 and
 * media packets over the second one, feeding every received signalling
 * PDU to bt_a2dp_source_on_signal().
 *
 * As initiator the source discovers the sink's stream endpoints, picks an
 * audio sink endpoint with SBC, configures it, opens it (the owner then
 * connects the transport channel and calls bt_a2dp_source_media_open())
 * and starts it. Commands the sink sends itself (discover, capabilities,
 * configuration, open, start, suspend, close, delay report) are answered
 * for the one local SBC source endpoint.
 */

#ifndef BTCORE_A2DP_SOURCE_H
#define BTCORE_A2DP_SOURCE_H

#include <btcore/types.h>
#include <btcore/status.h>
#include <btcore/sbc.h>

#define BT_AVDTP_PSM 0x0019

/* AVDTP signal identifiers */
#define BT_AVDTP_DISCOVER             0x01
#define BT_AVDTP_GET_CAPABILITIES     0x02
#define BT_AVDTP_SET_CONFIGURATION    0x03
#define BT_AVDTP_GET_CONFIGURATION    0x04
#define BT_AVDTP_RECONFIGURE          0x05
#define BT_AVDTP_OPEN                 0x06
#define BT_AVDTP_START                0x07
#define BT_AVDTP_CLOSE                0x08
#define BT_AVDTP_SUSPEND              0x09
#define BT_AVDTP_ABORT                0x0a
#define BT_AVDTP_SECURITY_CONTROL     0x0b
#define BT_AVDTP_GET_ALL_CAPABILITIES 0x0c
#define BT_AVDTP_DELAY_REPORT         0x0d

/* message types (low two bits of the first header octet) */
#define BT_AVDTP_MSG_COMMAND          0
#define BT_AVDTP_MSG_GENERAL_REJECT   1
#define BT_AVDTP_MSG_ACCEPT           2
#define BT_AVDTP_MSG_REJECT           3

/* service categories */
#define BT_AVDTP_CAT_MEDIA_TRANSPORT  0x01
#define BT_AVDTP_CAT_MEDIA_CODEC      0x07
#define BT_AVDTP_CAT_DELAY_REPORTING  0x08

#define BT_AVDTP_LOCAL_SEID           1

enum bt_a2dp_source_state
{
    BT_A2DP_IDLE = 0,
    BT_A2DP_DISCOVERING,
    BT_A2DP_GETTING_CAPABILITIES,
    BT_A2DP_CONFIGURING,
    BT_A2DP_OPENING,        /* OPEN sent */
    BT_A2DP_MEDIA_PENDING,  /* OPEN accepted; owner is connecting the media channel */
    BT_A2DP_STARTING,
    BT_A2DP_STREAMING,
    BT_A2DP_SUSPENDED,
    BT_A2DP_FAILED
};

struct bt_a2dp_source;

struct bt_a2dp_source_ops
{
    /* send one signalling PDU on the signalling channel */
    bt_status_t (*send_signal)(void *context, const uint8_t *pdu, size_t len);
    /* OPEN was accepted: connect the transport channel (a second L2CAP
       channel to PSM 0x0019), then call bt_a2dp_source_media_open() */
    void (*open_media)(void *context);
    /* the state changed; on BT_A2DP_STREAMING, config holds the SBC
       configuration to encode with */
    void (*state_changed)(void *context, enum bt_a2dp_source_state state);
};

struct bt_a2dp_source
{
    const struct bt_a2dp_source_ops *ops;
    void *context;
    enum bt_a2dp_source_state state;
    uint8_t label;              /* transaction label of the outstanding command */
    uint8_t remote_seid;
    uint8_t candidates[16];     /* audio sink SEIDs from DISCOVER, not in use */
    uint8_t candidate_count;
    uint8_t candidate_next;
    bool remote_delay_reporting;
    struct bt_sbc_config config; /* negotiated */
    uint8_t fail_signal;        /* signal that failed (FAILED state) */
    uint8_t fail_error;         /* AVDTP error code, 0 if none */
    bool initiator;             /* we configured the stream (else the sink did) */
    uint8_t prefer_frequency;   /* BT_SBC_FREQ_xxx to pick when the sink offers it, 0xff = none */
};

/* SBC capability octets (A2DP codec specific information elements) */
#define BT_A2DP_SBC_FREQ_16000   0x80
#define BT_A2DP_SBC_FREQ_32000   0x40
#define BT_A2DP_SBC_FREQ_44100   0x20
#define BT_A2DP_SBC_FREQ_48000   0x10
#define BT_A2DP_SBC_MODE_MONO    0x08
#define BT_A2DP_SBC_MODE_DUAL    0x04
#define BT_A2DP_SBC_MODE_STEREO  0x02
#define BT_A2DP_SBC_MODE_JOINT   0x01
#define BT_A2DP_SBC_BLOCKS_4     0x80
#define BT_A2DP_SBC_BLOCKS_8     0x40
#define BT_A2DP_SBC_BLOCKS_12    0x20
#define BT_A2DP_SBC_BLOCKS_16    0x10
#define BT_A2DP_SBC_SUBBANDS_4   0x08
#define BT_A2DP_SBC_SUBBANDS_8   0x04
#define BT_A2DP_SBC_ALLOC_SNR    0x02
#define BT_A2DP_SBC_ALLOC_LOUD   0x01

/* Highest bitpool we ask for: the usual "high quality" A2DP setting. */
#define BT_A2DP_SBC_MAX_BITPOOL  53

void bt_a2dp_source_init(struct bt_a2dp_source *src, const struct bt_a2dp_source_ops *ops, void *context);

/* begin discovery and configuration as initiator (signalling channel up) */
bt_status_t bt_a2dp_source_connect(struct bt_a2dp_source *src);

/* feed one received signalling PDU */
void bt_a2dp_source_on_signal(struct bt_a2dp_source *src, const uint8_t *pdu, size_t len);

/* the transport channel requested via open_media() is connected */
bt_status_t bt_a2dp_source_media_open(struct bt_a2dp_source *src);

/* suspend / resume a configured stream */
bt_status_t bt_a2dp_source_suspend(struct bt_a2dp_source *src);
bt_status_t bt_a2dp_source_start(struct bt_a2dp_source *src);

/* Choose an SBC configuration from the sink's capability octets (4 bytes).
   Prefers 44.1 kHz, then 48 kHz; joint stereo, then stereo; 16 blocks,
   8 subbands, loudness; bitpool min(sink max, BT_A2DP_SBC_MAX_BITPOOL).
   Returns false if there is no usable combination. */
bool bt_a2dp_sbc_select(const uint8_t caps[4], struct bt_sbc_config *config);

/* As bt_a2dp_sbc_select(), but picks frequency (BT_SBC_FREQ_xxx) if the sink
   supports it. */
bool bt_a2dp_sbc_select_preferring(const uint8_t caps[4], uint8_t frequency, struct bt_sbc_config *config);

/* Encode config as the 4 SBC codec information octets of a configuration. */
void bt_a2dp_sbc_to_octets(const struct bt_sbc_config *config, uint8_t out[4]);

/*
 * RTP media packets for the transport channel: 12-byte RTP header
 * (payload type 96) followed by the one-octet SBC media payload header
 * (frame count) and the SBC frames.
 */
struct bt_a2dp_packetizer
{
    uint16_t sequence;
    uint32_t timestamp; /* in samples */
    uint32_t ssrc;
};

void bt_a2dp_packetizer_init(struct bt_a2dp_packetizer *pk, uint32_t ssrc);

/* Builds one packet from frame_count frames of frame_len bytes each, each
   frame carrying samples_per_frame samples per channel. Returns the packet
   length, or 0 if it does not fit out_len or frame_count is not 1..15. */
size_t bt_a2dp_build_media_packet(struct bt_a2dp_packetizer *pk, const uint8_t *frames,
                                  size_t frame_len, unsigned frame_count,
                                  unsigned samples_per_frame, uint8_t *out, size_t out_len);

/* How many frames of frame_len bytes fit into one packet for the given
   transport channel MTU (at most 15). */
unsigned bt_a2dp_frames_per_packet(size_t mtu, size_t frame_len);

#endif /* BTCORE_A2DP_SOURCE_H */
