#include "test_a2dp_source.h"
#include "../support/test.h"
#include "../../../classes/btaudio/stream_state.h"

#include <btcore/a2dp_source.h>

#include <stdint.h>
#include <string.h>

struct fake_link
{
    uint8_t sent[8][32];
    size_t sent_len[8];
    unsigned sent_count;
    unsigned media_requests;
    enum bt_a2dp_source_state last_state;
};

static bt_status_t fake_send(void *context, const uint8_t *pdu, size_t len)
{
    struct fake_link *l = context;

    if(l->sent_count < 8 && len <= 32) {
        memcpy(l->sent[l->sent_count], pdu, len);
        l->sent_len[l->sent_count] = len;
    }
    l->sent_count++;
    return BT_OK;
}

static void fake_open_media(void *context)
{
    ((struct fake_link *)context)->media_requests++;
}

static void fake_state(void *context, enum bt_a2dp_source_state state)
{
    ((struct fake_link *)context)->last_state = state;
}

static const struct bt_a2dp_source_ops fake_ops = { fake_send, fake_open_media, fake_state };

/* The last PDU the source sent. */
static const uint8_t *last_sent(const struct fake_link *l, size_t *len)
{
    *len = l->sent_len[l->sent_count - 1];
    return l->sent[l->sent_count - 1];
}

/* Feed an accept for the last command, with params. */
static void accept_last(struct bt_a2dp_source *src, struct fake_link *l, const uint8_t *params, size_t n)
{
    size_t len;
    const uint8_t *cmd = last_sent(l, &len);
    uint8_t pdu[40];

    pdu[0] = (uint8_t)((cmd[0] & 0xf0) | BT_AVDTP_MSG_ACCEPT);
    pdu[1] = cmd[1];
    if(n) {
        memcpy(&pdu[2], params, n);
    }
    bt_a2dp_source_on_signal(src, pdu, n + 2);
}

static void test_sbc_select(void)
{
    struct bt_sbc_config c;
    /* A typical speaker: 44.1 and 48 kHz, all modes, all blocks, both
       subband counts, both allocations, bitpool 2..53. */
    static const uint8_t speaker[4] = { 0x3f, 0xff, 2, 53 };
    /* Mono-only 16 kHz headset, bitpool 2..32. */
    static const uint8_t headset[4] = { 0x88, 0x15, 2, 32 };
    static const uint8_t none[4] = { 0x0f, 0xff, 2, 53 };
    uint8_t o[4];

    BT_CHECK(bt_a2dp_sbc_select(speaker, &c));
    BT_CHECK(c.frequency == BT_SBC_FREQ_44100 && c.mode == BT_SBC_MODE_JOINT_STEREO);
    BT_CHECK(c.blocks == 16 && c.subbands == 8 && c.allocation == BT_SBC_ALLOC_LOUDNESS);
    BT_CHECK(c.bitpool == 53);
    bt_a2dp_sbc_to_octets(&c, o);
    BT_CHECK(o[0] == 0x21 && o[1] == 0x15 && o[2] == 53 && o[3] == 53);

    BT_CHECK(bt_a2dp_sbc_select(headset, &c));
    BT_CHECK(c.frequency == BT_SBC_FREQ_16000 && c.mode == BT_SBC_MODE_MONO);
    BT_CHECK(c.blocks == 16 && c.subbands == 8 && c.allocation == BT_SBC_ALLOC_LOUDNESS);
    BT_CHECK(c.bitpool == 32);

    BT_CHECK(!bt_a2dp_sbc_select(none, &c));

    /* A 48 kHz mix picks 48 kHz when the sink has it, and falls back
       otherwise. */
    BT_CHECK(bt_a2dp_sbc_select_preferring(speaker, BT_SBC_FREQ_48000, &c));
    BT_CHECK(c.frequency == BT_SBC_FREQ_48000 && c.mode == BT_SBC_MODE_JOINT_STEREO);
    BT_CHECK(bt_a2dp_sbc_select_preferring(headset, BT_SBC_FREQ_48000, &c));
    BT_CHECK(c.frequency == BT_SBC_FREQ_16000);
}

static void test_initiator_flow(void)
{
    struct bt_a2dp_source src;
    struct fake_link l;
    size_t len;
    const uint8_t *p;
    /* DISCOVER: SEID 1 is an in-use sink, SEID 2 a video sink, SEID 3 a free
       audio sink. */
    static const uint8_t endpoints[] = { 0x06, 0x08, 0x08, 0x18, 0x0c, 0x08 };
    static const uint8_t caps[] = {
        BT_AVDTP_CAT_MEDIA_TRANSPORT, 0,
        BT_AVDTP_CAT_MEDIA_CODEC, 6, 0x00, 0x00, 0x3f, 0xff, 2, 53,
        BT_AVDTP_CAT_DELAY_REPORTING, 0
    };

    memset(&l, 0, sizeof(l));
    bt_a2dp_source_init(&src, &fake_ops, &l);
    BT_CHECK(bt_a2dp_source_connect(&src) == BT_OK);
    p = last_sent(&l, &len);
    BT_CHECK(len == 2 && (p[0] & 0x0f) == 0 && p[1] == BT_AVDTP_DISCOVER);

    accept_last(&src, &l, endpoints, sizeof(endpoints));
    p = last_sent(&l, &len);
    BT_CHECK(len == 3 && p[1] == BT_AVDTP_GET_CAPABILITIES && p[2] == (3 << 2));
    BT_CHECK(src.state == BT_A2DP_GETTING_CAPABILITIES);

    accept_last(&src, &l, caps, sizeof(caps));
    BT_CHECK(src.remote_delay_reporting);
    p = last_sent(&l, &len);
    BT_CHECK(p[1] == BT_AVDTP_SET_CONFIGURATION && len == 14);
    BT_CHECK(p[2] == (3 << 2) && p[3] == (BT_AVDTP_LOCAL_SEID << 2));
    BT_CHECK(p[4] == BT_AVDTP_CAT_MEDIA_TRANSPORT && p[5] == 0);
    BT_CHECK(p[6] == BT_AVDTP_CAT_MEDIA_CODEC && p[7] == 6 && p[8] == 0 && p[9] == 0);
    BT_CHECK(p[10] == 0x21 && p[11] == 0x15 && p[12] == 53 && p[13] == 53);

    accept_last(&src, &l, NULL, 0);
    p = last_sent(&l, &len);
    BT_CHECK(p[1] == BT_AVDTP_OPEN && p[2] == (3 << 2));
    BT_CHECK(l.media_requests == 0);

    accept_last(&src, &l, NULL, 0);
    BT_CHECK(src.state == BT_A2DP_MEDIA_PENDING && l.media_requests == 1);

    BT_CHECK(bt_a2dp_source_media_open(&src) == BT_OK);
    p = last_sent(&l, &len);
    BT_CHECK(p[1] == BT_AVDTP_START && src.state == BT_A2DP_STARTING);

    accept_last(&src, &l, NULL, 0);
    BT_CHECK(src.state == BT_A2DP_STREAMING && l.last_state == BT_A2DP_STREAMING);
    BT_CHECK(src.config.bitpool == 53);

    /* A response with a stale transaction label is ignored. */
    {
        uint8_t stale[2] = { (uint8_t)(((src.label + 5) & 0x0f) << 4 | BT_AVDTP_MSG_ACCEPT), BT_AVDTP_SUSPEND };

        bt_a2dp_source_on_signal(&src, stale, 2);
        BT_CHECK(src.state == BT_A2DP_STREAMING);
    }
}

static void test_reject_fails(void)
{
    struct bt_a2dp_source src;
    struct fake_link l;
    size_t len;
    const uint8_t *cmd;
    uint8_t pdu[4];

    memset(&l, 0, sizeof(l));
    bt_a2dp_source_init(&src, &fake_ops, &l);
    bt_a2dp_source_connect(&src);
    cmd = last_sent(&l, &len);
    pdu[0] = (uint8_t)((cmd[0] & 0xf0) | BT_AVDTP_MSG_REJECT);
    pdu[1] = BT_AVDTP_DISCOVER;
    pdu[2] = 0x31;
    bt_a2dp_source_on_signal(&src, pdu, 3);
    BT_CHECK(src.state == BT_A2DP_FAILED && src.fail_error == 0x31);
}

static void test_acceptor_flow(void)
{
    /* The sink drives: DISCOVER, GET_CAPABILITIES, SET_CONFIGURATION, OPEN,
       (media channel), START. */
    struct bt_a2dp_source src;
    struct fake_link l;
    size_t len;
    const uint8_t *p;
    static const uint8_t discover[] = { 0x10, BT_AVDTP_DISCOVER };
    static const uint8_t getcaps[] = { 0x20, BT_AVDTP_GET_CAPABILITIES, BT_AVDTP_LOCAL_SEID << 2 };
    static const uint8_t setconf[] = { 0x30, BT_AVDTP_SET_CONFIGURATION, BT_AVDTP_LOCAL_SEID << 2, 4 << 2,
                                       BT_AVDTP_CAT_MEDIA_TRANSPORT, 0,
                                       BT_AVDTP_CAT_MEDIA_CODEC, 6, 0x00, 0x00, 0x21, 0x15, 2, 35 };
    static const uint8_t open_cmd[] = { 0x40, BT_AVDTP_OPEN, BT_AVDTP_LOCAL_SEID << 2 };
    static const uint8_t start_cmd[] = { 0x50, BT_AVDTP_START, BT_AVDTP_LOCAL_SEID << 2 };
    static const uint8_t bogus[] = { 0x60, 0x3e };

    memset(&l, 0, sizeof(l));
    bt_a2dp_source_init(&src, &fake_ops, &l);

    bt_a2dp_source_on_signal(&src, discover, sizeof(discover));
    p = last_sent(&l, &len);
    BT_CHECK(len == 4 && p[0] == (0x10 | BT_AVDTP_MSG_ACCEPT) && p[2] == (BT_AVDTP_LOCAL_SEID << 2) && p[3] == 0x00);

    bt_a2dp_source_on_signal(&src, getcaps, sizeof(getcaps));
    p = last_sent(&l, &len);
    BT_CHECK(len == 12 && p[0] == (0x20 | BT_AVDTP_MSG_ACCEPT) && p[4] == BT_AVDTP_CAT_MEDIA_CODEC);

    bt_a2dp_source_on_signal(&src, setconf, sizeof(setconf));
    p = last_sent(&l, &len);
    BT_CHECK(len == 2 && p[0] == (0x30 | BT_AVDTP_MSG_ACCEPT));
    BT_CHECK(src.remote_seid == 4 && src.config.bitpool == 35 && !src.initiator);

    bt_a2dp_source_on_signal(&src, open_cmd, sizeof(open_cmd));
    BT_CHECK(src.state == BT_A2DP_MEDIA_PENDING);
    BT_CHECK(bt_a2dp_source_media_open(&src) == BT_OK);
    BT_CHECK(src.state == BT_A2DP_MEDIA_PENDING); /* waits for the sink's START */

    bt_a2dp_source_on_signal(&src, start_cmd, sizeof(start_cmd));
    BT_CHECK(src.state == BT_A2DP_STREAMING);

    bt_a2dp_source_on_signal(&src, bogus, sizeof(bogus));
    p = last_sent(&l, &len);
    BT_CHECK(len == 2 && p[0] == (0x60 | BT_AVDTP_MSG_GENERAL_REJECT) && p[1] == 0x3e);
}

static void test_sink_open_without_start(void)
{
    struct bt_a2dp_source src;
    struct fake_link link = {0};
    size_t len;
    const uint8_t *pdu;
    static const uint8_t config[] = {0x30, BT_AVDTP_SET_CONFIGURATION,
        BT_AVDTP_LOCAL_SEID << 2, 4 << 2, BT_AVDTP_CAT_MEDIA_TRANSPORT, 0,
        BT_AVDTP_CAT_MEDIA_CODEC, 6, 0, 0, 0x21, 0x15, 2, 35};
    static const uint8_t open[] = {0x40, BT_AVDTP_OPEN, BT_AVDTP_LOCAL_SEID << 2};

    bt_a2dp_source_init(&src, &fake_ops, &link);
    bt_a2dp_source_on_signal(&src, config, sizeof(config));
    bt_a2dp_source_on_signal(&src, open, sizeof(open));
    BT_CHECK(!src.initiator && src.state == BT_A2DP_MEDIA_PENDING);
    /* Missing transport is still setup, but an open idle transport is not
       subject to the worker's setup timeout and may be started locally. */
    BT_CHECK(!bStreamWaiting(src.state, false));
    BT_CHECK(bt_a2dp_source_media_open(&src) == BT_OK);
    BT_CHECK(bStreamWaiting(src.state, true));
    BT_CHECK(bt_a2dp_source_start(&src) == BT_OK);
    pdu = last_sent(&link, &len);
    BT_CHECK(len == 3 && pdu[1] == BT_AVDTP_START && pdu[2] == (4 << 2));
    BT_CHECK(src.state == BT_A2DP_STARTING && !bStreamWaiting(src.state, true));
    accept_last(&src, &link, NULL, 0);
    BT_CHECK(src.state == BT_A2DP_STREAMING && !src.initiator);
}

static void test_media_packets(void)
{
    struct bt_a2dp_packetizer pk;
    uint8_t frames[119 * 7];
    uint8_t out[895];
    size_t n;

    memset(frames, 0xaa, sizeof(frames));
    BT_CHECK(bt_a2dp_frames_per_packet(895, 119) == 7);
    BT_CHECK(bt_a2dp_frames_per_packet(672, 119) == 5);
    BT_CHECK(bt_a2dp_frames_per_packet(13, 119) == 0);

    bt_a2dp_packetizer_init(&pk, 0x11223344);
    n = bt_a2dp_build_media_packet(&pk, frames, 119, 7, 128, out, sizeof(out));
    BT_CHECK(n == 13 + 7 * 119);
    BT_CHECK(out[0] == 0x80 && out[1] == 96 && out[2] == 0 && out[3] == 0);
    BT_CHECK(out[4] == 0 && out[7] == 0);
    BT_CHECK(out[8] == 0x11 && out[11] == 0x44 && out[12] == 7);
    n = bt_a2dp_build_media_packet(&pk, frames, 119, 7, 128, out, sizeof(out));
    BT_CHECK(n != 0 && out[3] == 1);
    /* timestamp advanced by 7 frames of 128 samples */
    BT_CHECK(out[6] == (896 >> 8) && out[7] == (896 & 0xff));
    BT_CHECK(bt_a2dp_build_media_packet(&pk, frames, 119, 16, 128, out, sizeof(out)) == 0);
    BT_CHECK(bt_a2dp_build_media_packet(&pk, frames, 119, 7, 128, out, 100) == 0);
}

static void test_malformed_avdtp(void)
{
    struct bt_a2dp_source src;
    struct fake_link link = {0};
    const uint8_t short_pdu[] = {0x10};
    const uint8_t fragmented[] = {0x14, BT_AVDTP_DISCOVER};
    const uint8_t bad_seid[] = {0x20, BT_AVDTP_OPEN, 63 << 2};
    const uint8_t truncated_config[] = {0x30, BT_AVDTP_SET_CONFIGURATION,
        BT_AVDTP_LOCAL_SEID << 2, 4 << 2, BT_AVDTP_CAT_MEDIA_CODEC, 6, 0};
    size_t len;
    const uint8_t *reply;
    bt_a2dp_source_init(&src, &fake_ops, &link);
    bt_a2dp_source_on_signal(&src, short_pdu, sizeof(short_pdu));
    bt_a2dp_source_on_signal(&src, fragmented, sizeof(fragmented));
    BT_CHECK(link.sent_count == 0 && src.state == BT_A2DP_IDLE);
    bt_a2dp_source_on_signal(&src, bad_seid, sizeof(bad_seid));
    reply = last_sent(&link, &len);
    BT_CHECK(len == 3 && (reply[0] & 3) == BT_AVDTP_MSG_REJECT);
    BT_CHECK(src.state == BT_A2DP_IDLE);
    bt_a2dp_source_on_signal(&src, truncated_config, sizeof(truncated_config));
    reply = last_sent(&link, &len);
    BT_CHECK(len == 4 && (reply[0] & 3) == BT_AVDTP_MSG_REJECT);
    BT_CHECK(src.state == BT_A2DP_IDLE && link.media_requests == 0);
}

void run_a2dp_source_tests(void)
{
    test_malformed_avdtp();
    test_sbc_select();
    test_initiator_flow();
    test_reject_fails();
    test_acceptor_flow();
    test_sink_open_without_start();
    test_media_packets();
}
