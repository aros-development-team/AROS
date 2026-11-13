#include <btcore/a2dp_source.h>

#include <stdint.h>
#include <string.h>

/* AVDTP error codes used in responses */
#define AVDTP_ERR_BAD_ACP_SEID         0x12
#define AVDTP_ERR_SEP_IN_USE           0x13
#define AVDTP_ERR_BAD_STATE            0x31

/* Capabilities of the one local source endpoint: everything the encoder
   handles, bitpool 2..BT_A2DP_SBC_MAX_BITPOOL. */
static const uint8_t local_sbc_caps[4] = {
    BT_A2DP_SBC_FREQ_16000 | BT_A2DP_SBC_FREQ_32000 | BT_A2DP_SBC_FREQ_44100 | BT_A2DP_SBC_FREQ_48000 |
    BT_A2DP_SBC_MODE_MONO | BT_A2DP_SBC_MODE_DUAL | BT_A2DP_SBC_MODE_STEREO | BT_A2DP_SBC_MODE_JOINT,
    BT_A2DP_SBC_BLOCKS_4 | BT_A2DP_SBC_BLOCKS_8 | BT_A2DP_SBC_BLOCKS_12 | BT_A2DP_SBC_BLOCKS_16 |
    BT_A2DP_SBC_SUBBANDS_4 | BT_A2DP_SBC_SUBBANDS_8 | BT_A2DP_SBC_ALLOC_SNR | BT_A2DP_SBC_ALLOC_LOUD,
    2, BT_A2DP_SBC_MAX_BITPOOL
};

static void set_state(struct bt_a2dp_source *src, enum bt_a2dp_source_state state)
{
    if(src->state != state) {
        src->state = state;
        if(src->ops->state_changed) {
            src->ops->state_changed(src->context, state);
        }
    }
}

static void fail(struct bt_a2dp_source *src, uint8_t signal, uint8_t error)
{
    src->fail_signal = signal;
    src->fail_error = error;
    set_state(src, BT_A2DP_FAILED);
}

static bt_status_t send_pdu(struct bt_a2dp_source *src, uint8_t label, uint8_t msg_type,
                            uint8_t signal, const uint8_t *params, size_t params_len)
{
    uint8_t pdu[32];

    if(params_len > sizeof(pdu) - 2) {
        return BT_ERR_BUFFER_OVERFLOW;
    }
    pdu[0] = (uint8_t)(((label & 0x0f) << 4) | msg_type); /* single packet */
    pdu[1] = signal & 0x3f;
    if(params_len) {
        memcpy(&pdu[2], params, params_len);
    }
    return src->ops->send_signal(src->context, pdu, params_len + 2);
}

static bt_status_t send_command(struct bt_a2dp_source *src, uint8_t signal,
                                const uint8_t *params, size_t params_len)
{
    src->label = (uint8_t)((src->label + 1) & 0x0f);
    return send_pdu(src, src->label, BT_AVDTP_MSG_COMMAND, signal, params, params_len);
}

static bt_status_t send_seid_command(struct bt_a2dp_source *src, uint8_t signal)
{
    uint8_t p = (uint8_t)(src->remote_seid << 2);

    return send_command(src, signal, &p, 1);
}

void bt_a2dp_source_init(struct bt_a2dp_source *src, const struct bt_a2dp_source_ops *ops, void *context)
{
    memset(src, 0, sizeof(*src));
    src->ops = ops;
    src->context = context;
    src->state = BT_A2DP_IDLE;
    src->prefer_frequency = 0xff;
}

bt_status_t bt_a2dp_source_connect(struct bt_a2dp_source *src)
{
    if(src->state != BT_A2DP_IDLE && src->state != BT_A2DP_FAILED) {
        return BT_ERR_INVALID_STATE;
    }
    src->candidate_count = 0;
    src->candidate_next = 0;
    src->initiator = true;
    set_state(src, BT_A2DP_DISCOVERING);
    return send_command(src, BT_AVDTP_DISCOVER, NULL, 0);
}

bool bt_a2dp_sbc_select(const uint8_t caps[4], struct bt_sbc_config *config)
{
    static const struct { uint8_t bit, value; } freqs[] = {
        { BT_A2DP_SBC_FREQ_44100, BT_SBC_FREQ_44100 }, { BT_A2DP_SBC_FREQ_48000, BT_SBC_FREQ_48000 },
        { BT_A2DP_SBC_FREQ_32000, BT_SBC_FREQ_32000 }, { BT_A2DP_SBC_FREQ_16000, BT_SBC_FREQ_16000 } };
    static const struct { uint8_t bit, value; } modes[] = {
        { BT_A2DP_SBC_MODE_JOINT, BT_SBC_MODE_JOINT_STEREO }, { BT_A2DP_SBC_MODE_STEREO, BT_SBC_MODE_STEREO },
        { BT_A2DP_SBC_MODE_DUAL, BT_SBC_MODE_DUAL_CHANNEL }, { BT_A2DP_SBC_MODE_MONO, BT_SBC_MODE_MONO } };
    static const struct { uint8_t bit, value; } blocks[] = {
        { BT_A2DP_SBC_BLOCKS_16, 16 }, { BT_A2DP_SBC_BLOCKS_12, 12 },
        { BT_A2DP_SBC_BLOCKS_8, 8 }, { BT_A2DP_SBC_BLOCKS_4, 4 } };
    struct bt_sbc_config c;
    unsigned i, limit, bitpool;
    bool ok;

    memset(&c, 0, sizeof(c));
    for(ok = false, i = 0; i < 4 && !ok; i++) {
        if(caps[0] & freqs[i].bit) { c.frequency = freqs[i].value; ok = true; }
    }
    if(!ok) return false;
    for(ok = false, i = 0; i < 4 && !ok; i++) {
        if(caps[0] & modes[i].bit) { c.mode = modes[i].value; ok = true; }
    }
    if(!ok) return false;
    for(ok = false, i = 0; i < 4 && !ok; i++) {
        if(caps[1] & blocks[i].bit) { c.blocks = blocks[i].value; ok = true; }
    }
    if(!ok) return false;
    if(caps[1] & BT_A2DP_SBC_SUBBANDS_8) {
        c.subbands = 8;
    } else if(caps[1] & BT_A2DP_SBC_SUBBANDS_4) {
        c.subbands = 4;
    } else {
        return false;
    }
    if(caps[1] & BT_A2DP_SBC_ALLOC_LOUD) {
        c.allocation = BT_SBC_ALLOC_LOUDNESS;
    } else if(caps[1] & BT_A2DP_SBC_ALLOC_SNR) {
        c.allocation = BT_SBC_ALLOC_SNR;
    } else {
        return false;
    }
    limit = (c.mode == BT_SBC_MODE_MONO || c.mode == BT_SBC_MODE_DUAL_CHANNEL) ?
            16u * c.subbands : 32u * c.subbands;
    if(limit > 250) {
        limit = 250;
    }
    bitpool = caps[3];
    if(bitpool > BT_A2DP_SBC_MAX_BITPOOL) {
        bitpool = BT_A2DP_SBC_MAX_BITPOOL;
    }
    if(bitpool > limit) {
        bitpool = limit;
    }
    if(bitpool < caps[2]) {
        bitpool = caps[2];
    }
    if(bitpool < 2 || bitpool > limit || bitpool > caps[3]) {
        return false;
    }
    c.bitpool = (uint8_t)bitpool;
    if(!bt_sbc_config_valid(&c)) {
        return false;
    }
    *config = c;
    return true;
}

bool bt_a2dp_sbc_select_preferring(const uint8_t caps[4], uint8_t frequency, struct bt_sbc_config *config)
{
    static const uint8_t freq_bits[4] = { BT_A2DP_SBC_FREQ_16000, BT_A2DP_SBC_FREQ_32000,
                                          BT_A2DP_SBC_FREQ_44100, BT_A2DP_SBC_FREQ_48000 };

    if(frequency <= BT_SBC_FREQ_48000 && (caps[0] & freq_bits[frequency])) {
        uint8_t only[4];

        memcpy(only, caps, 4);
        only[0] = (uint8_t)((caps[0] & 0x0f) | freq_bits[frequency]);
        return bt_a2dp_sbc_select(only, config);
    }
    return bt_a2dp_sbc_select(caps, config);
}

void bt_a2dp_sbc_to_octets(const struct bt_sbc_config *config, uint8_t out[4])
{
    static const uint8_t freq_bits[4] = { BT_A2DP_SBC_FREQ_16000, BT_A2DP_SBC_FREQ_32000,
                                          BT_A2DP_SBC_FREQ_44100, BT_A2DP_SBC_FREQ_48000 };
    static const uint8_t mode_bits[4] = { BT_A2DP_SBC_MODE_MONO, BT_A2DP_SBC_MODE_DUAL,
                                          BT_A2DP_SBC_MODE_STEREO, BT_A2DP_SBC_MODE_JOINT };
    static const uint8_t block_bits[4] = { BT_A2DP_SBC_BLOCKS_4, BT_A2DP_SBC_BLOCKS_8,
                                           BT_A2DP_SBC_BLOCKS_12, BT_A2DP_SBC_BLOCKS_16 };

    out[0] = (uint8_t)(freq_bits[config->frequency & 3] | mode_bits[config->mode & 3]);
    out[1] = (uint8_t)(block_bits[(config->blocks / 4 - 1) & 3] |
                       ((config->subbands == 8) ? BT_A2DP_SBC_SUBBANDS_8 : BT_A2DP_SBC_SUBBANDS_4) |
                       ((config->allocation == BT_SBC_ALLOC_SNR) ? BT_A2DP_SBC_ALLOC_SNR : BT_A2DP_SBC_ALLOC_LOUD));
    out[2] = config->bitpool;
    out[3] = config->bitpool;
}

/* Decode a single-choice configuration (as sent in SET_CONFIGURATION). */
static bool sbc_from_octets(const uint8_t o[4], struct bt_sbc_config *config)
{
    struct bt_sbc_config c;

    memset(&c, 0, sizeof(c));
    switch(o[0] & 0xf0) {
    case BT_A2DP_SBC_FREQ_16000: c.frequency = BT_SBC_FREQ_16000; break;
    case BT_A2DP_SBC_FREQ_32000: c.frequency = BT_SBC_FREQ_32000; break;
    case BT_A2DP_SBC_FREQ_44100: c.frequency = BT_SBC_FREQ_44100; break;
    case BT_A2DP_SBC_FREQ_48000: c.frequency = BT_SBC_FREQ_48000; break;
    default: return false;
    }
    switch(o[0] & 0x0f) {
    case BT_A2DP_SBC_MODE_MONO: c.mode = BT_SBC_MODE_MONO; break;
    case BT_A2DP_SBC_MODE_DUAL: c.mode = BT_SBC_MODE_DUAL_CHANNEL; break;
    case BT_A2DP_SBC_MODE_STEREO: c.mode = BT_SBC_MODE_STEREO; break;
    case BT_A2DP_SBC_MODE_JOINT: c.mode = BT_SBC_MODE_JOINT_STEREO; break;
    default: return false;
    }
    switch(o[1] & 0xf0) {
    case BT_A2DP_SBC_BLOCKS_4: c.blocks = 4; break;
    case BT_A2DP_SBC_BLOCKS_8: c.blocks = 8; break;
    case BT_A2DP_SBC_BLOCKS_12: c.blocks = 12; break;
    case BT_A2DP_SBC_BLOCKS_16: c.blocks = 16; break;
    default: return false;
    }
    switch(o[1] & 0x0c) {
    case BT_A2DP_SBC_SUBBANDS_4: c.subbands = 4; break;
    case BT_A2DP_SBC_SUBBANDS_8: c.subbands = 8; break;
    default: return false;
    }
    switch(o[1] & 0x03) {
    case BT_A2DP_SBC_ALLOC_SNR: c.allocation = BT_SBC_ALLOC_SNR; break;
    case BT_A2DP_SBC_ALLOC_LOUD: c.allocation = BT_SBC_ALLOC_LOUDNESS; break;
    default: return false;
    }
    /* A configuration carries a bitpool range; use its upper end. */
    c.bitpool = o[3];
    if(!bt_sbc_config_valid(&c)) {
        return false;
    }
    *config = c;
    return true;
}

/*
 * Walks service capabilities and returns the SBC codec octets (4 bytes),
 * or NULL. Also notes delay reporting support.
 */
static const uint8_t *find_sbc(const uint8_t *p, size_t len, bool *delay_reporting)
{
    const uint8_t *sbc = NULL;

    while(len >= 2) {
        uint8_t cat = p[0], losc = p[1];

        if((size_t)losc + 2 > len) {
            break;
        }
        if(cat == BT_AVDTP_CAT_MEDIA_CODEC && losc >= 6 && (p[2] >> 4) == 0x00 && p[3] == 0x00) {
            sbc = &p[4];
        } else if(cat == BT_AVDTP_CAT_DELAY_REPORTING && delay_reporting) {
            *delay_reporting = true;
        }
        p += losc + 2;
        len -= (size_t)losc + 2;
    }
    return sbc;
}

static void try_next_candidate(struct bt_a2dp_source *src, uint8_t signal, uint8_t error)
{
    if(src->candidate_next < src->candidate_count) {
        src->remote_seid = src->candidates[src->candidate_next++];
        src->remote_delay_reporting = false;
        set_state(src, BT_A2DP_GETTING_CAPABILITIES);
        if(send_seid_command(src, BT_AVDTP_GET_CAPABILITIES) != BT_OK) {
            fail(src, BT_AVDTP_GET_CAPABILITIES, 0);
        }
    } else {
        fail(src, signal, error);
    }
}

static void on_response(struct bt_a2dp_source *src, uint8_t msg_type, uint8_t signal,
                        const uint8_t *p, size_t len)
{
    uint8_t error = 0;

    if(msg_type != BT_AVDTP_MSG_ACCEPT) {
        /* SET_CONFIGURATION rejects carry [category, error], the others
           [error] or [seid, error]; the error is the last octet either way. */
        if(len >= 1) {
            error = p[len - 1];
        }
    }

    switch(signal) {
    case BT_AVDTP_DISCOVER:
        if(src->state != BT_A2DP_DISCOVERING) {
            return;
        }
        if(msg_type != BT_AVDTP_MSG_ACCEPT) {
            fail(src, signal, error);
            return;
        }
        while(len >= 2 && src->candidate_count < sizeof(src->candidates)) {
            uint8_t seid = p[0] >> 2;
            bool in_use = (p[0] & 0x02) != 0;
            uint8_t media = p[1] >> 4;
            bool sink = (p[1] & 0x08) != 0;

            if(!in_use && media == 0x00 && sink) {
                src->candidates[src->candidate_count++] = seid;
            }
            p += 2;
            len -= 2;
        }
        try_next_candidate(src, signal, 0);
        return;

    case BT_AVDTP_GET_CAPABILITIES:
    case BT_AVDTP_GET_ALL_CAPABILITIES:
        if(src->state != BT_A2DP_GETTING_CAPABILITIES) {
            return;
        }
        if(msg_type == BT_AVDTP_MSG_ACCEPT) {
            const uint8_t *sbc = find_sbc(p, len, &src->remote_delay_reporting);
            uint8_t params[2 + 2 + 2 + 6];

            if(sbc && bt_a2dp_sbc_select_preferring(sbc, src->prefer_frequency, &src->config)) {
                params[0] = (uint8_t)(src->remote_seid << 2);
                params[1] = (uint8_t)(BT_AVDTP_LOCAL_SEID << 2);
                params[2] = BT_AVDTP_CAT_MEDIA_TRANSPORT;
                params[3] = 0;
                params[4] = BT_AVDTP_CAT_MEDIA_CODEC;
                params[5] = 6;
                params[6] = 0x00; /* audio */
                params[7] = 0x00; /* SBC */
                bt_a2dp_sbc_to_octets(&src->config, &params[8]);
                set_state(src, BT_A2DP_CONFIGURING);
                if(send_command(src, BT_AVDTP_SET_CONFIGURATION, params, sizeof(params)) != BT_OK) {
                    fail(src, BT_AVDTP_SET_CONFIGURATION, 0);
                }
                return;
            }
        }
        try_next_candidate(src, signal, error);
        return;

    case BT_AVDTP_SET_CONFIGURATION:
        if(src->state != BT_A2DP_CONFIGURING) {
            return;
        }
        if(msg_type != BT_AVDTP_MSG_ACCEPT) {
            try_next_candidate(src, signal, error);
            return;
        }
        set_state(src, BT_A2DP_OPENING);
        if(send_seid_command(src, BT_AVDTP_OPEN) != BT_OK) {
            fail(src, BT_AVDTP_OPEN, 0);
        }
        return;

    case BT_AVDTP_OPEN:
        if(src->state != BT_A2DP_OPENING) {
            return;
        }
        if(msg_type != BT_AVDTP_MSG_ACCEPT) {
            fail(src, signal, error);
            return;
        }
        set_state(src, BT_A2DP_MEDIA_PENDING);
        if(src->ops->open_media) {
            src->ops->open_media(src->context);
        }
        return;

    case BT_AVDTP_START:
        if(src->state != BT_A2DP_STARTING) {
            return;
        }
        if(msg_type != BT_AVDTP_MSG_ACCEPT) {
            fail(src, signal, error);
            return;
        }
        set_state(src, BT_A2DP_STREAMING);
        return;

    case BT_AVDTP_SUSPEND:
        if(msg_type == BT_AVDTP_MSG_ACCEPT && src->state == BT_A2DP_STREAMING) {
            set_state(src, BT_A2DP_SUSPENDED);
        }
        return;

    default:
        return;
    }
}

static void on_command(struct bt_a2dp_source *src, uint8_t label, uint8_t signal,
                       const uint8_t *p, size_t len)
{
    uint8_t r[16];
    uint8_t seid = (len >= 1) ? (uint8_t)(p[0] >> 2) : 0;

    switch(signal) {
    case BT_AVDTP_DISCOVER:
        r[0] = (uint8_t)((BT_AVDTP_LOCAL_SEID << 2) |
                         ((src->state >= BT_A2DP_CONFIGURING && src->state != BT_A2DP_FAILED) ? 0x02 : 0x00));
        r[1] = 0x00; /* audio, source */
        send_pdu(src, label, BT_AVDTP_MSG_ACCEPT, signal, r, 2);
        return;

    case BT_AVDTP_GET_CAPABILITIES:
    case BT_AVDTP_GET_ALL_CAPABILITIES:
        if(seid != BT_AVDTP_LOCAL_SEID) {
            r[0] = AVDTP_ERR_BAD_ACP_SEID;
            send_pdu(src, label, BT_AVDTP_MSG_REJECT, signal, r, 1);
            return;
        }
        r[0] = BT_AVDTP_CAT_MEDIA_TRANSPORT;
        r[1] = 0;
        r[2] = BT_AVDTP_CAT_MEDIA_CODEC;
        r[3] = 6;
        r[4] = 0x00;
        r[5] = 0x00;
        memcpy(&r[6], local_sbc_caps, 4);
        send_pdu(src, label, BT_AVDTP_MSG_ACCEPT, signal, r, 10);
        return;

    case BT_AVDTP_SET_CONFIGURATION: {
        const uint8_t *sbc;
        struct bt_sbc_config c;

        if(seid != BT_AVDTP_LOCAL_SEID || len < 2) {
            r[0] = 0;
            r[1] = AVDTP_ERR_BAD_ACP_SEID;
            send_pdu(src, label, BT_AVDTP_MSG_REJECT, signal, r, 2);
            return;
        }
        if(src->state != BT_A2DP_IDLE && src->state != BT_A2DP_FAILED &&
           src->state != BT_A2DP_DISCOVERING && src->state != BT_A2DP_GETTING_CAPABILITIES) {
            r[0] = 0;
            r[1] = AVDTP_ERR_SEP_IN_USE;
            send_pdu(src, label, BT_AVDTP_MSG_REJECT, signal, r, 2);
            return;
        }
        sbc = find_sbc(p + 2, len - 2, &src->remote_delay_reporting);
        if(!sbc || !sbc_from_octets(sbc, &c)) {
            r[0] = BT_AVDTP_CAT_MEDIA_CODEC;
            r[1] = 0x29; /* unsupported configuration */
            send_pdu(src, label, BT_AVDTP_MSG_REJECT, signal, r, 2);
            return;
        }
        src->config = c;
        src->remote_seid = (uint8_t)(p[1] >> 2);
        src->initiator = false;
        send_pdu(src, label, BT_AVDTP_MSG_ACCEPT, signal, NULL, 0);
        /* The sink drives from here: it sends OPEN, opens the media
           channel, then sends START. */
        set_state(src, BT_A2DP_OPENING);
        return;
    }

    case BT_AVDTP_OPEN:
    case BT_AVDTP_START:
    case BT_AVDTP_SUSPEND:
    case BT_AVDTP_CLOSE:
    case BT_AVDTP_ABORT:
        if(seid != BT_AVDTP_LOCAL_SEID) {
            r[0] = (uint8_t)(seid << 2);
            r[1] = AVDTP_ERR_BAD_ACP_SEID;
            send_pdu(src, label, BT_AVDTP_MSG_REJECT, signal, r, (signal == BT_AVDTP_OPEN ||
                     signal == BT_AVDTP_CLOSE) ? 1 : 2);
            return;
        }
        send_pdu(src, label, BT_AVDTP_MSG_ACCEPT, signal, NULL, 0);
        if(signal == BT_AVDTP_OPEN) {
            /* The sink opens the transport channel; the owner accepts it
               and calls bt_a2dp_source_media_open(). */
            set_state(src, BT_A2DP_MEDIA_PENDING);
        } else if(signal == BT_AVDTP_START) {
            set_state(src, BT_A2DP_STREAMING);
        } else if(signal == BT_AVDTP_SUSPEND) {
            set_state(src, BT_A2DP_SUSPENDED);
        } else {
            set_state(src, BT_A2DP_IDLE);
        }
        return;

    case BT_AVDTP_DELAY_REPORT:
        send_pdu(src, label, BT_AVDTP_MSG_ACCEPT, signal, NULL, 0);
        return;

    default:
        /* Unknown or unsupported: general reject. */
        send_pdu(src, label, BT_AVDTP_MSG_GENERAL_REJECT, signal, NULL, 0);
        return;
    }
}

void bt_a2dp_source_on_signal(struct bt_a2dp_source *src, const uint8_t *pdu, size_t len)
{
    uint8_t label, packet_type, msg_type, signal;

    if(len < 2) {
        return;
    }
    label = pdu[0] >> 4;
    packet_type = (pdu[0] >> 2) & 0x03;
    msg_type = pdu[0] & 0x03;
    if(packet_type != 0) {
        /* Fragmented signalling is not needed for the PDUs we exchange. */
        return;
    }
    signal = pdu[1] & 0x3f;
    if(msg_type == BT_AVDTP_MSG_COMMAND) {
        on_command(src, label, signal, pdu + 2, len - 2);
    } else if(label == src->label) {
        on_response(src, msg_type, signal, pdu + 2, len - 2);
    }
}

bt_status_t bt_a2dp_source_media_open(struct bt_a2dp_source *src)
{
    if(src->state != BT_A2DP_MEDIA_PENDING) {
        return BT_ERR_INVALID_STATE;
    }
    if(!src->initiator) {
        /* Either peer may send START; the owner starts when it has audio. */
        return BT_OK;
    }
    set_state(src, BT_A2DP_STARTING);
    return send_seid_command(src, BT_AVDTP_START);
}

bt_status_t bt_a2dp_source_suspend(struct bt_a2dp_source *src)
{
    if(src->state != BT_A2DP_STREAMING) {
        return BT_ERR_INVALID_STATE;
    }
    return send_seid_command(src, BT_AVDTP_SUSPEND);
}

bt_status_t bt_a2dp_source_start(struct bt_a2dp_source *src)
{
    if(src->state != BT_A2DP_SUSPENDED && src->state != BT_A2DP_MEDIA_PENDING) {
        return BT_ERR_INVALID_STATE;
    }
    set_state(src, BT_A2DP_STARTING);
    return send_seid_command(src, BT_AVDTP_START);
}

void bt_a2dp_packetizer_init(struct bt_a2dp_packetizer *pk, uint32_t ssrc)
{
    pk->sequence = 0;
    pk->timestamp = 0;
    pk->ssrc = ssrc;
}

unsigned bt_a2dp_frames_per_packet(size_t mtu, size_t frame_len)
{
    size_t n;

    if(frame_len == 0 || mtu <= 13) {
        return 0;
    }
    n = (mtu - 13) / frame_len;
    return (unsigned)((n > 15) ? 15 : n);
}

size_t bt_a2dp_build_media_packet(struct bt_a2dp_packetizer *pk, const uint8_t *frames,
                                  size_t frame_len, unsigned frame_count,
                                  unsigned samples_per_frame, uint8_t *out, size_t out_len)
{
    size_t len = 13 + frame_len * frame_count;

    if(frame_count == 0 || frame_count > 15 || len > out_len) {
        return 0;
    }
    out[0] = 0x80;                  /* RTP version 2 */
    out[1] = 96;                    /* dynamic payload type, no marker */
    out[2] = (uint8_t)(pk->sequence >> 8);
    out[3] = (uint8_t)pk->sequence;
    out[4] = (uint8_t)(pk->timestamp >> 24);
    out[5] = (uint8_t)(pk->timestamp >> 16);
    out[6] = (uint8_t)(pk->timestamp >> 8);
    out[7] = (uint8_t)pk->timestamp;
    out[8] = (uint8_t)(pk->ssrc >> 24);
    out[9] = (uint8_t)(pk->ssrc >> 16);
    out[10] = (uint8_t)(pk->ssrc >> 8);
    out[11] = (uint8_t)pk->ssrc;
    out[12] = (uint8_t)(frame_count & 0x0f); /* not fragmented */
    memcpy(&out[13], frames, frame_len * frame_count);
    pk->sequence++;
    pk->timestamp += (uint32_t)samples_per_frame * frame_count;
    return len;
}
