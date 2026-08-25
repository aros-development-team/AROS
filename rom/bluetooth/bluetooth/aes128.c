/*
 *----------------------------------------------------------------------------
 *        bluetooth.library: AES-128 block encryption (FIPS-197)
 *----------------------------------------------------------------------------
 * Encryption only (that is all SMP needs). The S-box is generated once from
 * the GF(2^8) inverse + affine transform instead of being typed in as a
 * table, so there is nothing to mis-transcribe.
 */

#include "aes128.h"
#include <string.h>

static UBYTE aes_sbox[256];
static BOOL  aes_sbox_ready = FALSE;

#define ROTL8(x, s) ((UBYTE) (((x) << (s)) | ((x) >> (8 - (s)))))

static void bAESInitSBox(void)
{
    UBYTE p = 1, q = 1;
    /* walk the multiplicative group with generator 3: p = p*3, q = q/3 */
    do {
        p = p ^ (UBYTE) (p << 1) ^ ((p & 0x80) ? 0x1b : 0x00);
        q ^= (UBYTE) (q << 1);
        q ^= (UBYTE) (q << 2);
        q ^= (UBYTE) (q << 4);
        if(q & 0x80) q ^= 0x09;
        aes_sbox[p] = (UBYTE) (q ^ ROTL8(q, 1) ^ ROTL8(q, 2) ^ ROTL8(q, 3) ^ ROTL8(q, 4) ^ 0x63);
    } while(p != 1);
    aes_sbox[0] = 0x63;
    aes_sbox_ready = TRUE;
}

static UBYTE xtime(UBYTE x)
{
    return (UBYTE) ((x << 1) ^ ((x & 0x80) ? 0x1b : 0x00));
}

void bAES128Encrypt(const UBYTE key[16], const UBYTE in[16], UBYTE out[16])
{
    static const UBYTE rcon[10] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 };
    UBYTE rk[176];
    UBYTE s[16];
    ULONG i, r, c;

    if(!aes_sbox_ready) {
        bAESInitSBox();
    }

    /* key expansion */
    memcpy(rk, key, 16);
    for(i = 16; i < 176; i += 4) {
        UBYTE t0 = rk[i - 4], t1 = rk[i - 3], t2 = rk[i - 2], t3 = rk[i - 1];
        if((i % 16) == 0) {
            UBYTE tmp = t0;
            t0 = (UBYTE) (aes_sbox[t1] ^ rcon[i / 16 - 1]);
            t1 = aes_sbox[t2];
            t2 = aes_sbox[t3];
            t3 = aes_sbox[tmp];
        }
        rk[i]     = (UBYTE) (rk[i - 16] ^ t0);
        rk[i + 1] = (UBYTE) (rk[i - 15] ^ t1);
        rk[i + 2] = (UBYTE) (rk[i - 14] ^ t2);
        rk[i + 3] = (UBYTE) (rk[i - 13] ^ t3);
    }

    /* state is column-major: s[c*4 + r] */
    for(i = 0; i < 16; i++) {
        s[i] = (UBYTE) (in[i] ^ rk[i]);
    }
    for(r = 1; r <= 10; r++) {
        UBYTE t[16];
        /* SubBytes + ShiftRows: row j of column c comes from column (c + j) mod 4 */
        for(c = 0; c < 4; c++) {
            t[c * 4 + 0] = aes_sbox[s[((c + 0) & 3) * 4 + 0]];
            t[c * 4 + 1] = aes_sbox[s[((c + 1) & 3) * 4 + 1]];
            t[c * 4 + 2] = aes_sbox[s[((c + 2) & 3) * 4 + 2]];
            t[c * 4 + 3] = aes_sbox[s[((c + 3) & 3) * 4 + 3]];
        }
        if(r != 10) {
            /* MixColumns */
            for(c = 0; c < 4; c++) {
                UBYTE a0 = t[c * 4], a1 = t[c * 4 + 1], a2 = t[c * 4 + 2], a3 = t[c * 4 + 3];
                UBYTE all = (UBYTE) (a0 ^ a1 ^ a2 ^ a3);
                t[c * 4 + 0] = (UBYTE) (a0 ^ all ^ xtime((UBYTE) (a0 ^ a1)));
                t[c * 4 + 1] = (UBYTE) (a1 ^ all ^ xtime((UBYTE) (a1 ^ a2)));
                t[c * 4 + 2] = (UBYTE) (a2 ^ all ^ xtime((UBYTE) (a2 ^ a3)));
                t[c * 4 + 3] = (UBYTE) (a3 ^ all ^ xtime((UBYTE) (a3 ^ a0)));
            }
        }
        /* AddRoundKey */
        for(i = 0; i < 16; i++) {
            s[i] = (UBYTE) (t[i] ^ rk[r * 16 + i]);
        }
    }
    memcpy(out, s, 16);
}
