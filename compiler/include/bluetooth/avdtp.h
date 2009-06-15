#ifndef BLUETOOTH_AVDTP_H
#define BLUETOOTH_AVDTP_H
/*
**	$VER: avdtp.h 1.0 (22.05.06)
**
**	Bluetooth Audio/Video Distribution Protocol (AVDTP) definitions include file
**
**	(C) Copyright 2006 Chris Hodges
**	    All Rights Reserved
*/

#if defined(__GNUC__)
# pragma pack(1)
#endif

/* Commands */
#define AVDTP_DISCOVER          0x01
#define AVDTP_GET_CAPABILITIES  0x02
#define AVDTP_SET_CONFIGURATION 0x03
#define AVDTP_GET_CONFIGURATION 0x04
#define AVDTP_RECONFIGURE       0x05
#define AVDTP_OPEN              0x06
#define AVDTP_START             0x07
#define AVDTP_CLOSE             0x08
#define AVDTP_SUSPEND           0x09
#define AVDTP_ABORT             0x0a
#define AVDTP_SECURITY_CONTROL  0x0b

/* State machine */
#define AVDTP_STATE_IDLE        0x00
#define AVDTP_STATE_CONFIGURED  0x01
#define AVDTP_STATE_OPEN        0x02
#define AVDTP_STATE_STREAMING   0x03
#define AVDTP_STATE_CLOSING     0x04
#define AVDTP_STATE_ABORTING    0x05

struct BTAVDTPMediaPkt
{
    UBYTE avdtpm_Control;      /* Control field */
    UBYTE avdtpm_MarkerPT;     /* Marker and PayloadType */
    UWORD avdtpm_SeqNum;       /* Sequence number */
    ULONG avdtpm_TimeStamp;    /* Time Stamp */
    ULONG avdtpm_SSRC;         /* Random unique ID */
    ULONG avdtpm_CSRS[0];      /* Contributing sources */
};

/* Bits in control field */
#define AVDTP_CTRLM_VERSION     0xc0 /* Version */
#define AVDTP_CTRLS_VERSION     6

#define AVDTP_CTRLF_PADDING     0x20 /* Padding bytes */
#define AVDTP_CTRLF_EXTENSION   0x10 /* One extra header extension */
#define AVDTP_CTRLM_CSRC_CNT    0x0f /* Number of CSRCs to follow */
#define AVDTP_CTRLS_CSRC_CNT    0

/* Bits in MarkerPT field */
#define AVDTP_MPTF_MARKER       0x80 /* Marker */
#define AVDTP_MPTM_PAYLOAD_TYPE 0x7f /* Payload type */
#define AVDTP_MPTS_PAYLOAD_TYPE 0


struct BTAVDTPReportBlock
{
    ULONG avdtpb_SSRC;         /* SSRC of source */
    ULONG avdtpb_PacketsLost;  /* Packets lost (High byte: fraction lost) */
    ULONG avdtpb_HighSeqNum;   /* Extended Highest Sequence Number Received */
    ULONG avdtpb_Jitter;       /* Interval Jitter */
    ULONG avdtpb_LastSR;       /* Last SR */
    ULONG avdtpb_DelayLastSR;  /* Delay Since Last SR */
};

struct BTAVDTPSenderReportingPkt
{
    UBYTE avdtpr_VPRC;         /* Version/Padding/Reception Count field */
    UBYTE avdtpr_PT;           /* PacketType = 200 (RTCP_SR) */
    UWORD avdtpr_Length;       /* Length in ULONGs in RTCP minus one (incl header) */
    ULONG avdtpr_SenderSSRC;   /* SSRC of Sender */
    ULONG avdtpr_NTPStampHigh; /* NTP Time Stamp (MSW) */
    ULONG avdtpr_NTPStampLow;  /* NTP Time Stamp (LSW) */
    ULONG avdtpr_RTPStamp;     /* RTP Time Stamp */
    ULONG avdtpr_TXPacketCnt;  /* Sender's Packet Count */
    ULONG avdtpr_TXByteCnt;    /* Sender's Octet Count */
    struct BTAVDTPReportBlock avdtpr_Blocks[0]; /* Report blocks following */
};

struct BTAVDTPReceiverReportingPkt
{
    UBYTE avdtpr_VPRC;         /* Version/Padding/Reception Count field */
    UBYTE avdtpr_PT;           /* PacketType = 201 (RTCP_RR) */
    UWORD avdtpr_Length;       /* Length in ULONGs in RTCP minus one (incl header) */
    ULONG avdtpr_SenderSSRC;   /* SSRC of Sender */
    struct BTAVDTPReportBlock avdtpr_Blocks[0]; /* Report blocks following */
};

struct BTAVDTPSourceDescPkt
{
    UBYTE avdtpr_VPSC;         /* Version/Padding/Source Count field */
    UBYTE avdtpr_PT;           /* PacketType = 202 (RTCP_SDES) */
    UWORD avdtpr_Length;       /* Length in ULONGs in RTCP minus one (incl header) */
    ULONG avdtpr_Chunks[0];    /* Chunks */
};

struct BTAVDTPCmdPkt
{
    UBYTE avdtpc_Control;      /* Transaction label, packet type, message type */
    UBYTE avdtpc_SignalID;     /* RFA and Signal Idenitifer */
    UBYTE avdtpc_Param[0];     /* More data fields */
};

/* Control field bits */
#define AVDTP_CMDM_TRANSLABEL 0xf0
#define AVDTP_CMDS_TRANSLABEL 4

#define AVDTP_CMDM_FRAGINFO   0x0c
#define AVDTP_CMDF_SINGLE     0x00
#define AVDTP_CMDF_START      0x04
#define AVDTP_CMDF_CONTINUE   0x08
#define AVDTP_CMDF_END        0x0c

#define AVDTP_CMDM_CMDRSP     0x03
#define AVDTP_CMDF_COMMAND    0x00
#define AVDTP_CMDF_ACCEPT     0x02
#define AVDTP_CMDF_REJECT     0x03

#define AVDTP_CMDM_SIGNALID   0x3f
#define AVDTP_CMDS_SIGNALID   0

/* Service Category information elements */
#define AVDTP_SC_MEDIA_TRANSPORT    0x01
#define AVDTP_SC_REPORTING          0x02
#define AVDTP_SC_RECOVERY           0x03
#define AVDTP_SC_CONTENT_PROTECTION 0x04
#define AVDTP_SC_HEADER_COMPRESSION 0x05
#define AVDTP_SC_MULTIPLEXING       0x06
#define AVDTP_SC_MEDIA_CODEC        0x07


#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* BLUETOOTH_AVDTP_H */
