/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Yubico OTP: modhex, the CRC and the token layout. Self-contained,
          shared by the keys plugin (which checks OTPs) and the tests
          (which make them).

          An OTP is the key's public id (0-16 bytes, modhex) followed by 32
          modhex characters: 16 bytes encrypted with the key's AES-128 key:

            0  private id (6)     a secret the key shares with the server
            6  use counter (2 LE) incremented at power-up; bit 15 is volatile
            8  timestamp (3)      8 Hz since power-up
           11  session counter    OTPs since power-up
           12  random (2)
           14  CRC-16 (2 LE)      over the 16 bytes; the residue is 0xF0B8

          Replay: (use counter << 8 | session counter) must grow.
*/
#ifndef YUBIOTP_H
#define YUBIOTP_H

#include <exec/types.h>
#include "aes128.h"

#define YUBIOTP_TOKENCHARS      32
#define YUBIOTP_MAXPUBIDCHARS   32
#define YUBIOTP_CRC_RESIDUE     0xf0b8

static const char yubiotp_modhex[16] = { 'c','b','d','e','f','g','h','i','j','k','l','n','r','t','u','v' };

static inline int yubiotp_modhex_value(char c)
{
    int i;

    for (i = 0; i < 16; i++)
        if (yubiotp_modhex[i] == c)
            return i;
    return -1;
}

static inline BOOL yubiotp_is_modhex(CONST_STRPTR s, ULONG n)
{
    ULONG i;

    for (i = 0; i < n; i++)
        if (yubiotp_modhex_value(s[i]) < 0)
            return FALSE;
    return TRUE;
}

/* 2n modhex characters -> n bytes; FALSE on a bad character */
static inline BOOL yubiotp_modhex_decode(CONST_STRPTR s, UBYTE *out, ULONG n)
{
    ULONG i;

    for (i = 0; i < n; i++)
    {
        int hi = yubiotp_modhex_value(s[2 * i]), lo = yubiotp_modhex_value(s[2 * i + 1]);

        if (hi < 0 || lo < 0)
            return FALSE;
        out[i] = (UBYTE)((hi << 4) | lo);
    }
    return TRUE;
}

static inline void yubiotp_modhex_encode(const UBYTE *in, ULONG n, STRPTR out)
{
    ULONG i;

    for (i = 0; i < n; i++)
    {
        out[2 * i] = yubiotp_modhex[in[i] >> 4];
        out[2 * i + 1] = yubiotp_modhex[in[i] & 15];
    }
    out[2 * n] = '\0';
}

/* CRC-16 (ISO 13239 / X.25), as Yubico uses it */
static inline UWORD yubiotp_crc16(const UBYTE *data, ULONG n)
{
    UWORD crc = 0xffff;
    ULONG i;
    int bit;

    for (i = 0; i < n; i++)
    {
        crc ^= data[i];
        for (bit = 0; bit < 8; bit++)
            crc = (crc & 1) ? (UWORD)((crc >> 1) ^ 0x8408) : (UWORD)(crc >> 1);
    }
    return crc;
}

/*
 * Check the 32 token characters of an OTP against the AES key and the
 * private id. On success the OTP's counter (use << 8 | session) is in
 * *counter. The 16 decrypted bytes are wiped before returning.
 */
static inline BOOL yubiotp_check(const UBYTE *aeskey, const UBYTE *privateid, CONST_STRPTR token, ULONG *counter)
{
    struct aes128_key k;
    UBYTE enc[16], dec[16];
    BOOL ok;
    int i;

    if (!yubiotp_modhex_decode(token, enc, 16))
        return FALSE;
    aes128_set_key(&k, aeskey);
    aes128_decrypt(&k, enc, dec);
    ok = (yubiotp_crc16(dec, 16) == YUBIOTP_CRC_RESIDUE);
    for (i = 0; i < 6; i++)
        if (dec[i] != privateid[i])
            ok = FALSE;
    if (ok)
        *counter = ((ULONG)((dec[6] | (dec[7] << 8)) & 0x7fff) << 8) | dec[11];
    for (i = 0; i < 16; i++)
        dec[i] = 0;
    for (i = 0; i < (int)sizeof(k.rk); i++)
        k.rk[i] = 0;
    return ok;
}

/*
 * Make the 32 token characters of an OTP (for tests and tools): the
 * counter (use << 8 | session), a timestamp and random bytes of the
 * caller's choice.
 */
static inline void yubiotp_make(const UBYTE *aeskey, const UBYTE *privateid, ULONG counter, ULONG timestamp, UWORD rnd, STRPTR token)
{
    struct aes128_key k;
    UBYTE dec[16], enc[16];
    UWORD crc;
    int i;

    for (i = 0; i < 6; i++)
        dec[i] = privateid[i];
    dec[6] = (UBYTE)((counter >> 8) & 0xff);
    dec[7] = (UBYTE)((counter >> 16) & 0x7f);
    dec[8] = (UBYTE)(timestamp & 0xff);
    dec[9] = (UBYTE)((timestamp >> 8) & 0xff);
    dec[10] = (UBYTE)((timestamp >> 16) & 0xff);
    dec[11] = (UBYTE)(counter & 0xff);
    dec[12] = (UBYTE)(rnd & 0xff);
    dec[13] = (UBYTE)(rnd >> 8);
    /* the CRC of the 14 bytes, inverted and stored LE, makes the residue */
    crc = (UWORD)~yubiotp_crc16(dec, 14);
    dec[14] = (UBYTE)(crc & 0xff);
    dec[15] = (UBYTE)(crc >> 8);
    aes128_set_key(&k, aeskey);
    aes128_encrypt(&k, dec, enc);
    yubiotp_modhex_encode(enc, 16, token);
}

#endif /* YUBIOTP_H */
