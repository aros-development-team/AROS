#ifndef ZUNE_QRCODE_H
#define ZUNE_QRCODE_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <libraries/mui.h>

#define MUIC_QRCode "QRCode.mcc"

/* The class-local attribute base is subject to upstream namespace review. */
#define MUIB_QRCode (MUIB_AROS | 0x00000B00)

/* [IS.] CONST_STRPTR; UTF-8 NUL-terminated, copied on set. */
#define MUIA_QRCode_Text   (MUIB_QRCode | 0x01)
/* [IS.] const struct MUI_QRCodePayload *; copied on set. */
#define MUIA_QRCode_Data   (MUIB_QRCode | 0x02)
/* [ISG] ULONG; exact encoded level, one of MUIV_QRCode_ECC_*. */
#define MUIA_QRCode_ECC    (MUIB_QRCode | 0x03)
/* [..G] ULONG; one of MUIV_QRCode_Status_*. */
#define MUIA_QRCode_Status (MUIB_QRCode | 0x04)

#define MUIV_QRCode_ECC_L 0
#define MUIV_QRCode_ECC_M 1
#define MUIV_QRCode_ECC_Q 2
#define MUIV_QRCode_ECC_H 3

#define MUIV_QRCode_Status_Empty      0
#define MUIV_QRCode_Status_Ready      1
#define MUIV_QRCode_Status_Invalid    2
#define MUIV_QRCode_Status_NoMemory   3
#define MUIV_QRCode_Status_NoCapacity 4

/* The payload pointer is borrowed only for the duration of OM_NEW/OM_SET. */
struct MUI_QRCodePayload
{
    const UBYTE *Bytes;
    ULONG Length;
};

#define QRCodeObject MUIOBJMACRO_START(MUIC_QRCode)

#endif /* ZUNE_QRCODE_H */
