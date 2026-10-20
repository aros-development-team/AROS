#include "test_sbc.h"
#include "../support/test.h"

#include <btcore/sbc.h>

#include <stdint.h>
#include <string.h>

/* The common high-quality A2DP setting: 44.1 kHz, joint stereo, loudness,
   8 subbands, 16 blocks, bitpool 53. */
static const struct bt_sbc_config hq = {
    BT_SBC_FREQ_44100, 16, BT_SBC_MODE_JOINT_STEREO, BT_SBC_ALLOC_LOUDNESS, 8, 53
};

static void test_sizes(void)
{
    struct bt_sbc_config mono = { BT_SBC_FREQ_44100, 16, BT_SBC_MODE_MONO, BT_SBC_ALLOC_SNR, 8, 64 };
    struct bt_sbc_config dual = { BT_SBC_FREQ_48000, 8, BT_SBC_MODE_DUAL_CHANNEL, BT_SBC_ALLOC_SNR, 4, 32 };

    BT_CHECK(bt_sbc_frame_length(&hq) == 119);
    BT_CHECK(bt_sbc_codesize(&hq) == 512);
    BT_CHECK(bt_sbc_bitrate(&hq) == 327993);
    /* 4 + 4 + ceil(16 * 64 / 8) */
    BT_CHECK(bt_sbc_frame_length(&mono) == 136);
    BT_CHECK(bt_sbc_codesize(&mono) == 256);
    /* 4 + 4 + ceil(8 * 2 * 32 / 8) */
    BT_CHECK(bt_sbc_frame_length(&dual) == 72);
}

static void test_config_limits(void)
{
    struct bt_sbc_config c = hq;

    BT_CHECK(bt_sbc_config_valid(&c));
    c.bitpool = 1;
    BT_CHECK(!bt_sbc_config_valid(&c));
    c.bitpool = 250;
    BT_CHECK(bt_sbc_config_valid(&c));
    c.bitpool = 251;
    BT_CHECK(!bt_sbc_config_valid(&c));
    c = hq;
    c.mode = BT_SBC_MODE_MONO;
    c.bitpool = 129; /* mono limit is 16 * subbands */
    BT_CHECK(!bt_sbc_config_valid(&c));
    c = hq;
    c.blocks = 6;
    BT_CHECK(!bt_sbc_config_valid(&c));
    c = hq;
    c.subbands = 6;
    BT_CHECK(!bt_sbc_config_valid(&c));
    BT_CHECK(bt_sbc_frame_length(&c) == 0);
}

static void test_crc_reference_frame(void)
{
    /* First frame of a stream from the reference encoder (libsbc): mono,
       44.1 kHz, 16 blocks, SNR, 8 subbands, bitpool 64. The CRC covers the
       two octets after the syncword and the 32 scale factor bits. */
    static const uint8_t covered[] = { 0xb3, 0x40, 0xd7, 0x65, 0x44, 0x34 };

    BT_CHECK(bt_sbc_crc8(covered, sizeof(covered) * 8) == 0x5c);
}

static void test_silence_frame(void)
{
    struct bt_sbc_encoder enc;
    int16_t pcm[16 * 8 * 2];
    uint8_t frame[128];
    uint8_t covered[11];
    size_t i;

    memset(pcm, 0, sizeof(pcm));
    BT_CHECK(bt_sbc_encoder_init(&enc, &hq));
    BT_CHECK(bt_sbc_encode(&enc, pcm, frame, sizeof(frame)) == 119);
    BT_CHECK(frame[0] == 0x9c);
    /* 44.1 kHz (10), 16 blocks (11), joint stereo (11), loudness (0),
       8 subbands (1) */
    BT_CHECK(frame[1] == 0xbd);
    BT_CHECK(frame[2] == 53);
    /* No joined subbands and all scale factors zero for silence. */
    for(i = 4; i < 4 + 1 + 8; i++) {
        BT_CHECK(frame[i] == 0);
    }
    /* The CRC in the frame matches the covered bits: 16 header bits, 8 join
       bits and 64 scale factor bits. */
    covered[0] = frame[1];
    covered[1] = frame[2];
    memcpy(&covered[2], &frame[4], 9);
    BT_CHECK(frame[3] == bt_sbc_crc8(covered, 16 + 8 + 64));
    /* Too small an output buffer is refused. */
    BT_CHECK(bt_sbc_encode(&enc, pcm, frame, 118) == 0);
}

static void test_tone_lands_in_lowest_subband(void)
{
    struct bt_sbc_config mono = { BT_SBC_FREQ_44100, 16, BT_SBC_MODE_MONO, BT_SBC_ALLOC_SNR, 8, 64 };
    struct bt_sbc_encoder enc;
    int16_t pcm[16 * 8];
    uint8_t frame[160];
    unsigned f, i;
    /* 689.06 Hz (fs / 64), a sine with a whole number of periods per
       frame, so every frame sees the same input. It lies in subband 0
       (0..1378 Hz). */
    static const int16_t period[64] = {
        0, 1606, 3196, 4756, 6270, 7723, 9102, 10394,
        11585, 12665, 13623, 14449, 15137, 15679, 16069, 16305,
        16384, 16305, 16069, 15679, 15137, 14449, 13623, 12665,
        11585, 10394, 9102, 7723, 6270, 4756, 3196, 1606,
        0, -1606, -3196, -4756, -6270, -7723, -9102, -10394,
        -11585, -12665, -13623, -14449, -15137, -15679, -16069, -16305,
        -16384, -16305, -16069, -15679, -15137, -14449, -13623, -12665,
        -11585, -10394, -9102, -7723, -6270, -4756, -3196, -1606,
    };

    BT_CHECK(bt_sbc_encoder_init(&enc, &mono));
    for(f = 0; f < 4; f++) {
        for(i = 0; i < 128; i++) {
            pcm[i] = period[i % 64];
        }
        BT_CHECK(bt_sbc_encode(&enc, pcm, frame, sizeof(frame)) == 136);
    }
    /* After the filter has settled, subband 0 carries the signal and the
       others stay small. */
    {
        unsigned sf0 = frame[4] >> 4;
        unsigned sb;

        BT_CHECK(sf0 >= 12);
        for(sb = 1; sb < 8; sb++) {
            unsigned sf = (sb & 1) ? (frame[4 + sb / 2] & 0x0f) : (frame[4 + sb / 2] >> 4);

            BT_CHECK(sf + 6 <= sf0);
        }
    }
}

void run_sbc_tests(void)
{
    test_sizes();
    test_config_limits();
    test_crc_reference_frame();
    test_silence_frame();
    test_tone_lands_in_lowest_subband();
}
