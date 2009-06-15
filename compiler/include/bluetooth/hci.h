#ifndef BLUETOOTH_HCI_H
#define BLUETOOTH_HCI_H
/*
**	$VER: hci.h 1.4 (16.08.02)
**
**	Bluetooth HCI definitions include file
**
**	(C) Copyright 2005 Chris Hodges
**	    All Rights Reserved
*/

#if defined(__GNUC__)
# pragma pack(1)
#endif

/* BT HCI Command */
struct BTHCICommand
{
    UWORD bhc_CmdOpcode;       /* Opcode (OGF+OCF) little endian */
    UBYTE bhc_ParamLength;     /* length of parameters to follow */
    UBYTE bhc_Param[255];      /* actual parameters */
};

/* BT HCI Event */
struct BTHCIEvent
{
    UBYTE bhe_EventType;       /* event type according to BT specs */
    UBYTE bhe_PayloadLength;   /* length of the event payload in bytes */
    UBYTE bhe_Payload[255];    /* actual payload */
};

/* BT ACL Data Packet */
struct BTHCIACLData
{
    UWORD bha_ConnHandle;      /* Connection Handle and Packet Boundary / Broadcast Flag (LE) */
    UWORD bha_DataLength;      /* length of the data frame (LE) */
    UBYTE bha_Data[0];         /* The data itself */
};

#define ACLHF_PB_MASK   0x3000 /* Mask for both flags */
#define ACLHF_PB_CONT   0x1000 /* Continuing fragment packet of higher layer message */
#define ACLHF_PB_FIRST  0x2000 /* first packet of higher layer message */

#define ACLHF_BF_MASK   0xc000 /* Mask for both flags */
#define ACLHF_BF_NONE   0x0000 /* No Broadcast */
#define ACLHF_BF_ASB    0x4000 /* Active Slave Broadcast */
#define ACLHF_BF_PSB    0x8000 /* Parked Slave Broadcast */

/* BT HCI Command Completion Event */
struct HCICommandCompleteEvent
{
    UBYTE cce_NumHCICmdPkt;    /* number of HCI command packets allowed to be sent */
    UBYTE cce_CmdOpcodeLB;     /* command opcode caused this event (LowByte) */
    UBYTE cce_CmdOpcodeHB;     /* command opcode caused this event (HighByte) */
    UBYTE cce_Param[252];      /* return parameters */
};

/* BT HCI Command Status Event */
struct HCICommandStatusEvent
{
    UBYTE cse_Status;          /* error code */
    UBYTE cse_NumHCICmdPkt;    /* number of HCI command packets allowed to be sent */
    UBYTE cse_CmdOpcodeLB;     /* command opcode caused this event (LowByte) */
    UBYTE cse_CmdOpcodeHB;     /* command opcode caused this event (HighByte) */
};

/* /// "HCI Commands, Events and Errors" */

/* Commands and Events */

#define OGF_LCC 0x01 // Link Control Commands
#define OGF_LPC 0x02 // Link Policy Commands
#define OGF_CBC 0x03 // Controller & Baseband Commands
#define OGF_INP 0x04 // Informational Parameters
#define OGF_STP 0x05 // Status Parameters
#define OGF_TST 0x06 // Testing Commands

#define OGFOCF(x,y) ((x)<<10|y)

/* HCI Link Control Commands */
#define HCICMD_INQUIRY                     OGFOCF(OGF_LCC, 0x01)
#define HCICMD_INQUIRY_CANCEL              OGFOCF(OGF_LCC, 0x02)
#define HCICMD_PERIODIC_INQUIRY_MODE       OGFOCF(OGF_LCC, 0x03)
#define HCICMD_EXIT_PERIODIC_INQUIRY_MODE  OGFOCF(OGF_LCC, 0x04)
#define HCICMD_CREATE_CONN                 OGFOCF(OGF_LCC, 0x05)
#define HCICMD_DISCONNECT                  OGFOCF(OGF_LCC, 0x06)
#define HCICMD_ADD_SCO_CONN                OGFOCF(OGF_LCC, 0x07) // deprecated!
#define HCICMD_CREATE_CONN_CANCEL          OGFOCF(OGF_LCC, 0x08) // >= V1.2
#define HCICMD_ACCEPT_CONN_REQ             OGFOCF(OGF_LCC, 0x09)
#define HCICMD_REJECT_CONN_REQ             OGFOCF(OGF_LCC, 0x0a)
#define HCICMD_LINK_KEY_REQ_REPLY          OGFOCF(OGF_LCC, 0x0b)
#define HCICMD_LINK_KEY_REQ_NEG_REPLY      OGFOCF(OGF_LCC, 0x0c)
#define HCICMD_PIN_CODE_REQ_REPLY          OGFOCF(OGF_LCC, 0x0d)
#define HCICMD_PIN_CODE_REQ_NEG_REPLY      OGFOCF(OGF_LCC, 0x0e)
#define HCICMD_CHANGE_CONN_PKT_TYPE        OGFOCF(OGF_LCC, 0x0f)
#define HCICMD_AUTH_REQ                    OGFOCF(OGF_LCC, 0x11)
#define HCICMD_SET_CONN_ENCRYPTION         OGFOCF(OGF_LCC, 0x13)
#define HCICMD_CHANGE_CONN_LINK_KEY        OGFOCF(OGF_LCC, 0x15)
#define HCICMD_MASTER_LINK_KEY             OGFOCF(OGF_LCC, 0x17)
#define HCICMD_REMOTE_NAME_REQ             OGFOCF(OGF_LCC, 0x19)
#define HCICMD_REMOTE_NAME_REQ_CANCEL      OGFOCF(OGF_LCC, 0x1a) // >= V1.2
#define HCICMD_READ_REMOTE_SUPP_FEAT       OGFOCF(OGF_LCC, 0x1b)
#define HCICMD_READ_REMOTE_EXTENDED_FEAT   OGFOCF(OGF_LCC, 0x1c) // >= V1.2
#define HCICMD_READ_REMOTE_VERSION_INFO    OGFOCF(OGF_LCC, 0x1d)
#define HCICMD_READ_CLOCK_OFFSET           OGFOCF(OGF_LCC, 0x1f)
#define HCICMD_READ_LMP_HANDLE             OGFOCF(OGF_LCC, 0x20) // >= V1.2
#define HCICMD_SETUP_SYNC_CONN             OGFOCF(OGF_LCC, 0x28) // >= V1.2
#define HCICMD_ACCEPT_SYNC_CONN_REQ        OGFOCF(OGF_LCC, 0x29) // >= V1.2
#define HCICMD_REJECT_SYNC_CONN_REQ        OGFOCF(OGF_LCC, 0x2a) // >= V1.2

/* HCI Link Policy Commands */
#define HCICMD_HOLD_MODE                   OGFOCF(OGF_LPC, 0x01)
#define HCICMD_SNIFF_MODE                  OGFOCF(OGF_LPC, 0x03)
#define HCICMD_EXIT_SNIFF_MODE             OGFOCF(OGF_LPC, 0x04)
#define HCICMD_PARK_STATE                  OGFOCF(OGF_LPC, 0x05)
#define HCICMD_EXIT_PARK_STATE             OGFOCF(OGF_LPC, 0x06)
#define HCICMD_QOS_SETUP                   OGFOCF(OGF_LPC, 0x07)
#define HCICMD_ROLE_DISCOVERY              OGFOCF(OGF_LPC, 0x09)
#define HCICMD_SWITCH_ROLE                 OGFOCF(OGF_LPC, 0x0b)
#define HCICMD_READ_LINK_POLICY_SET        OGFOCF(OGF_LPC, 0x0c)
#define HCICMD_WRITE_LINK_POLICY_SET       OGFOCF(OGF_LPC, 0x0d)
#define HCICMD_READ_DEF_LINK_POLICY_SET    OGFOCF(OGF_LPC, 0x0e) // >= V1.2
#define HCICMD_WRITE_DEF_LINK_POLICY_SET   OGFOCF(OGF_LPC, 0x0f) // >= V1.2
#define HCICMD_FLOW_SPECIFICATION          OGFOCF(OGF_LPC, 0x10) // >= V1.2

/* HCI Controller & Baseband Commands */
#define HCICMD_SET_EVENT_MASK              OGFOCF(OGF_CBC, 0x01)
#define HCICMD_RESET                       OGFOCF(OGF_CBC, 0x03)
#define HCICMD_SET_EVENT_FILTER            OGFOCF(OGF_CBC, 0x05)
#define HCICMD_FLUSH                       OGFOCF(OGF_CBC, 0x08)
#define HCICMD_READ_PIN_TYPE               OGFOCF(OGF_CBC, 0x09)
#define HCICMD_WRITE_PIN_TYPE              OGFOCF(OGF_CBC, 0x0a)
#define HCICMD_CREATE_NEW_UNIT_KEY         OGFOCF(OGF_CBC, 0x0b)
#define HCICMD_READ_STORED_LINK_KEY        OGFOCF(OGF_CBC, 0x0d)
#define HCICMD_WRITE_STORED_LINK_KEY       OGFOCF(OGF_CBC, 0x11)
#define HCICMD_DELETE_STORED_LINK_KEY      OGFOCF(OGF_CBC, 0x12)
#define HCICMD_WRITE_LOCAL_NAME            OGFOCF(OGF_CBC, 0x13)
#define HCICMD_READ_LOCAL_NAME             OGFOCF(OGF_CBC, 0x14)
#define HCICMD_READ_CONN_ACCEPT_TIMEOUT    OGFOCF(OGF_CBC, 0x15)
#define HCICMD_WRITE_CONN_ACCEPT_TIMEOUT   OGFOCF(OGF_CBC, 0x16)
#define HCICMD_READ_PAGE_TIMEOUT           OGFOCF(OGF_CBC, 0x17)
#define HCICMD_WRITE_PAGE_TIMEOUT          OGFOCF(OGF_CBC, 0x18)
#define HCICMD_READ_SCAN_ENABLE            OGFOCF(OGF_CBC, 0x19)
#define HCICMD_WRITE_SCAN_ENABLE           OGFOCF(OGF_CBC, 0x1a)
#define HCICMD_READ_PAGE_SCAN_ACT          OGFOCF(OGF_CBC, 0x1b)
#define HCICMD_WRITE_PAGE_SCAN_ACT         OGFOCF(OGF_CBC, 0x1c)
#define HCICMD_READ_INQUIRY_SCAN_ACT       OGFOCF(OGF_CBC, 0x1d)
#define HCICMD_WRITE_INQUIRY_SCAN_ACT      OGFOCF(OGF_CBC, 0x1e)
#define HCICMD_READ_AUTH_ENABLE            OGFOCF(OGF_CBC, 0x1f)
#define HCICMD_WRITE_AUTH_ENABLE           OGFOCF(OGF_CBC, 0x20)
#define HCICMD_READ_ENCRYPTION_MODE        OGFOCF(OGF_CBC, 0x21)
#define HCICMD_WRITE_ENCRYPTION_MODE       OGFOCF(OGF_CBC, 0x22)
#define HCICMD_READ_CLASS_OF_DEVICE        OGFOCF(OGF_CBC, 0x23)
#define HCICMD_WRITE_CLASS_OF_DEVICE       OGFOCF(OGF_CBC, 0x24)
#define HCICMD_READ_VOICE_SETTING          OGFOCF(OGF_CBC, 0x25)
#define HCICMD_WRITE_VOICE_SETTING         OGFOCF(OGF_CBC, 0x26)
#define HCICMD_READ_AUTO_FLUSH_TIMEOUT     OGFOCF(OGF_CBC, 0x27)
#define HCICMD_WRITE_AUTO_FLUSH_TIMEOUT    OGFOCF(OGF_CBC, 0x28)
#define HCICMD_READ_NUM_BROADCAST_RETRANS  OGFOCF(OGF_CBC, 0x29)
#define HCICMD_WRITE_NUM_BROADCAST_RETRANS OGFOCF(OGF_CBC, 0x2a)
#define HCICMD_READ_HOLD_MODE_ACT          OGFOCF(OGF_CBC, 0x2b)
#define HCICMD_WRITE_HOLD_MODE_ACT         OGFOCF(OGF_CBC, 0x2c)
#define HCICMD_READ_TRANSMIT_POWER_LEVEL   OGFOCF(OGF_CBC, 0x2d)
#define HCICMD_READ_SYNC_FLOW_CTRL_ENABLE  OGFOCF(OGF_CBC, 0x2e)
#define HCICMD_WRITE_SYNC_FLOW_CTRL_ENABLE OGFOCF(OGF_CBC, 0x2f)
#define HCICMD_SET_CTRL2HOST_FLOW_CONTROL  OGFOCF(OGF_CBC, 0x31)
#define HCICMD_HOST_BUFFER_SIZE            OGFOCF(OGF_CBC, 0x33)
#define HCICMD_HOST_NUM_OF_CMPL_PKTS       OGFOCF(OGF_CBC, 0x35)
#define HCICMD_READ_LINK_SV_TIMEOUT        OGFOCF(OGF_CBC, 0x36)
#define HCICMD_WRITE_LINK_SV_TIMEOUT       OGFOCF(OGF_CBC, 0x37)
#define HCICMD_READ_NUM_OF_SUPP_IAC        OGFOCF(OGF_CBC, 0x38)
#define HCICMD_READ_CURRENT_IAC_LAP        OGFOCF(OGF_CBC, 0x39)
#define HCICMD_WRITE_CURRENT_IAC_LAP       OGFOCF(OGF_CBC, 0x3a)
#define HCICMD_READ_PAGE_SCAN_PERIOD_MODE  OGFOCF(OGF_CBC, 0x3b) // deprecated!
#define HCICMD_WRITE_PAGE_SCAN_PERIOD_MODE OGFOCF(OGF_CBC, 0x3c) // deprecated!
#define HCICMD_READ_PAGE_SCAN_MODE         OGFOCF(OGF_CBC, 0x3d) // deprecated!
#define HCICMD_WRITE_PAGE_SCAN_MODE        OGFOCF(OGF_CBC, 0x3e) // deprecated!
#define HCICMD_SET_AFH_HOST_CHANNEL_CLASS  OGFOCF(OGF_CBC, 0x3f) // >= V1.2
#define HCICMD_READ_INQUIRY_SCAN_TYPE      OGFOCF(OGF_CBC, 0x42) // >= V1.2
#define HCICMD_WRITE_INQUIRY_SCAN_TYPE     OGFOCF(OGF_CBC, 0x43) // >= V1.2
#define HCICMD_READ_INQUIRY_MODE           OGFOCF(OGF_CBC, 0x44) // >= V1.2
#define HCICMD_WRITE_INQUIRY_MODE          OGFOCF(OGF_CBC, 0x45) // >= V1.2
#define HCICMD_READ_PAGE_SCAN_TYPE         OGFOCF(OGF_CBC, 0x46) // >= V1.2
#define HCICMD_WRITE_PAGE_SCAN_TYPE        OGFOCF(OGF_CBC, 0x47) // >= V1.2
#define HCICMD_READ_AFH_CHAN_ASSESS_MODE   OGFOCF(OGF_CBC, 0x48) // >= V1.2
#define HCICMD_WRITE_AFH_CHAN_ASSESS_MODE  OGFOCF(OGF_CBC, 0x49) // >= V1.2

/* HCI Informational Parameters */
#define HCICMD_READ_LOCAL_VERSION_INFO     OGFOCF(OGF_INP, 0x01)
#define HCICMD_READ_LOCAL_SUPP_COMMANDS    OGFOCF(OGF_INP, 0x02) // >= V1.2
#define HCICMD_READ_LOCAL_SUPP_FEAT        OGFOCF(OGF_INP, 0x03)
#define HCICMD_READ_LOCAL_EXTENDED_FEAT    OGFOCF(OGF_INP, 0x04) // >= V1.2
#define HCICMD_READ_BUFFER_SIZE            OGFOCF(OGF_INP, 0x05)
#define HCICMD_READ_COUNTRY_CODE           OGFOCF(OGF_INP, 0x07) // deprecated!
#define HCICMD_READ_BD_ADDR                OGFOCF(OGF_INP, 0x09)

/* HCI Status Parameters */
#define HCICMD_READ_FAILED_CONTACT_COUNT   OGFOCF(OGF_STP, 0x01)
#define HCICMD_RESET_FAILED_CONTACT_COUNT  OGFOCF(OGF_STP, 0x02)
#define HCICMD_READ_LINK_QUALITY           OGFOCF(OGF_STP, 0x03)
#define HCICMD_READ_RSSI                   OGFOCF(OGF_STP, 0x05)
#define HCICMD_READ_AFH_CHANNEL_MAP        OGFOCF(OGF_STP, 0x06) // >= V1.2
#define HCICMD_READ_CLOCK                  OGFOCF(OGF_STP, 0x07) // >= V1.2

/* HCI Testing Commands */
#define HCICMD_READ_LOOPBACK_MODE          OGFOCF(OGF_TST, 0x01)
#define HCICMD_WRITE_LOOPBACK_MODE         OGFOCF(OGF_TST, 0x02)
#define HCICMD_ENABLE_DEVICE_TEST_MODE     OGFOCF(OGF_TST, 0x03)

/* HCI Events */
#define HCIEVT_INQUIRY_CMPL                0x01
#define HCIEVT_INQUIRY_RES                 0x02
#define HCIEVT_CONN_CMPL                   0x03
#define HCIEVT_CONN_REQ                    0x04
#define HCIEVT_DISCONN_CMPL                0x05
#define HCIEVT_AUTH_CMPL                   0x06
#define HCIEVT_REMOTE_NAME_REQ_CMPL        0x07
#define HCIEVT_ENCRYPTION_CHG              0x08
#define HCIEVT_CHANGE_CONN_LINK_KEY_CMPL   0x09
#define HCIEVT_MASTER_LINK_KEY_CMPL        0x0a
#define HCIEVT_READ_REMOTE_SUPP_FEAT_CMPL  0x0b
#define HCIEVT_READ_REMOTE_VERS_INFO_CMPL  0x0c
#define HCIEVT_QOS_SETUP_CMPL              0x0d
#define HCIEVT_CMD_CMPL                    0x0e
#define HCIEVT_CMD_STATUS                  0x0f
#define HCIEVT_HARDWARE_ERROR              0x10
#define HCIEVT_FLUSH_OCCURRED              0x11
#define HCIEVT_ROLE_CHG                    0x12
#define HCIEVT_NUM_OF_CMPL_PACKETS         0x13
#define HCIEVT_MODE_CHG                    0x14
#define HCIEVT_RETURN_LINK_KEYS            0x15
#define HCIEVT_PIN_CODE_REQ                0x16
#define HCIEVT_LINK_KEY_REQ                0x17
#define HCIEVT_LINK_KEY_NOTIFICATION       0x18
#define HCIEVT_LOOPBACK_CMD                0x19
#define HCIEVT_DATA_BUFFER_OVERFLOW        0x1a
#define HCIEVT_MAX_SLOTS_CHG               0x1b
#define HCIEVT_READ_CLOCK_OFFSET_CMPL      0x1c
#define HCIEVT_CONN_PKT_TYPE_CHG           0x1d
#define HCIEVT_QOS_VIOLATION               0x1e
#define HCIEVT_PAGE_SCAN_REP_MODE_CHG      0x20
#define HCIEVT_HCI_FLOW_SPEC_CMPL          0x21 // >= V1.2
#define HCIEVT_INQUIRY_RES_WITH_RSSI       0x22 // >= V1.2
#define HCIEVT_READ_REMOTE_EXT_FEAT_CMPL   0x23 // >= V1.2
#define HCIEVT_SYNC_CONN_CMPL              0x2c // >= V1.2
#define HCIEVT_SYNC_CONN_CHG               0x2d // >= V1.2

/* HCI Error codes from the BT 1.2 Spec Vol 2 Part D Nov 2003 */
#define HCIERR_SUCCESS                            0x00
#define HCIERR_UNKNOWN_HCI_CMD                    0x01
#define HCIERR_UNKNOWN_CONN_ID                    0x02
#define HCIERR_HARDWARE_FAILURE                   0x03
#define HCIERR_PAGE_TIMEOUT                       0x04
#define HCIERR_AUTH_FAILURE                       0x05
#define HCIERR_PIN_MISSING                        0x06
#define HCIERR_MEMORY_CAPACITY_EXCEEDED           0x07
#define HCIERR_CONN_TIMEOUT                       0x08
#define HCIERR_CONN_LIMIT_EXCEEDED                0x09
#define HCIERR_SYNC_CONN_LIMIT_EXCEEDED           0x0A
#define HCIERR_ACL_CONN_ALREADY_EXISTS            0x0B
#define HCIERR_CMD_DISALLOWED                     0x0C
#define HCIERR_CONN_REJECTED_LIMITED_RESOURCES    0x0D
#define HCIERR_CONN_REJECTED_SECURITY_REASONS     0x0E
#define HCIERR_CONN_REJECTED_UNACCEPTABLE_BD_ADDR 0x0F
#define HCIERR_CONN_ACCEPT_TIMEOUT_EXCEEDED       0x10
#define HCIERR_UNSUPP_FEATURE_OR_PARM_VALUE       0x11
#define HCIERR_INVALID_HCI_COMMAND_PARMS          0x12
#define HCIERR_REMOTE_USER_TERM_CONN              0x13
#define HCIERR_REMOTE_DEVICE_TERM_CONN_LOW_RES    0x14
#define HCIERR_REMOTE_DEVICE_TERM_CONN_POWER_OFF  0x15
#define HCIERR_CONN_TERM_BY_LOCAL_HOST            0x16
#define HCIERR_REPEATED_ATTEMPTS                  0x17
#define HCIERR_PAIRING_NOT_ALLOWED                0x18
#define HCIERR_UNKNOWN_LMP_PDU                    0x19
#define HCIERR_UNSUPP_REMOTE_FEATURE              0x1A
#define HCIERR_SCO_OFFSET_REJECTED                0x1B
#define HCIERR_SCO_INTERVAL_REJECTED              0x1C
#define HCIERR_SCO_AIR_MODE_REJECTED              0x1D
#define HCIERR_INVALID_LMP_PARMS                  0x1E
#define HCIERR_UNSPECIFIED_ERROR                  0x1F
#define HCIERR_UNSUPP_LMP_PARM_VALUE              0x20
#define HCIERR_ROLE_CHANGE_NOT_ALLOWED            0x21
#define HCIERR_LMP_RESPONSE_TIMEOUT               0x22
#define HCIERR_LMP_ERROR_TRANSACTION_COLLISION    0x23
#define HCIERR_LMP_PDU_NOT_ALLOWED                0x24
#define HCIERR_ENCRYPTION_MODE_NOT_ACCEPTABLE     0x25
#define HCIERR_LINK_KEY_CAN_NOT_BE_CHANGED        0x26
#define HCIERR_REQUESTED_QOS_NOT_SUPP             0x27
#define HCIERR_INSTANT_PASSED                     0x28
#define HCIERR_PAIRING_WITH_UNIT_KEY_NOT_SUPP     0x29
#define HCIERR_DIFFERENT_TRANSACTION_COLLISION    0x2A
#define HCIERR_QOS_UNACCEPTABLE_PARM              0x2C
#define HCIERR_QOS_REJECTED                       0x2D
#define HCIERR_CHANNEL_CLASSIFICATION_NOT_SUPP    0x2E
#define HCIERR_INSUFFICIENT_SECURITY              0x2F
#define HCIERR_PARM_OUT_OF_MANDATORY_RANGE        0x30
#define HCIERR_ROLE_SWITCH_PENDING                0x32
#define HCIERR_RESERVED_SLOT_VIOLATION            0x34
#define HCIERR_ROLE_SWITCH_FAILED                 0x35

/* \\\ */

/* various stuff */
#define BT_CONNHANDLE_MASK 0x0fff

#define BD_ADDR_SIZE 6

typedef struct BD_ADDR
{
    UBYTE bd_Addr[BD_ADDR_SIZE];
} BD_ADDR;

#define BT_LINK_KEY_SIZE 16
typedef struct BT_LINK_KEY
{
    UBYTE lk_LinkKey[BT_LINK_KEY_SIZE];
} BT_LINK_KEY;

/* Link Types */
#define LINKTYPE_SCO  0x00
#define LINKTYPE_ACL  0x01

/* Encryption Modes */
#define ENCMODE_NONE   0x00
#define ENCMODE_ALL    0x01

/* Packet types (see Page 134 (300) of BT Spec 2.0) */

/* for all types */
#define BTPT_NULL  0x00 /* no payload, 126 bits */
#define BTPT_POLL  0x01 /* no payload, ack required */
#define BTPT_FHS   0x02 /* 144 bits + 16 bit CRC, payload 240 bits */
#define BTPT_DM1   0x03 /* standard , 1- 18 bytes info, 2/3 FEC, 16 bit CRC */
#define BTPT_AUX1  0x09 /* ACL  only, 1- 30 bytes info, no  FEC, no CRC */

/* for 1 MBit/s */
#define BTPT_DH1   0x04 /* ACL  only, 1- 28 bytes info, no  FEC, 16 bit CRC */
#define BTPT_DM3   0x0a /* ACL  only, 2-123 bytes info, 2/3 FEC, 16 bit CRC */
#define BTPT_DH3   0x0b /* ACL  only, 2-185 bytes info, no  FEC, 16 bit CRC */
#define BTPT_DM5   0x0e /* ACL  only, 2-226 bytes info, 2/3 FEC, 16 bit CRC */
#define BTPT_DH5   0x0f /* ACL  only, 2-341 bytes info, no  FEC, 16 bit CRC */
#define BTPT_HV1   0x05 /* SCO  only,    10 bytes info, 1/3 FEC, no CRC, payload 240 bits */
#define BTPT_HV2   0x06 /* SCO  only,    20 bytes info, 2/3 FEC, no CRC, payload 240 bits */
#define BTPT_HV3   0x07 /* SCO  only,    30 bytes info, no  FEC, no CRC, payload 240 bits */
#define BTPT_DV    0x08 /* SCO  only, 1- 10 bytes data, 2/3 FEC, 16 bit CRC, 80 bits voice, 150 bits data */
#define BTPT_EV3   0x07 /* eSCO only, 1- 30 bytes info, no  FEC, 16 bit CRC */
#define BTPT_EV4   0x0c /* eSCO only, 1-120 bytes info, 2/3 FEC, 16 bit CRC */
#define BTPT_EV5   0x0d /* eSCO only, 1-180 bytes info, no  FEC, 16 bit CRC */

/* for 2-3 MBit/s */
#define BTPT_2_DH1 0x04 /* ACL  only, 2- 56 bytes info, no  FEC, 16 bit CRC */
#define BTPT_3_DH1 0x08 /* ACL  only, 2- 85 bytes info, no  FEC, 16 bit CRC */
#define BTPT_2_DH3 0x0a /* ACL  only, 2-369 bytes info, no  FEC, 16 bit CRC */
#define BTPT_3_DH3 0x0b /* ACL  only, 2-554 bytes info, no  FEC, 16 bit CRC */
#define BTPT_2_DH5 0x0e /* ACL  only, 2-681 bytes info, no  FEC, 16 bit CRC */
#define BTPT_3_DH5 0x0f /* ACL  only, 2-1023 bytes info,no  FEC, 16 bit CRC */
#define BTPT_2_EV3 0x06 /* eSCO only, 1- 60 bytes info, no  FEC, 16 bit CRC */
#define BTPT_3_EV3 0x07 /* eSCO only, 1- 90 bytes info, no  FEC, 16 bit CRC */
#define BTPT_2_EV5 0x0c /* eSCO only, 1-360 bytes info, no  FEC, 16 bit CRC */
#define BTPT_3_EV5 0x0d /* eSCO only, 1-540 bytes info, no  FEC, 16 bit CRC */

/* currently, even with EDR, the maximum packet size for ACL packets is 1023 bytes (plus 4 bytes ACL header) */
#define MAX_ACL_IN_BUFFER_SIZE 1028

/* Packet Type Flags */
#define BTPTF_NOT_2_DH1 0x0002
#define BTPTF_NOT_3_DH1 0x0004
#define BTPTF_DM1       0x0008
#define BTPTF_DH1       0x0010
#define BTPTF_HV1       0x0020 // SCO
#define BTPTF_HV2       0x0040 // SCO
#define BTPTF_HV3       0x0080 // SCO
#define BTPTF_NOT_2_DH3 0x0100
#define BTPTF_NOT_3_DH3 0x0200
#define BTPTF_DM3       0x0400
#define BTPTF_DH3       0x0800
#define BTPTF_NOT_2_DH5 0x1000
#define BTPTF_NOT_3_DH5 0x2000
#define BTPTF_DM5       0x4000
#define BTPTF_DH5       0x8000

#define BTPTF_ALL_ACL   (BTPTF_NOT_2_DH1|BTPTF_NOT_3_DH1|BTPTF_DM1|BTPTF_DH1|BTPTF_NOT_2_DH3|BTPTF_NOT_3_DH3|BTPTF_DM3|BTPTF_DH3|BTPTF_NOT_2_DH5|BTPTF_NOT_3_DH5|BTPTF_DM5|BTPTF_DH5)
#define BTPTF_ALL_SCO   (BTPTF_HV1|BTPTF_HV2|BTPTF_HV3)
#define BTPTF_1_SLOT    (BTPTF_NOT_2_DH1|BTPTF_NOT_3_DH1|BTPTF_DM1|BTPTF_DH1|BTPTF_HV1|BTPTF_HV2|BTPTF_HV3)
#define BTPTF_3_SLOTS   (BTPTF_NOT_2_DH3|BTPTF_NOT_3_DH3|BTPTF_DM3|BTPTF_DH3)
#define BTPTF_5_SLOTS   (BTPTF_NOT_2_DH5|BTPTF_NOT_3_DH5|BTPTF_DM5|BTPTF_DH5)
#define BTPTF_EDR_MASK  (BTPTF_NOT_2_DH1|BTPTF_NOT_3_DH1|BTPTF_NOT_2_DH3|BTPTF_NOT_3_DH3|BTPTF_NOT_2_DH5|BTPTF_NOT_3_DH5)
#define BTPTF_NOT_MASK  (BTPTF_NOT_2_DH1|BTPTF_NOT_3_DH1|BTPTF_NOT_2_DH3|BTPTF_NOT_3_DH3|BTPTF_NOT_2_DH5|BTPTF_NOT_3_DH5)

/* Link Policy Settings */
#define BTLPF_ROLE_SWITCH   0x0001
#define BTLPF_HOLD_MODE     0x0002
#define BTLPF_SNIFF_MODE    0x0004
#define BTLPF_PARK_STATE    0x0008

/* QoS stuff */
#define BTQOSST_NO_TRAFFIC  0x00
#define BTQOSST_BEST_EFFORT 0x01
#define BTQOSST_GUARANTEED  0x02

/* Supported features for LMP */
#define LMPF_3_SLOT_PACKETS    0x00
#define LMPF_5_SLOT_PACKETS    0x01
#define LMPF_ENCRYPTION        0x02
#define LMPF_SLOT_OFFSET       0x03
#define LMPF_TIMING_ACCURACY   0x04
#define LMPF_ROLE_SWITCH       0x05
#define LMPF_HOLD_MODE         0x06
#define LMPF_SNIFF_MODE        0x07
#define LMPF_PARK_STATE        0x08
#define LMPF_POWER_CTRL_REQ    0x09
#define LMPF_CQDDR             0x0a
#define LMPF_SCO_LINK          0x0b
#define LMPF_HV2_PACKETS       0x0c
#define LMPF_HV3_PACKETS       0x0d
#define LMPF_ULAW_SCO_DATA     0x0e
#define LMPF_ALAW_SCO_DATA     0x0f
#define LMPF_CVSD_SCO_DATA     0x10
#define LMPF_PAIRING_PARAM_NEG 0x11
#define LMPF_POWER_CTRL        0x12
#define LMPF_TRANS_SCO_DATA    0x13
#define LMPF_FLOW_CTRL_LAG_LSB 0x14
#define LMPF_FLOW_CTRL_LAG_MB  0x15
#define LMPF_FLOW_CTRL_LAG_MSB 0x16
#define LMPF_BCAST_ENCRYPTION  0x17 // possibly V1.1
#define LMPF_EDR_ACL_2MBPS     0x19 // >= V2.0
#define LMPF_EDR_ACL_3MBPS     0x1a // >= V2.0
#define LMPF_ENH_INQ_SCAN      0x1b
#define LMPF_ILACE_INQ_SCAN    0x1c
#define LMPF_ILACE_PAGE_SCAN   0x1d
#define LMPF_RSSI_WITH_INQUIRY 0x1e
#define LMPF_EV3_PACKETS       0x1f
#define LMPF_EV4_PACKETS       0x20
#define LMPF_EV5_PACKETS       0x21
#define LMPF_AFH_CAP_SLAVE     0x23
#define LMPF_AFH_CLASS_SLAVE   0x24
#define LMPF_3_SLOT_EDR_ACL    0x27 // >= V2.0
#define LMPF_5_SLOT_EDR_ACL    0x28 // >= V2.0
#define LMPF_AFH_CAP_MASTER    0x2b
#define LMPF_AFH_CLASS_MASTER  0x2c
#define LMPF_EDR_ESCO_2MBPS    0x2d
#define LMPF_EDR_ESCO_3MBPS    0x2e
#define LMPF_3_SLOT_EDR_ESCO   0x2f
#define LMPF_EXT_FEATURES      0x3f

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* BLUETOOTH_HCI_H */
