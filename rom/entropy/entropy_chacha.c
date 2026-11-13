/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ChaCha20 "fast key erasure" CSPRNG core for entropy.resource.

    The resource keeps a 256-bit key that is run forward with the ChaCha20
    block function (D. J. Bernstein, RFC 8439).  Every generated block also
    overwrites the key with fresh keystream ("fast key erasure"), so output
    already handed out cannot be reconstructed from the current state, giving
    backtracking resistance.  Reseeding folds new entropy into the key by XOR
    and then diffuses it, so mixing in seed material can never lower the
    quality of the state.
*/

#include <exec/types.h>

#include "entropy_intern.h"

#define ROTL32(v, n)    (((v) << (n)) | ((v) >> (32 - (n))))

static inline ULONG LE32(const UBYTE *p)
{
    return (ULONG)p[0] | ((ULONG)p[1] << 8) |
           ((ULONG)p[2] << 16) | ((ULONG)p[3] << 24);
}

static inline void ST32(UBYTE *p, ULONG v)
{
    p[0] = (UBYTE)(v);
    p[1] = (UBYTE)(v >> 8);
    p[2] = (UBYTE)(v >> 16);
    p[3] = (UBYTE)(v >> 24);
}

#define QUARTERROUND(a, b, c, d)        \
    do {                                \
        a += b; d ^= a; d = ROTL32(d, 16); \
        c += d; b ^= c; b = ROTL32(b, 12); \
        a += b; d ^= a; d = ROTL32(d, 8);  \
        c += d; b ^= c; b = ROTL32(b, 7);  \
    } while (0)

/* Produce one 64-byte ChaCha20 keystream block from key/counter/nonce. */
static void chacha20_block(const UBYTE key[ENTROPY_KEYSZ], ULONG counter,
                           const UBYTE nonce[ENTROPY_NONCESZ], UBYTE out[64])
{
    static const ULONG sigma[4] =
        { 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574 };
    ULONG state[16], x[16];
    int i;

    state[0]  = sigma[0];
    state[1]  = sigma[1];
    state[2]  = sigma[2];
    state[3]  = sigma[3];
    for (i = 0; i < 8; i++)
        state[4 + i] = LE32(key + 4 * i);
    state[12] = counter;
    state[13] = LE32(nonce + 0);
    state[14] = LE32(nonce + 4);
    state[15] = LE32(nonce + 8);

    for (i = 0; i < 16; i++)
        x[i] = state[i];

    for (i = 0; i < 10; i++)            /* 10 double-rounds = 20 rounds */
    {
        QUARTERROUND(x[0], x[4], x[ 8], x[12]);
        QUARTERROUND(x[1], x[5], x[ 9], x[13]);
        QUARTERROUND(x[2], x[6], x[10], x[14]);
        QUARTERROUND(x[3], x[7], x[11], x[15]);
        QUARTERROUND(x[0], x[5], x[10], x[15]);
        QUARTERROUND(x[1], x[6], x[11], x[12]);
        QUARTERROUND(x[2], x[7], x[ 8], x[13]);
        QUARTERROUND(x[3], x[4], x[ 9], x[14]);
    }

    for (i = 0; i < 16; i++)
        ST32(out + 4 * i, x[i] + state[i]);

    Entropy_Wipe(x, sizeof(x));
    Entropy_Wipe(state, sizeof(state));
}

/* Advance the block counter, carrying into the nonce so a wrap can never
   reuse a (key, counter, nonce) triple. */
static void chacha20_advance(struct EntropyBase *EntropyBase)
{
    if (++EntropyBase->eb_Counter == 0)
    {
        int i;
        for (i = 0; i < ENTROPY_NONCESZ; i++)
            if (++EntropyBase->eb_Nonce[i] != 0)
                break;
    }
}

void Entropy_CRNG_Generate(struct EntropyBase *EntropyBase,
                           UBYTE *out, ULONG len)
{
    UBYTE block[64];

    while (len > 0)
    {
        ULONG take = (len < 32) ? len : 32;
        ULONG i;

        chacha20_block(EntropyBase->eb_Key, EntropyBase->eb_Counter,
                       EntropyBase->eb_Nonce, block);
        chacha20_advance(EntropyBase);

        /* First 32 bytes re-key (fast key erasure), the rest are output. */
        for (i = 0; i < take; i++)
            out[i] = block[32 + i];
        for (i = 0; i < ENTROPY_KEYSZ; i++)
            EntropyBase->eb_Key[i] = block[i];

        out += take;
        len -= take;
    }

    Entropy_Wipe(block, sizeof(block));
}

void Entropy_CRNG_Reseed(struct EntropyBase *EntropyBase,
                         const UBYTE *seed, ULONG len)
{
    UBYTE block[64];
    ULONG i;

    /* XOR-fold the seed into the key: this preserves whatever unpredictability
       the key already had while adding the new material. */
    for (i = 0; i < len; i++)
        EntropyBase->eb_Key[i % ENTROPY_KEYSZ] ^= seed[i];

    /* Diffuse and rekey so the folded-in bytes are spread across the whole
       state before any output is produced. */
    chacha20_block(EntropyBase->eb_Key, EntropyBase->eb_Counter,
                   EntropyBase->eb_Nonce, block);
    chacha20_advance(EntropyBase);
    for (i = 0; i < ENTROPY_KEYSZ; i++)
        EntropyBase->eb_Key[i] = block[i];

    Entropy_Wipe(block, sizeof(block));
}
