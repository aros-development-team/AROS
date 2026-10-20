#include <btcore/sbc.h>

#include <stdint.h>
#include <string.h>

/*
 * SBC encoder following the encoder description in the A2DP
 * specification's SBC appendix: windowed analysis filterbank, scale
 * factors, optional joint stereo, bit allocation, quantisation and frame
 * packing. Fixed point throughout: PCM in, subband samples in Q15.
 */

#define SBC_SYNCWORD 0x9C

/* Analysis windows C[i] (A2DP specification, SBC appendix, tables
   Proto_4_40 and Proto_8_80), Q31. They are the low-pass prototype with
   the sign alternated every 2 * subbands taps, as the polyphase analysis
   requires; the tables are therefore not symmetric. */
static const int32_t sbc_proto4[40] = {
    0, 1152230, 3203796, 5870595, 8240328, 8358117, 4006811, -6571564,
    23437125, 43891363, 62010178, 69135936, 55569964, 13169340, -61894188, -166744266,
    291184339, 418733200, 529648199, 605221457, 632037363, 605221457, 529648199, 418733200,
    -291184339, -166744266, -61894188, 13169340, 55569964, 69135936, 62010178, 43891363,
    -23437125, -6571564, 4006811, 8358117, 8240328, 5870595, 3203796, 1152230,
};
static const int32_t sbc_proto8[80] = {
    0, 336243, 737138, 1191038, 1769354, 2447970, 3170548, 3830504,
    4320362, 4517704, 4283254, 3471542, 1937362, -383982, -3542770, -7510125,
    12153672, 17243030, 22459338, 27374475, 31466061, 34154783, 34834004, 32896036,
    27782384, 19021498, 6279423, -10556558, -31440036, -56070530, -83913220, -114218864,
    146026618, 178208410, 209541558, 238793071, 264708601, 286183152, 302265850, 312222319,
    315583606, 312222319, 302265850, 286183152, 264708601, 238793071, 209541558, 178208410,
    -146026618, -114218864, -83913220, -56070530, -31440036, -10556558, 6279423, 19021498,
    27782384, 32896036, 34834004, 34154783, 31466061, 27374475, 22459338, 17243030,
    -12153672, -7510125, -3542770, -383982, 1937362, 3471542, 4283254, 4517704,
    4320362, 3830504, 3170548, 2447970, 1769354, 1191038, 737138, 336243,
};

/* Analysis matrices M[i][k] = cos((i + 0.5) * (k - S/2) * pi / S), Q30. */
static const int32_t sbc_cos4[32] = {
    759250125, 992008094, 1073741824, 992008094, 759250125, 410903207, 0, -410903207,
    -759250125, 410903207, 1073741824, 410903207, -759250125, -992008094, 0, 992008094,
    -759250125, -410903207, 1073741824, -410903207, -759250125, 992008094, 0, -992008094,
    759250125, -992008094, 1073741824, -992008094, 759250125, -410903207, 0, 410903207,
};
static const int32_t sbc_cos8[128] = {
    759250125, 892783698, 992008094, 1053110176, 1073741824, 1053110176, 992008094, 892783698,
    759250125, 596538995, 410903207, 209476638, 0, -209476638, -410903207, -596538995,
    -759250125, -209476638, 410903207, 892783698, 1073741824, 892783698, 410903207, -209476638,
    -759250125, -1053110176, -992008094, -596538995, 0, 596538995, 992008094, 1053110176,
    -759250125, -1053110176, -410903207, 596538995, 1073741824, 596538995, -410903207, -1053110176,
    -759250125, 209476638, 992008094, 892783698, 0, -892783698, -992008094, -209476638,
    759250125, -596538995, -992008094, 209476638, 1073741824, 209476638, -992008094, -596538995,
    759250125, 892783698, -410903207, -1053110176, 0, 1053110176, 410903207, -892783698,
    759250125, 596538995, -992008094, -209476638, 1073741824, -209476638, -992008094, 596538995,
    759250125, -892783698, -410903207, 1053110176, 0, -1053110176, 410903207, 892783698,
    -759250125, 1053110176, -410903207, -596538995, 1073741824, -596538995, -410903207, 1053110176,
    -759250125, -209476638, 992008094, -892783698, 0, 892783698, -992008094, 209476638,
    -759250125, 209476638, 410903207, -892783698, 1073741824, -892783698, 410903207, 209476638,
    -759250125, 1053110176, -992008094, 596538995, 0, -596538995, 992008094, -1053110176,
    759250125, -892783698, 992008094, -1053110176, 1073741824, -1053110176, 992008094, -892783698,
    759250125, -596538995, 410903207, -209476638, 0, 209476638, -410903207, 596538995,
};

/* Loudness offsets per sampling frequency and subband (same appendix). */
static const int8_t sbc_offset4[4][4] = {
    { -1, 0, 0, 0 },
    { -2, 0, 0, 1 },
    { -2, 0, 0, 1 },
    { -2, 0, 0, 1 },
};
static const int8_t sbc_offset8[4][8] = {
    { -2, 0, 0, 0, 0, 0, 0, 1 },
    { -3, 0, 0, 0, 0, 0, 1, 2 },
    { -4, 0, 0, 0, 0, 0, 1, 2 },
    { -4, 0, 0, 0, 0, 0, 1, 2 },
};

static uint8_t sbc_channels(const struct bt_sbc_config *config)
{
    return (config->mode == BT_SBC_MODE_MONO) ? 1 : 2;
}

bool bt_sbc_config_valid(const struct bt_sbc_config *config)
{
    unsigned max_bitpool;

    if(config->frequency > BT_SBC_FREQ_48000 || config->mode > BT_SBC_MODE_JOINT_STEREO ||
       config->allocation > BT_SBC_ALLOC_SNR) {
        return false;
    }
    if(config->blocks != 4 && config->blocks != 8 && config->blocks != 12 && config->blocks != 16) {
        return false;
    }
    if(config->subbands != 4 && config->subbands != 8) {
        return false;
    }
    max_bitpool = (config->mode == BT_SBC_MODE_MONO || config->mode == BT_SBC_MODE_DUAL_CHANNEL) ?
                  16u * config->subbands : 32u * config->subbands;
    if(max_bitpool > 250) {
        max_bitpool = 250;
    }
    return config->bitpool >= 2 && config->bitpool <= max_bitpool;
}

size_t bt_sbc_frame_length(const struct bt_sbc_config *config)
{
    size_t nsb, nch, bits;

    if(!bt_sbc_config_valid(config)) {
        return 0;
    }
    nsb = config->subbands;
    nch = sbc_channels(config);
    switch(config->mode) {
    case BT_SBC_MODE_MONO:
    case BT_SBC_MODE_DUAL_CHANNEL:
        bits = (size_t)config->blocks * nch * config->bitpool;
        break;
    case BT_SBC_MODE_JOINT_STEREO:
        bits = nsb + (size_t)config->blocks * config->bitpool;
        break;
    default:
        bits = (size_t)config->blocks * config->bitpool;
        break;
    }
    return 4 + (4 * nsb * nch) / 8 + (bits + 7) / 8;
}

size_t bt_sbc_codesize(const struct bt_sbc_config *config)
{
    if(!bt_sbc_config_valid(config)) {
        return 0;
    }
    return (size_t)config->blocks * config->subbands * sbc_channels(config) * sizeof(int16_t);
}

uint32_t bt_sbc_bitrate(const struct bt_sbc_config *config)
{
    static const uint32_t rates[4] = { 16000, 32000, 44100, 48000 };
    size_t len = bt_sbc_frame_length(config);

    if(len == 0) {
        return 0;
    }
    return (uint32_t)((8u * len * rates[config->frequency]) /
                      ((uint32_t)config->subbands * config->blocks));
}

bool bt_sbc_encoder_init(struct bt_sbc_encoder *encoder, const struct bt_sbc_config *config)
{
    if(!bt_sbc_config_valid(config)) {
        return false;
    }
    memset(encoder, 0, sizeof(*encoder));
    encoder->config = *config;
    encoder->channels = sbc_channels(config);
    return true;
}

uint8_t bt_sbc_crc8(const uint8_t *data, size_t bit_count)
{
    uint8_t crc = 0x0F;
    size_t i;

    for(i = 0; i < bit_count; i++) {
        unsigned bit = (data[i >> 3] >> (7 - (i & 7))) & 1u;
        unsigned feedback = ((crc >> 7) & 1u) ^ bit;

        crc = (uint8_t)(crc << 1);
        if(feedback) {
            crc ^= 0x1D;
        }
    }
    return crc;
}

static int32_t sbc_clamp32(int64_t v)
{
    if(v > INT32_MAX) {
        return INT32_MAX;
    }
    if(v < -INT32_MAX) {
        return -INT32_MAX;
    }
    return (int32_t)v;
}

/*
 * One analysis step for one channel: takes nsb new samples (every stride-th
 * value from pcm) and produces nsb subband samples in Q15.
 */
static void sbc_analyze(int32_t *x, const int16_t *pcm, unsigned stride, unsigned nsb, int32_t *out)
{
    const int32_t *proto = (nsb == 8) ? sbc_proto8 : sbc_proto4;
    const int32_t *cosm = (nsb == 8) ? sbc_cos8 : sbc_cos4;
    unsigned taps = 10 * nsb;
    unsigned i, j, k;
    int32_t y[16];

    memmove(&x[nsb], &x[0], (taps - nsb) * sizeof(x[0]));
    for(i = 0; i < nsb; i++) {
        /* The first new sample lands in X[nsb - 1], the last in X[0]. */
        x[nsb - 1 - i] = pcm[i * stride];
    }
    for(k = 0; k < 2 * nsb; k++) {
        int64_t acc = 0;

        for(j = 0; j < 5; j++) {
            acc += (int64_t)proto[k + j * 2 * nsb] * x[k + j * 2 * nsb];
        }
        y[k] = (int32_t)(acc >> 16); /* Q31 * sample -> Q15 */
    }
    for(i = 0; i < nsb; i++) {
        int64_t acc = 0;

        for(k = 0; k < 2 * nsb; k++) {
            acc += (int64_t)cosm[i * 2 * nsb + k] * y[k];
        }
        out[i] = sbc_clamp32(acc >> 30);
    }
}

static uint8_t sbc_scale_factor(const int32_t *s, unsigned blocks, unsigned stride)
{
    uint32_t peak = 0;
    uint8_t sf = 0;
    unsigned b;

    for(b = 0; b < blocks; b++) {
        int32_t v = s[b * stride];
        uint32_t a = (v < 0) ? (uint32_t)(-(int64_t)v) : (uint32_t)v;

        if(a > peak) {
            peak = a;
        }
    }
    /* Smallest sf with |s| < 2^(sf + 1) in sample units, i.e. 2^(sf + 16)
       in Q15. */
    while(sf < 15 && peak >= (UINT32_C(1) << (sf + 16))) {
        sf++;
    }
    return sf;
}

/*
 * Bit allocation (A2DP specification, SBC appendix). For stereo and joint
 * stereo both channels share one bitpool; for mono and dual channel each
 * channel is allocated on its own, so call this once per channel with
 * nch == 1.
 */
static void sbc_allocate(const struct bt_sbc_config *cfg, unsigned nch,
                         const uint8_t sf[2][8], uint8_t bits[2][8])
{
    unsigned nsb = cfg->subbands;
    int bitneed[2][8];
    int max_bitneed = 0, bitcount = 0, slicecount = 0, bitslice;
    unsigned ch, sb;

    for(ch = 0; ch < nch; ch++) {
        for(sb = 0; sb < nsb; sb++) {
            int need;

            if(cfg->allocation == BT_SBC_ALLOC_SNR) {
                need = sf[ch][sb];
            } else if(sf[ch][sb] == 0) {
                need = -5;
            } else {
                int loudness = sf[ch][sb] - ((nsb == 4) ? sbc_offset4[cfg->frequency][sb] :
                                                          sbc_offset8[cfg->frequency][sb]);
                need = (loudness > 0) ? loudness / 2 : loudness;
            }
            bitneed[ch][sb] = need;
            if(need > max_bitneed) {
                max_bitneed = need;
            }
        }
    }

    bitslice = max_bitneed + 1;
    do {
        bitslice--;
        bitcount += slicecount;
        slicecount = 0;
        for(ch = 0; ch < nch; ch++) {
            for(sb = 0; sb < nsb; sb++) {
                if(bitneed[ch][sb] > bitslice + 1 && bitneed[ch][sb] < bitslice + 16) {
                    slicecount++;
                } else if(bitneed[ch][sb] == bitslice + 1) {
                    slicecount += 2;
                }
            }
        }
    } while(bitcount + slicecount < cfg->bitpool);
    if(bitcount + slicecount == cfg->bitpool) {
        bitcount += slicecount;
        bitslice--;
    }

    for(ch = 0; ch < nch; ch++) {
        for(sb = 0; sb < nsb; sb++) {
            if(bitneed[ch][sb] < bitslice + 2) {
                bits[ch][sb] = 0;
            } else {
                int b = bitneed[ch][sb] - bitslice;
                bits[ch][sb] = (uint8_t)((b < 16) ? b : 16);
            }
        }
    }

    /* Hand out what is left, alternating channels within each subband. */
    ch = 0;
    sb = 0;
    while(bitcount < cfg->bitpool && sb < nsb) {
        if(bits[ch][sb] >= 2 && bits[ch][sb] < 16) {
            bits[ch][sb]++;
            bitcount++;
        } else if(bitneed[ch][sb] == bitslice + 1 && cfg->bitpool > bitcount + 1) {
            bits[ch][sb] = 2;
            bitcount += 2;
        }
        if(nch == 2 && ch == 0) {
            ch = 1;
        } else {
            ch = 0;
            sb++;
        }
    }
    ch = 0;
    sb = 0;
    while(bitcount < cfg->bitpool && sb < nsb) {
        if(bits[ch][sb] < 16) {
            bits[ch][sb]++;
            bitcount++;
        }
        if(nch == 2 && ch == 0) {
            ch = 1;
        } else {
            ch = 0;
            sb++;
        }
    }
}

struct sbc_bit_writer
{
    uint8_t *data;
    size_t pos; /* in bits */
};

static void sbc_put_bits(struct sbc_bit_writer *w, uint32_t value, unsigned count)
{
    while(count > 0) {
        unsigned bit;

        count--;
        bit = (value >> count) & 1u;
        if(bit) {
            w->data[w->pos >> 3] |= (uint8_t)(0x80u >> (w->pos & 7));
        }
        w->pos++;
    }
}

size_t bt_sbc_encode(struct bt_sbc_encoder *encoder, const int16_t *pcm,
                     uint8_t *out, size_t out_len)
{
    const struct bt_sbc_config *cfg = &encoder->config;
    unsigned nsb = cfg->subbands, nch = encoder->channels, blocks = cfg->blocks;
    size_t len = bt_sbc_frame_length(cfg);
    int32_t sb_sample[16][2][8];
    uint8_t sf[2][8], bits[2][8];
    uint8_t join = 0; /* bit (nsb - 1 - sb) set: subband sb is mid/side */
    struct sbc_bit_writer w;
    uint8_t crc_buf[12];
    size_t crc_bits;
    unsigned blk, ch, sb;

    if(len == 0 || out_len < len) {
        return 0;
    }

    for(blk = 0; blk < blocks; blk++) {
        for(ch = 0; ch < nch; ch++) {
            sbc_analyze(encoder->history[ch], &pcm[blk * nsb * nch + ch], nch, nsb,
                        sb_sample[blk][ch]);
        }
    }

    for(ch = 0; ch < nch; ch++) {
        for(sb = 0; sb < nsb; sb++) {
            sf[ch][sb] = sbc_scale_factor(&sb_sample[0][ch][sb], blocks, 2 * 8);
        }
    }

    if(cfg->mode == BT_SBC_MODE_JOINT_STEREO) {
        /* Code a subband as mid/side when that needs smaller scale factors.
           The last subband is never joined. */
        for(sb = 0; sb + 1 < nsb; sb++) {
            int32_t mid[16] = {0}, side[16] = {0};
            uint8_t sf_mid, sf_side;

            for(blk = 0; blk < blocks; blk++) {
                int64_t l = sb_sample[blk][0][sb], r = sb_sample[blk][1][sb];

                mid[blk] = (int32_t)((l + r) / 2);
                side[blk] = (int32_t)((l - r) / 2);
            }
            sf_mid = sbc_scale_factor(mid, blocks, 1);
            sf_side = sbc_scale_factor(side, blocks, 1);
            if((unsigned)sf_mid + sf_side < (unsigned)sf[0][sb] + sf[1][sb]) {
                join |= (uint8_t)(1u << (nsb - 1 - sb));
                sf[0][sb] = sf_mid;
                sf[1][sb] = sf_side;
                for(blk = 0; blk < blocks; blk++) {
                    sb_sample[blk][0][sb] = mid[blk];
                    sb_sample[blk][1][sb] = side[blk];
                }
            }
        }
    }

    memset(bits, 0, sizeof(bits));
    if(cfg->mode == BT_SBC_MODE_STEREO || cfg->mode == BT_SBC_MODE_JOINT_STEREO) {
        sbc_allocate(cfg, 2, sf, bits);
    } else {
        for(ch = 0; ch < nch; ch++) {
            uint8_t sf1[2][8], bits1[2][8];

            memcpy(sf1[0], sf[ch], sizeof(sf1[0]));
            sbc_allocate(cfg, 1, (const uint8_t (*)[8])sf1, bits1);
            memcpy(bits[ch], bits1[0], sizeof(bits[ch]));
        }
    }

    memset(out, 0, len);
    w.data = out;
    w.pos = 0;
    sbc_put_bits(&w, SBC_SYNCWORD, 8);
    sbc_put_bits(&w, cfg->frequency, 2);
    sbc_put_bits(&w, (uint32_t)(blocks / 4 - 1), 2);
    sbc_put_bits(&w, cfg->mode, 2);
    sbc_put_bits(&w, cfg->allocation, 1);
    sbc_put_bits(&w, (nsb == 8) ? 1u : 0u, 1);
    sbc_put_bits(&w, cfg->bitpool, 8);
    sbc_put_bits(&w, 0, 8); /* CRC, filled in below */
    if(cfg->mode == BT_SBC_MODE_JOINT_STEREO) {
        sbc_put_bits(&w, join, nsb);
    }
    for(ch = 0; ch < nch; ch++) {
        for(sb = 0; sb < nsb; sb++) {
            sbc_put_bits(&w, sf[ch][sb], 4);
        }
    }

    /* The CRC covers the two header octets after the syncword, then the
       join bits and scale factors, but not the CRC octet itself. */
    memset(crc_buf, 0, sizeof(crc_buf));
    crc_buf[0] = out[1];
    crc_buf[1] = out[2];
    crc_bits = w.pos - 32;
    {
        struct sbc_bit_writer cw = { crc_buf, 16 };
        size_t i;

        for(i = 0; i < crc_bits; i++) {
            size_t p = 32 + i;

            sbc_put_bits(&cw, (out[p >> 3] >> (7 - (p & 7))) & 1u, 1);
        }
        out[3] = bt_sbc_crc8(crc_buf, 16 + crc_bits);
    }

    for(blk = 0; blk < blocks; blk++) {
        for(ch = 0; ch < nch; ch++) {
            for(sb = 0; sb < nsb; sb++) {
                unsigned b = bits[ch][sb];
                int64_t scale, q;
                uint32_t levels;

                if(b == 0) {
                    continue;
                }
                levels = (UINT32_C(1) << b) - 1;
                scale = INT64_C(1) << (sf[ch][sb] + 16);
                q = (((int64_t)sb_sample[blk][ch][sb] + scale) * levels) >> (sf[ch][sb] + 17);
                if(q < 0) {
                    q = 0;
                } else if(q > (int64_t)levels) {
                    q = levels;
                }
                sbc_put_bits(&w, (uint32_t)q, b);
            }
        }
    }
    return len;
}
