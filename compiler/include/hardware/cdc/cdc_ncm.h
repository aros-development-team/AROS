#ifndef HARDWARE_CDC_NCM_H
#define HARDWARE_CDC_NCM_H

#include <exec/types.h>

#if defined(__GNUC__)
# pragma pack(1)
#endif

#define CDC_NCM_FUNC_DESC_SUBTYPE    0x1a

#define CDC_NCM_NTH16_SIGNATURE      0x484d434eUL /* "NCMH" */
#define CDC_NCM_NDP16_SIGNATURE_NCM0 0x304d434eUL /* "NCM0" */
#define CDC_NCM_NDP16_SIGNATURE_NCM1 0x314d434eUL /* "NCM1" */

struct UsbCDCNcmFunctionalDesc
{
    UBYTE bFunctionLength;
    UBYTE bDescriptorType;
    UBYTE bDescriptorSubtype;
    UWORD bcdNcmVersion;
    UWORD wMaxSegmentSize;
    UWORD wNumberMCFilters;
    UBYTE bNumberPowerFilters;
};

struct UsbCDCNcmNtb16Header
{
    ULONG dwSignature;
    UWORD wHeaderLength;
    UWORD wSequence;
    UWORD wBlockLength;
    UWORD wNdpIndex;
};

struct UsbCDCNcmNdp16
{
    ULONG dwSignature;
    UWORD wLength;
    UWORD wNextNdpIndex;
};

struct UsbCDCNcmDatagramPointer16
{
    UWORD wDatagramIndex;
    UWORD wDatagramLength;
};

#if defined(__GNUC__)
# pragma pack()
#endif

#endif
