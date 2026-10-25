#ifndef ZUNE_QRCODE_PRIVATE_H
#define ZUNE_QRCODE_PRIVATE_H

#include <exec/types.h>

struct QRCode_DATA
{
    UBYTE *input;
    ULONG length;
    UBYTE *matrix;
    ULONG ecc;
    ULONG status;
    LONG darkPen;
    LONG lightPen;
    BOOL textInput;
};

#endif /* ZUNE_QRCODE_PRIVATE_H */
