#ifndef BLUETOOTH_L2CAP_H
#define BLUETOOTH_L2CAP_H
/*
**	$VER: l2cap.h 1.0 (23.04.06)
**
**	Bluetooth L2CAP definitions include file
**
**	(C) Copyright 2006 Chris Hodges
**	    All Rights Reserved
*/

#if defined(__GNUC__)
# pragma pack(1)
#endif

#define L2CAP_CID_NULL      0x0000
#define L2CAP_CID_SIGCHAN   0x0001
#define L2CAP_CID_CONNLESS  0x0002
#define L2CAP_CID_USERSTART 0x0040

struct BTL2CAPBasicFrame
{
    UWORD l2bf_Length;         /* Length field (payload, without header) */
    UWORD l2bf_ChannelID;      /* Channel ID (CID) for channel endpoint */
    UBYTE l2bf_Payload[0];     /* Data Payload */
};

struct BTL2CAPConnLessFrame
{
    UWORD l2cf_Length;         /* Length field (payload, without header) */
    UWORD l2cf_ChannelID;      /* Channel ID (CID) for channel endpoint */
    UWORD l2cf_PSM;            /* Protocol Service Multiplexer, minimum 2 bytes */
    UBYTE l2cf_Payload[0];     /* Data Payload */
};

struct BTL2CAPSuperFrame
{
    UWORD l2sf_Length;         /* Length field (payload, without header) */
    UWORD l2sf_ChannelID;      /* Channel ID (CID) for channel endpoint */
    UWORD l2sf_Control;        /* Control field */
    UWORD l2sf_FCS;            /* Frame Check Sequence */
};

struct BTL2CAPInfoFrame
{
    UWORD l2if_Length;         /* Length field (payload, without header) */
    UWORD l2if_ChannelID;      /* Channel ID (CID) for channel endpoint */
    UWORD l2if_Control;        /* Control field */
    UWORD l2if_SDULength;      /* optional SDU Length field */
    UBYTE l2if_Payload[0];     /* Payload */
    //UWORD l2if_FCS;          /* Frame Check Sequence at the end */
};

#define L2CF_SUPERFRAME 0x0001 /* This is a supervisor frame instead of informational frame */
#define L2CF_NORESEND   0x0080 /* Disable retransmitting of lost/corrupt packets */

#define L2CF_SAR_NOFRAG 0x0000 /* SDU is not segmented */
#define L2CF_SAR_START  0x4000 /* Start of SDU */
#define L2CF_SAR_END    0x8000 /* End of SDU */
#define L2CF_SAR_CONT   0xc000 /* Continuation of SDU */
#define L2CM_SAR        0xc000 /* SAR mask */

#define L2CM_REQSEQ     0x3f00 /* Receiver sequence number */
#define L2CS_REQSEQ     9
#define L2CM_TXSEQ      0x007e /* Transmission sequence number */
#define L2CS_TXSEQ      1

#define L2CF_SUPER_RR   0x0000 /* Supervisor: Receiver ready */
#define L2CF_SUPER_REJ  0x0004 /* Supervisor: Reject */
#define L2CM_SUPER      0x000c /* Supervisor mask */

struct BTL2CAPCommand
{
    UBYTE l2c_Code;            /* Code */
    UBYTE l2c_Identifier;      /* Identifier */
    UWORD l2c_DataLength;      /* Length of Command Data */
    UBYTE l2c_Data[0];         /* Command payload */
};

#define L2CMD_COMMAND_REJECT         0x01
#define L2CMD_CONNECTION_REQUEST     0x02
#define L2CMD_CONNECTION_RESPONSE    0x03
#define L2CMD_CONFIGURATION_REQUEST  0x04
#define L2CMD_CONFIGURATION_RESPONSE 0x05
#define L2CMD_DISCONNECTION_REQUEST  0x06
#define L2CMD_DISCONNECTION_RESPONSE 0x07
#define L2CMD_ECHO_REQUEST           0x08
#define L2CMD_ECHO_RESPONSE          0x09
#define L2CMD_INFORMATION_REQUEST    0x0a
#define L2CMD_INFORMATION_RESPONSE   0x0b

/* error/status codes for different responses */

// Command reject
#define L2ERR_COMMAND_NOT_UNDERSTOOD  0x0000 
#define L2ERR_SIGNALLING_MTU_EXCEEDED 0x0001
#define L2ERR_INVALID_CID_IN_REQUEST  0x0002

// Connection response
#define L2ERR_CONN_SUCCESSFUL         0x0000
#define L2ERR_CONN_PENDING            0x0001
#define L2ERR_CONN_REFUSED_BAD_PSM    0x0002
#define L2ERR_CONN_REFUSED_SECURITY   0x0003
#define L2ERR_CONN_REFUSED_UNAVAIL    0x0004
#define L2STS_NO_INFO                 0x0000
#define L2STS_AUTHENTICATION_PENDING  0x0001
#define L2STS_AUTHORIZATION_PENDING   0x0002

// Configuration response
#define L2ERR_CFG_SUCCESS             0x0000
#define L2ERR_CFG_FAIL_BAD_PARAMS     0x0001
#define L2ERR_CFG_FAIL_REJECTED       0x0002
#define L2ERR_CFG_FAIL_UNKNOWN_OPTS   0x0003

// Information response
#define L2ERR_INFO_SUCCESS            0x0000
#define L2ERR_INFO_NOT_SUPPORTED      0x0001

#define L2CAP_INFOTYPE_CONNLESS_MTU   0x0001
#define L2CAP_INFOTYPE_EXT_FEAT_MASK  0x0002

/* MTU sizes for L2CAP */
#define L2CAP_DEFAULT_MTU 672
#define L2CAP_MIN_MTU 48

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* BLUETOOTH_L2CAP_H */
