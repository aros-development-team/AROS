#ifndef BLUETOOTH_SDP_H
#define BLUETOOTH_SDP_H
/*
**	$VER: sdp.h 1.0 (15.05.06)
**
**	Bluetooth Service Discovery Protocol (SDP) definitions include file
**
**	(C) Copyright 2006 Chris Hodges
**	    All Rights Reserved
*/

#if defined(__GNUC__)
# pragma pack(1)
#endif

/* Data Element Type Descriptors */
#define SDPDET_MASK  0xf8
#define SDPDET_SHIFT    3

#define SDPDET_NIL      0 // Nil, the null type (0)
#define SDPDET_UINT     1 // Unsigned integer (0 - 4)
#define SDPDET_SINT     2 // Signed integer (0 - 4)
#define SDPDET_UUID     3 // UUID (1, 2, 4)
#define SDPDET_STRING   4 // Text string (5, 6, 7)
#define SDPDET_BOOLEAN  5 // Boolean (0)
#define SDPDET_DATASEQ  6 // Data element sequence (5, 6, 7)
#define SDPDET_DATAMUT  7 // Data element sequence, one mutually exclusive (5, 6, 7)
#define SDPDET_URL      8 // URL (5, 6, 7)

/* Data Element Size Descriptors */
#define SDPDES_MASK  0x07
#define SDPDES_SHIFT    0

#define SDPDES_1BYTE     0 // 1 Byte (except for NIL)
#define SDPDES_2BYTES    1 // 2 Bytes
#define SDPDES_4BYTES    2 // 4 Bytes
#define SDPDES_8BYTES    3 // 8 Bytes
#define SDPDES_16BYTES   4 // 16 Bytes
#define SDPDES_8BITSIZE  5 // BYTE with size follows
#define SDPDES_16BITSIZE 6 // WORD with size follows
#define SDPDES_32BITSIZE 7 // LONG with size follows

struct BTSDPPkt
{
    UBYTE sdpp_ID;             /* PDU ID */
    UBYTE sdpp_TransIDHigh;    /* Transaction ID HighByte */
    UBYTE sdpp_TransIDLow;     /* Transaction ID LowByte */
    UBYTE sdpp_ParamLenHigh;   /* Parameter Length HighByte */
    UBYTE sdpp_ParamLenLow;    /* Parameter Length LowByte */
    UBYTE sdpp_Param[0];       /* Parameters */
};

/* PDU IDs */
#define SDPPID_ERROR_RESPONSE            0x01
#define SDPPID_SEARCH_REQUEST            0x02
#define SDPPID_SEARCH_RESPONSE           0x03
#define SDPPID_ATTRIBUTE_REQUEST         0x04
#define SDPPID_ATTRIBUTE_RESPONSE        0x05
#define SDPPID_SEARCH_ATTRIBUTE_REQUEST  0x06
#define SDPPID_SEARCH_ATTRIBUTE_RESPONSE 0x07

/* Errors */

#define SDPERR_INVALID_SDP_VERSION           0x0001
#define SDPERR_INVALID_SERVICE_RECORD_HANDLE 0x0002
#define SDPERR_INVALID_REQUEST_SYNTAX        0x0003
#define SDPERR_INVALID_PDU_SIZE              0x0004
#define SDPERR_INVALID_CONTINUATION_STATE    0x0005
#define SDPERR_INSUFFICIENT_RESOURCES        0x0006

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* BLUETOOTH_SDP_H */
