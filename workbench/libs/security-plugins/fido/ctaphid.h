#ifndef CTAPHID_H
#define CTAPHID_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: CTAPHID framing and the U2F raw message format, as fido.secplugin
          speaks them to a key through fido.device, and fidosim.device
          answers them. FIDO Alliance: "FIDO U2F HID Protocol" and "FIDO
          U2F Raw Message Formats".
*/

#define CTAPHID_REPORTSIZE          64
#define CTAPHID_BROADCAST_CID       0xffffffffUL
#define CTAPHID_INIT_DATA           (CTAPHID_REPORTSIZE - 7)    /* data bytes in an initial packet      */
#define CTAPHID_CONT_DATA           (CTAPHID_REPORTSIZE - 5)    /* ... and in a continuation packet     */
#define CTAPHID_MAXPAYLOAD          (CTAPHID_INIT_DATA + 128 * CTAPHID_CONT_DATA)

#define CTAPHID_TYPE_INIT           0x80
#define CTAPHID_MSG                 (CTAPHID_TYPE_INIT | 0x03)
#define CTAPHID_INIT                (CTAPHID_TYPE_INIT | 0x06)
#define CTAPHID_WINK                (CTAPHID_TYPE_INIT | 0x08)
#define CTAPHID_KEEPALIVE           (CTAPHID_TYPE_INIT | 0x3b)
#define CTAPHID_ERROR               (CTAPHID_TYPE_INIT | 0x3f)

#define CTAPHID_ERR_INVALID_CMD     0x01
#define CTAPHID_ERR_INVALID_LEN     0x03
#define CTAPHID_ERR_INVALID_SEQ     0x04
#define CTAPHID_ERR_CHANNEL_BUSY    0x06
#define CTAPHID_ERR_INVALID_CHANNEL 0x0b

#define CTAPHID_INIT_NONCELEN       8
#define CTAPHID_INIT_RESPLEN        17                          /* nonce, cid, version, 3 x version, capabilities */
#define CTAPHID_CAP_WINK            0x01

/* U2F raw messages: ISO 7816-4 APDUs with extended length */
#define U2F_REGISTER                0x01
#define U2F_AUTHENTICATE            0x02
#define U2F_VERSION                 0x03

#define U2F_AUTH_CHECK_ONLY         0x07
#define U2F_AUTH_ENFORCE            0x03
#define U2F_AUTH_DONT_ENFORCE       0x08

#define U2F_PARAMSIZE               32                          /* a challenge or application parameter */
#define U2F_PUBKEYSIZE              65                          /* X9.62 uncompressed P-256 point       */
#define U2F_MAXKHSIZE               255
#define U2F_REGISTER_ID             0x05
#define U2F_PRESENCE                0x01

#define U2F_SW_NO_ERROR             0x9000
#define U2F_SW_CONDITIONS_NOT_SATISFIED 0x6985
#define U2F_SW_WRONG_DATA           0x6a80
#define U2F_SW_WRONG_LENGTH         0x6700
#define U2F_SW_INS_NOT_SUPPORTED    0x6d00
#define U2F_SW_CLA_NOT_SUPPORTED    0x6e00

#endif /* CTAPHID_H */
