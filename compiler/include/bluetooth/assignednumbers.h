#ifndef BLUETOOTH_ASSIGNEDNUMBERS_H
#define BLUETOOTH_ASSIGNEDNUMBERS_H
/*
**	$VER: assignednumbers.h 1.0 (06.05.06)
**
**	Bluetooth Assigned Numbers definitions include file
**
**	(C) Copyright 2006 Chris Hodges
**	    All Rights Reserved
*/

/* Inquiry Access Codes */

#define BT_GIAC 0x9e8b33
#define BT_LIAC 0x9e8b00 // others reserved

/* HCI_Version levels */
#define HCI_VERSION_1_0B 0      /* Bluetooth HCI Specification 1.0B */
#define HCI_VERSION_1_1  1      /* Bluetooth HCI Specification 1.1 */
#define HCI_VERSION_1_2  2      /* Bluetooth HCI Specification 1.2 */
#define HCI_VERSION_2_0  3      /* Bluetooth HCI Specification 2.0 */

/* Link Manager Protocol (LMP) Version levels */
#define LMP_VERSION_1_0  0      /* Bluetooth LMP 1.0 */
#define LMP_VERSION_1_1  1      /* Bluetooth LMP 1.1 */
#define LMP_VERSION_1_2  2      /* Bluetooth LMP 1.2 */
#define LMP_VERSION_2_0  3      /* Bluetooth LMP 2.0 */

/* Pre-defined L2CAP Channel Identifiers 3.2 Protocol and Service Multiplexor (PSM)  */

#define PSM_SDP              0x0001   /* Service Discovery Protocol (SDP) */
#define PSM_RFCOMM           0x0003   /* RFCOMM with TS 07.10 */
#define PSM_TCS_BIN          0x0005   /* Telephony Control / TCS Binary */
#define PSM_TCS_BIN_CORDLESS 0x0007   /* Cordless Telephony Control / TCS Binary */
#define PSM_BNEP             0x000f   /* Network Encapsulation Protocol */
#define PSM_HID_CONTROL      0x0011   /* Human Interface Device Control */
#define PSM_HID_INTERRUPT    0x0013   /* Human Interface Device Interrupt */
#define PSM_UPNP             0x0015   /* Universal Plug'n Play */
#define PSM_AVCTP            0x0017   /* Audio/Video Control Transport Protocol */
#define PSM_AVDTP            0x0019   /* Audio/Video Distribution Transport Protocol */
#define PSM_UDI_C_PLANE      0x001d   /* Unrestricted Digital Information Profile */

/* Bluetooth Base UUID */

// 00000000-0000-1000-8000-00805F9B34FB

/* Protocol UUID16 */
#define BTPID_SDP         0x0001 /* bt-sdp, Service Discovery Protocol (SDP) */
#define BTPID_UDP         0x0002
#define BTPID_RFCOMM      0x0003 /* bt-rfcomm, RFCOMM with TS 07.10 */
#define BTPID_TCP         0x0004
#define BTPID_TCS_BIN     0x0005 /* bt-tcs, Telephony Control Specification / TCS Binary */
#define BTPID_TCS_AT      0x0006 /* modem */
#define BTPID_OBEX        0x0008 /* obex */
#define BTPID_IP          0x0009
#define BTPID_FTP         0x000a /* ftp */
#define BTPID_HTTP        0x000c /* http */
#define BTPID_WSP         0x000e /* wsp */
#define BTPID_BNEP        0x000f /* BNEP */
#define BTPID_UPNP        0x0010 /* ESDP */
#define BTPID_HIDP        0x0011 /* Human Interface Device Profile (HID) */
#define BTPID_HCRP_CTRL   0x0012 /* HardcopyControlChannel, Hardcopy Cable Replacement Profile (HCRP) */
#define BTPID_HCRP_DATA   0x0014 /* HardcopyDataChannel, Hardcopy Cable Replacement Profile (HCRP) */
#define BTPID_HCRP_NOTIFY 0x0016 /* HardcopyNotification, Hardcopy Cable Replacement Profile (HCRP) */
#define BTPID_AVCTP       0x0017 /* Audio/Video Control Transport Protocol */
#define BTPID_AVDTP       0x0019 /* Audio/Video Distribution Transport Protocol */
#define BTPID_CMTP        0x001b /* bt-cmtp, CAPI Message Transport Protocol */
#define BTPID_UDI_C_PLANE 0x001d /* Unrestricted Digital Information Profile [UDI] */
#define BTPID_L2CAP       0x0100 /* bt-l2cap, Logical Link Control and Adaptation Protocol */

/* Service Class UUID16 */
#define BTSID_SERVICE_DISCOVERY_SERVER       0x1000 /* SDP */
#define BTSID_BROWSE_GROUP_DESCRIPTOR        0x1001 /* SDP */
#define BTSID_PUBLIC_BROWSE_GROUP            0x1002 /* SDP */
#define BTSID_SERIAL_PORT                    0x1101 /* GAP */
#define BTSID_LAN_ACCESS_USING_PPP           0x1102
#define BTSID_DIALUP_NETWORKING              0x1103 /* DUN */
#define BTSID_IRMC_SYNC                      0x1104 /* SYNC */
#define BTSID_OBEX_OBJECT_PUSH               0x1105 /* OBEX Push */
#define BTSID_OBEX_FILE_TRANSFER             0x1106 /* OBEX FTP */
#define BTSID_IRMC_SYNC_COMMAND              0x1107 /* SYNC */
#define BTSID_HEADSET                        0x1108 /* GAP */
#define BTSID_CORDLESS_TELEPHONY             0x1109 /* CTP */
#define BTSID_AUDIO_SOURCE                   0x110a
#define BTSID_AUDIO_SINK                     0x110b
#define BTSID_AVRCP_TARGET                   0x110c /* AVRCP-T */
#define BTSID_A2DP                           0x110d /* A2DP */
#define BTSID_AVRCP                          0x110e /* AVRCP */
#define BTSID_VIDEO_CONFERENCING             0x110f /* VCP */
#define BTSID_INTERCOM                       0x1110 /* Intercom */
#define BTSID_FAX                            0x1111 /* Fax */
#define BTSID_HEADSET_AUDIO_GATEWAY          0x1112 /* GAP */
#define BTSID_WAP                            0x1113 /* WAP */
#define BTSID_WAP_CLIENT                     0x1114
#define BTSID_PAN_U                          0x1115 /* PAN */
#define BTSID_PAN_NAP                        0x1116 /* PAN */
#define BTSID_PAN_GN                         0x1117 /* PAN */
#define BTSID_DIRECT_PRINTING                0x1118 /* Basic Printing */
#define BTSID_REFERENCE_PRINTING             0x1119 /* Basic Printing */
#define BTSID_IMAGING                        0x111a /* Imaging */
#define BTSID_IMAGING_RESPONDER              0x111b /* Imaging */
#define BTSID_IMAGING_AUTOMATIC_ARCHIVE      0x111c /* Imaging */
#define BTSID_IMAGING_REFERENCED_OBJECTS     0x111d /* Imaging */
#define BTSID_HANDSFREE                      0x111e /* HFP */
#define BTSID_HANDSFREE_AUDIO_GATEWAY        0x111f /* HFP */
#define BTSID_DIRECT_PRINTING_REF_OBJECTS    0x1120 /* Basic Printing */
#define BTSID_REFLECTED_UI                   0x1121 /* Basic Printing */
#define BTSID_BASIC_PRINTING                 0x1122 /* Basic Printing */
#define BTSID_PRINTING_STATUS                0x1123 /* Basic Printing */
#define BTSID_HID                            0x1124 /* HIDP */
#define BTSID_HCRP                           0x1125 /* HCRP */
#define BTSID_HCRP_PRINT                     0x1126 /* HCRP */
#define BTSID_HCRP_SCAN                      0x1127 /* HCRP */
#define BTSID_COMMON_ISDN_ACCESS             0x1128 /* CAPI */
#define BTSID_VIDEO_CONFERENCING_GW          0x1129 /* VCP */
#define BTSID_UDI_MT                         0x112a /* UDI */
#define BTSID_UDI_TA                         0x112b /* UDI */
#define BTSID_AUDIO_VIDEO                    0x112c /* VCP */
#define BTSID_SIM_ACCESS                     0x112d /* SAP */
#define BTSID_PHONEBOOK_ACCESS_PCE           0x112e /* PBAP */
#define BTSID_PHONEBOOK_ACCESS_PSE           0x112f /* PBAP */
#define BTSID_PNP_INFORMATION                0x1200 /* Device Id */
#define BTSID_GENERIC_NETWORKING             0x1201
#define BTSID_GENERIC_FILETRANSFER           0x1202
#define BTSID_GENERIC_AUDIO                  0x1203
#define BTSID_GENERIC_TELEPHONY              0x1204
#define BTSID_UPNP_SERVICE                   0x1205 /* ESDP */
#define BTSID_UPNP_IP_SERVICE                0x1206 /* ESDP */
#define BTSID_ESDP_UPNP_IP_PAN               0x1300 /* ESDP */
#define BTSID_ESDP_UPNP_IP_LAP               0x1301 /* ESDP */
#define BTSID_ESDP_UPNP_L2CAP                0x1302 /* ESDP */
#define BTSID_VIDEO_SOURCE                   0x1303 /* VDP */
#define BTSID_VIDEO_SINK                     0x1304 /* VDP */
#define BTSID_VIDEO_DISTRIBUTION             0x1305 /* VDP */

/* Attribute Identifier codes Numeric IDs */

#define BTAID_SERVICE_RECORD_HANDLE                0x0000 /* SDP */
#define BTAID_SERVICE_CLASS_ID_LIST                0x0001 /* SDP */
#define BTAID_SERVICE_RECORD_STATE                 0x0002 /* SDP */
#define BTAID_SERVICE_ID                           0x0003 /* SDP */
#define BTAID_PROTOCOL_DESCRIPTOR_LIST             0x0004 /* SDP */
#define BTAID_BROWSE_GROUP_LIST                    0x0005 /* SDP */
#define BTAID_LANGUAGE_BASE_ATTRIBUTE_ID_LIST      0x0006 /* SDP */
#define BTAID_SERVICE_INFO_TTL                     0x0007 /* SDP */
#define BTAID_SERVICE_AVAILABILITY                 0x0008 /* SDP */
#define BTAID_BLUETOOTH_PROFILE_DESCRIPTOR_LIST    0x0009 /* SDP */
#define BTAID_DOCUMENTATION_URL                    0x000a /* SDP */
#define BTAID_CLIENT_EXECUTABLE_URL                0x000b /* SDP */
#define BTAID_ICON_URL                             0x000c /* SDP */
#define BTAID_ADDITIONAL_PROTOCOL_DESCRIPTOR_LISTS 0x000d /* SDP */
#define BTAID_PRIMARY_SERVICE_NAME                 0x0100 /* SDP (moves with the language) */
#define BTAID_PRIMARY_SERVICE_DESCRIPTION          0x0101 /* SDP (moves with the language) */
#define BTAID_PRIMARY_PROVIDER_NAME                0x0102 /* SDP (moves with the language) */
#define BTAID_GROUP_ID                             0x0200 /* SDP */
#define BTAID_IP_SUBNET                            0x0200 /* PAN */
#define BTAID_VERSION_NUMBER_LIST                  0x0200 /* SDP */
#define BTAID_SERVICE_DATABASE_STATE               0x0201 /* SDP */
#define BTAID_SERVICE_VERSION                      0x0300 /* SDP */
#define BTAID_EXTERNAL_NETWORK                     0x0301 /* CTP */
#define BTAID_NETWORK                              0x0301 /* HFP */
#define BTAID_SUPPORTED_DATA_STORES_LIST           0x0301 /* SYNC */
#define BTAID_FAX_CLASS_1_SUPPORT                  0x0302 /* FAX */
#define BTAID_REMOTE_AUDIO_VOLUME_CONTROL          0x0302 /* GAP */
#define BTAID_FAX_CLASS_2_0_SUPPORT                0x0303 /* FAX */
#define BTAID_SUPPORTED_FORMATS_LIST               0x0303 /* OBEX PUSH */
#define BTAID_FAX_CLASS_2_SUPPORT                  0x0304 /* FAX */
#define BTAID_AUDIO_FEEDBACK_SUPPORT               0x0305
#define BTAID_NETWORK_ADDRESS                      0x0306 /* WAP */
#define BTAID_WAP_GATEWAY                          0x0307 /* WAP */
#define BTAID_HOME_PAGE_URL                        0x0308 /* WAP */
#define BTAID_WAP_STACK_TYPE                       0x0309 /* WAP */
#define BTAID_SECURITY_DESCRIPTION                 0x030a /* PAN */
#define BTAID_NET_ACCESS_TYPE                      0x030b /* PAN */
#define BTAID_MAX_NET_ACCESSRATE                   0x030c /* PAN */
#define BTAID_IPV4_SUBNET                          0x030d /* PAN */
#define BTAID_IPV6_SUBNET                          0x030e /* PAN */
#define BTAID_SUPPORTED_CAPABILITIES               0x0310 /* Imaging */
#define BTAID_SUPPORTED_FEATURES                   0x0311 /* Imaging and HFP */
#define BTAID_SUPPORTED_FUNCTIONS                  0x0312 /* Imaging */
#define BTAID_TOTAL_IMAGING_DATA_CAPACITY          0x0313 /* Imaging */
#define BTAID_SUPPORTED_REPOSITORIES               0x0314 /* PBAP */

/* Major Service Classes */
#define MSCB_LIMITED_DISCOVERY 13
#define MSCB_POSITIONING       16
#define MSCB_NETWORKING        17
#define MSCB_RENDERING         18
#define MSCB_CAPTURING         19
#define MSCB_OBJECT_TRANSFER   20
#define MSCB_AUDIO             21
#define MSCB_TELEPHONY         22
#define MSCB_INFORMATION       23

#define MSCF_LIMITED_DISCOVERY (1<<MSCB_LIMITED_DISCOVER)
#define MSCF_POSITIONING       (1<<MSCB_POSITIONING)
#define MSCF_NETWORKING        (1<<MSCB_NETWORKING)
#define MSCF_RENDERING         (1<<MSCB_RENDERING)
#define MSCF_CAPTURING         (1<<MSCB_CAPTURING)
#define MSCF_OBJECT_TRANSFER   (1<<MSCB_OBJECT_TRANSFER)
#define MSCF_AUDIO             (1<<MSCB_AUDIO)
#define MSCF_TELEPHONY         (1<<MSCB_TELEPHONY)
#define MSCF_INFORMATION       (1<<MSCB_INFORMATION)

/* Major Device Classes (shifted right by 8 bits) */
#define MAJORDEVICECLASS_MASK  0x001f00
#define MAJORDEVICECLASS_SHIFT 8

#define MDC_MISC           0
#define MDC_COMPUTER       1
#define MDC_PHONE          2
#define MDC_LAN            3
#define MDC_AUDIO_VIDEO    4
#define MDC_PERIPHERAL     5
#define MDC_IMAGING        6
#define MDC_WEARABLE       7
#define MDC_TOY            8
#define MDC_UNCATEGORIZED 31

/* Minor Device Classes (combined with Major Device Class) */
#define MINORDEVICECLASS_MASK  0x001ffc
#define MINORDEVICECLASS_SHIFT 2

#define MDC_COMPUTER_UNCATEGORIZED ((MDC_COMPUTER<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|0)
#define MDC_COMPUTER_DESKTOP       ((MDC_COMPUTER<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|1)
#define MDC_COMPUTER_SERVER        ((MDC_COMPUTER<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|2)
#define MDC_COMPUTER_LAPTOP        ((MDC_COMPUTER<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|3)
#define MDC_COMPUTER_HANDHELD      ((MDC_COMPUTER<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|4)
#define MDC_COMPUTER_PALM          ((MDC_COMPUTER<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|5)
#define MDC_COMPUTER_WEARABLE      ((MDC_COMPUTER<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|6)

#define MDC_PHONE_UNCATEGORIZED    ((MDC_PHONE<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|0)
#define MDC_PHONE_CELLULAR         ((MDC_PHONE<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|1)
#define MDC_PHONE_CORDLESS         ((MDC_PHONE<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|2)
#define MDC_PHONE_SMARTPHONE       ((MDC_PHONE<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|3)
#define MDC_PHONE_WIREDMODEM       ((MDC_PHONE<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|4)
#define MDC_PHONE_ISDN             ((MDC_PHONE<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|5)

#define MDC_LAN_FULLYAVAIL         ((MDC_LAN<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(0<<3))
#define MDC_LAN_17UTIL             ((MDC_LAN<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(1<<3))
#define MDC_LAN_33UTIL             ((MDC_LAN<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(2<<3))
#define MDC_LAN_50UTIL             ((MDC_LAN<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(3<<3))
#define MDC_LAN_67UTIL             ((MDC_LAN<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(4<<3))
#define MDC_LAN_83UTIL             ((MDC_LAN<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(5<<3))
#define MDC_LAN_99UTIL             ((MDC_LAN<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(6<<3))
#define MDC_LAN_UNAVAILABLE        ((MDC_LAN<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(7<<3))

#define MDC_AV_UNCATEGORIZED       ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|0)
#define MDC_AV_HEADSET             ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|1)
#define MDC_AV_HANDSFREE           ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|2)
#define MDC_AV_MICROPHONE          ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|4)
#define MDC_AV_LOUDSPEAKER         ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|5)
#define MDC_AV_HEADPHONES          ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|6)
#define MDC_AV_PORTABLEAUDIO       ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|7)
#define MDC_AV_CARAUDIO            ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|8)
#define MDC_AV_SETTOPBOX           ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|9)
#define MDC_AV_HIFIAUDIO           ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|10)
#define MDC_AV_VCR                 ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|11)
#define MDC_AV_VIDEOCAMERA         ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|12)
#define MDC_AV_CAMCORDER           ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|13)
#define MDC_AV_VIDEOMONITOR        ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|14)
#define MDC_AV_VIDEODISPLAY        ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|15)
#define MDC_AV_VIDEOCONFERENCING   ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|16)
#define MDC_AV_GAMINGTOY           ((MDC_AUDIO_VIDEO<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|18)

#define MDC_PERI_UNCATEGORIZED     ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(0<<4)|0)
#define MDC_PERI_U_JOYSTICK        ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(0<<4)|1)
#define MDC_PERI_U_GAMEPAD         ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(0<<4)|2)
#define MDC_PERI_U_REMOTECONTROL   ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(0<<4)|3)
#define MDC_PERI_U_SENSING         ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(0<<4)|4)
#define MDC_PERI_U_TABLET          ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(0<<4)|5)
#define MDC_PERI_U_CARDREADER      ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(0<<4)|6)
#define MDC_PERI_KEYBOARD          ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(1<<4)|0)
#define MDC_PERI_K_JOYSTICK        ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(1<<4)|1)
#define MDC_PERI_K_GAMEPAD         ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(1<<4)|2)
#define MDC_PERI_K_REMOTECONTROL   ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(1<<4)|3)
#define MDC_PERI_K_SENSING         ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(1<<4)|4)
#define MDC_PERI_K_TABLET          ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(1<<4)|5)
#define MDC_PERI_K_CARDREADER      ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(1<<4)|6)
#define MDC_PERI_POINTING          ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(2<<4)|0)
#define MDC_PERI_P_JOYSTICK        ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(2<<4)|1)
#define MDC_PERI_P_GAMEPAD         ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(2<<4)|2)
#define MDC_PERI_P_REMOTECONTROL   ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(2<<4)|3)
#define MDC_PERI_P_SENSING         ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(2<<4)|4)
#define MDC_PERI_P_TABLET          ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(2<<4)|5)
#define MDC_PERI_P_CARDREADER      ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(2<<4)|6)
#define MDC_PERI_COMBO             ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(3<<4)|0)
#define MDC_PERI_C_JOYSTICK        ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(3<<4)|1)
#define MDC_PERI_C_GAMEPAD         ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(3<<4)|2)
#define MDC_PERI_C_REMOTECONTROL   ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(3<<4)|3)
#define MDC_PERI_C_SENSING         ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(3<<4)|4)
#define MDC_PERI_C_TABLET          ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(3<<4)|5)
#define MDC_PERI_C_CARDREADER      ((MDC_PERIPHERAL<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(3<<4)|6)

#define MDC_IMAGING_DISPLAY        ((MDC_IMAGING<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(1<<2))
#define MDC_IMAGING_CAMERA         ((MDC_IMAGING<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(2<<2))
#define MDC_IMAGING_SCANNER        ((MDC_IMAGING<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(3<<2))
#define MDC_IMAGING_PRINTER        ((MDC_IMAGING<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|(4<<2))

#define MDC_WEARABLE_WATCH         ((MDC_WEARABLE<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|1)
#define MDC_WEARABLE_PAGER         ((MDC_WEARABLE<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|2)
#define MDC_WEARABLE_JACKET        ((MDC_WEARABLE<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|3)
#define MDC_WEARABLE_HELMET        ((MDC_WEARABLE<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|4)
#define MDC_WEARABLE_GLASSES       ((MDC_WEARABLE<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|5)

#define MDC_TOY_ROBOT              ((MDC_TOY<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|1)
#define MDC_TOY_VEHICLE            ((MDC_TOY<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|2)
#define MDC_TOY_DOLL               ((MDC_TOY<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|3)
#define MDC_TOY_CONTROLLER         ((MDC_TOY<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|4)
#define MDC_TOY_GAME               ((MDC_TOY<<(MAJORDEVICECLASS_SHIFT-MINORDEVICECLASS_SHIFT))|5)

/* AVDTP Media Types */
#define AVDTP_MT_AUDIO      0
#define AVDTP_MT_VIDEO      1
#define AVDTP_MT_MULTIMEDIA 2

/* AVDTP Audio Codec IDs */
#define AUDIO_CODEC_SBC         0
#define AUDIO_CODEC_MPEG1_2     1
#define AUDIO_CODEC_MPEG2_4_AAC 2
#define AUDIO_CODEC_ATRAC       4

/* AVDTP Video Codec IDs */
#define VIDEO_CODEC_H263_BASE   1
#define VIDEO_CODEC_MPEG4       2
#define VIDEO_CODEC_H263_PROF_3 3
#define VIDEO_CODEC_H263_PROF_8 4

#endif /* BLUETOOTH_ASSIGNEDNUMBERS_H */
