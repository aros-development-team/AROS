#ifndef BLUETOOTH_RFCOMM_H
#define BLUETOOTH_RFCOMM_H
/*
**	$VER: rfcomm.h 1.0 (09.07.06)
**
**	Bluetooth RFCOMM with TS07.10 Protocol (RFCOMM) definitions include file
**
**	(C) Copyright 2006 Chris Hodges
**	    All Rights Reserved
*/

#if defined(__GNUC__)
# pragma pack(1)
#endif

#define RFCOMMF_EA   0x01
#define RFCOMMF_CR   0x02
#define RFCOMMM_DLCI 0x7c
#define RFCOMMS_DLCI 2
#define RFCOMMM_LEN  0xfe
#define RFCOMMS_LEN  1

struct RFFrame
{
    //UBYTE rff_OpenFlag; // not used with RFCOMM
    UBYTE rff_Address;
    UBYTE rff_Control;
    UBYTE rff_Data[0];
    UBYTE rff_FCS;
    //UBYTE rff_CloseFlag;
};

#define RFCOMMF_CTRL_PF    0x10
#define RFCOMMF_CTRL_SABN  0x2f // Set Asynchronous Balanced Mode
#define RFCOMMF_CTRL_UA    0x63 // Unnumbered Acknoledgement
#define RFCOMMF_CTRL_DM    0x0f // Disconnect Mode
#define RFCOMMF_CTRL_DISC  0x43 // Disconnect
#define RFCOMMF_CTRL_UIH   0xef // Unnumbered Information with Header Check
#define RFCOMMF_CTRL_UI    0x02 // Unnumbered Information, not supported by RFCOMM

struct RFCOMMMsg
{
    UBYTE rfm_Type;
    UBYTE rfm_Length; // variable size (RFCAF_EA set indicates last byte)
    UBYTE rfm_Data[0];
};

#define RFCOMM_CMD_PN    0x20 // Parameter Negotiation, 8 bytes
#define RFCOMM_CMD_PSC   0x10 // Power Saving Control, not used in RFCOMM
#define RFCOMM_CMD_CLD   0x30 // Multiplexer close down, not used in RFCOMM
#define RFCOMM_CMD_TEST  0x08 // Test (echo)
#define RFCOMM_CMD_FCON  0x28 // Flow Control on
#define RFCOMM_CMD_FCOFF 0x18 // Flow Control off
#define RFCOMM_CMD_MSC   0x38 // Modem Status Command, 2-3 bytes
#define RFCOMM_CMD_NSC   0x04 // Not Supported Command, 1 byte
#define RFCOMM_CMD_RPN   0x24 // Remote Port Negotiation Command, 1 or 8 bytes
#define RFCOMM_CMD_RLS   0x14 // Remote Line Status, 1 byte
#define RFCOMM_CMD_SNC   0x34 // Service Negotiation Command, not used in RFCOMM

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* BLUETOOTH_RFCOMM_H */
