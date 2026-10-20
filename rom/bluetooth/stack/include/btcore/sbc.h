#ifndef BTCORE_SBC_H
#define BTCORE_SBC_H

#include <btcore/types.h>

/*
 * SBC (low-complexity subband codec) encoder, as specified in the A2DP
 * specification's SBC appendix. It is the codec every A2DP sink must
 * support. Integer arithmetic only, so it runs on targets without an FPU.
 *
 * The enum values are the ones carried in the SBC frame header, which are
 * not the same bit layout as the A2DP codec-capability octets.
 */

enum bt_sbc_frequency
{
    BT_SBC_FREQ_16000 = 0,
    BT_SBC_FREQ_32000 = 1,
    BT_SBC_FREQ_44100 = 2,
    BT_SBC_FREQ_48000 = 3
};

enum bt_sbc_mode
{
    BT_SBC_MODE_MONO = 0,
    BT_SBC_MODE_DUAL_CHANNEL = 1,
    BT_SBC_MODE_STEREO = 2,
    BT_SBC_MODE_JOINT_STEREO = 3
};

enum bt_sbc_allocation
{
    BT_SBC_ALLOC_LOUDNESS = 0,
    BT_SBC_ALLOC_SNR = 1
};

struct bt_sbc_config
{
    uint8_t frequency;  /* enum bt_sbc_frequency */
    uint8_t blocks;     /* 4, 8, 12 or 16 */
    uint8_t mode;       /* enum bt_sbc_mode */
    uint8_t allocation; /* enum bt_sbc_allocation */
    uint8_t subbands;   /* 4 or 8 */
    uint8_t bitpool;    /* 2..250, further limited by subbands and mode */
};

struct bt_sbc_encoder
{
    struct bt_sbc_config config;
    uint8_t channels;
    /* Analysis filter history per channel, newest sample first. */
    int32_t history[2][80];
};

/* Returns false if config is not a valid SBC configuration. */
bool bt_sbc_config_valid(const struct bt_sbc_config *config);

/* Size in bytes of one encoded frame for config (0 if invalid). */
size_t bt_sbc_frame_length(const struct bt_sbc_config *config);

/* Bytes of 16-bit interleaved PCM consumed per frame (0 if invalid). */
size_t bt_sbc_codesize(const struct bt_sbc_config *config);

/* Bit rate of the encoded stream in bits per second (0 if invalid). */
uint32_t bt_sbc_bitrate(const struct bt_sbc_config *config);

/* Returns false if config is invalid. */
bool bt_sbc_encoder_init(struct bt_sbc_encoder *encoder, const struct bt_sbc_config *config);

/*
 * Encodes one frame from blocks * subbands samples per channel of native-
 * endian, interleaved, signed 16-bit PCM (bt_sbc_codesize() bytes). Writes
 * bt_sbc_frame_length() bytes to out and returns that count, or 0 if
 * out_len is too small.
 */
size_t bt_sbc_encode(struct bt_sbc_encoder *encoder, const int16_t *pcm,
                     uint8_t *out, size_t out_len);

/* CRC-8 used in the SBC frame header (polynomial 0x1D, initial value
 * 0x0F) over the first bit_count bits of data, most significant bit
 * first. Exposed for tests and for decoders. */
uint8_t bt_sbc_crc8(const uint8_t *data, size_t bit_count);

#endif /* BTCORE_SBC_H */
