#ifndef HARDWARE_XHCI_H
#define HARDWARE_XHCI_H
/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved.
 
    Desc: xHCI USB Controllers specific definitions
*/

#include <exec/types.h>

#define ALIGN_XHCI_16                           16
#define ALIGN_XHCI_32                           32
#define ALIGN_XHCI_64                           64

#define ALIGN_TRB                               ALIGN_XHCI_16
#define ALIGN_SLOT                              ALIGN_XHCI_32
#define ALIGN_SPBA                              ALIGN_XHCI_64
#define ALIGN_DCBAA                             ALIGN_XHCI_64
#define ALIGN_CMDRING_SEG                       ALIGN_XHCI_64
#define ALIGN_EVTRING_SEG                       ALIGN_XHCI_64
#define ALIGN_EVTRING_TBL                       ALIGN_XHCI_64
#define ALIGN_CTX                               ALIGN_XHCI_64

#define XHCI_SBRN                               0x60                    // (B) Serial Bus Release Number Register
#define XHCI_FLADJ                              0x61                    // (B) Frame Length Adjustment Register
#define XHCI_DBES                               0x62                    // (B) 4bit Default Best Effort Service Latency (DBESL)
                                                                        //     4bit Default Best Effort Service Latency Deep (DBESLD)

struct xhci_address {
    union {
        UQUAD                   addr64;
        struct {
            ULONG               addr_lo;
            ULONG               addr_hi;
        };
    };
}  __packed;


/*
 * Host Controller Capability Registers 
 * See eXtendible Host Controller Interface
 * for Universal Serial Bus (xHCI) Requirement Specification,
 * Chapter 5.3
 */
struct xhci_hccapr {
    UBYTE                       caplength;                                  // (B) Capability Register Length
    UBYTE                       rsvd1;
    UWORD                       hciversion;                                 // (W) Interface Version Number
    ULONG                       hcsparams1;                                 // (L) Structural Params 1
    ULONG                       hcsparams2;                                 // (L) Structural Params 2
    ULONG                       hcsparams3;                                 // (L) Structural Params 3
    ULONG                       hccparams1;                                 // (L) Capability Params 1
    ULONG                       dboff;                                      // (L) Doorbell Offset
    ULONG                       rrsoff;                                     // (L) Runtime Register Space Offset
    ULONG                       hccparams2;                                 // (L) Capability Params 2
}  __packed;

#define XHCIB_HCCPARAMS1_AC64                   0
#define XHCIF_HCCPARAMS1_AC64                   (1 << XHCIB_HCCPARAMS1_AC64)
#define XHCIB_HCCPARAMS1_BNC                    1
#define XHCIF_HCCPARAMS1_BNC                    (1 << XHCIB_HCCPARAMS1_BNC)
#define XHCIB_HCCPARAMS1_CSZ                    2
#define XHCIF_HCCPARAMS1_CSZ                    (1 << XHCIB_HCCPARAMS1_CSZ)
#define XHCIB_HCCPARAMS1_PPC                    3
#define XHCIF_HCCPARAMS1_PPC                    (1 << XHCIB_HCCPARAMS1_PPC)
#define XHCIS_HCCPARAMS1_ECP                    16
#define XHCI_HCCPARAMS1_ECP_SMASK               0xFFFF
#define XHCI_HCCPARAMS1_ECP_MASK                (XHCI_HCCPARAMS1_ECP_SMASK << XHCIS_HCCPARAMS1_ECP)

#define XHCIB_HCCPARAMS2_CPSM                   30
#define XHCIF_HCCPARAMS2_CPSM                   (1 << XHCIB_HCCPARAMS2_CPSM)
#define XHCIB_HCCPARAMS2_PRS                    31
#define XHCIF_HCCPARAMS2_PRS                    (1 << XHCIB_HCCPARAMS2_PRS)

/*
 * Host Controller Operational Registers
 * See eXtendible Host Controller Interface
 * for Universal Serial Bus (xHCI) Requirement Specification,
 * Chapter 5.4
 */
struct xhci_hcopr {
    ULONG                       usbcmd;                                     // USB Command
    ULONG                       usbsts;                                     // USB Status
    ULONG                       pagesize;                                   // Page Size
    ULONG                       rsvd1[2];
    ULONG                       dnctl;                                      // Device Notification Control
    // Command Ring Control Register
    struct xhci_address         crcr;
    ULONG                       rsvd2[4];
    // Device Context Base Address Array Pointer
    struct xhci_address         dcbaap;
    ULONG                       config;
}  __packed;

#define XHCIB_USBCMD_RS                         0
#define XHCIF_USBCMD_RS                         (1 << XHCIB_USBCMD_RS)
#define XHCIB_USBCMD_HCRST                      1
#define XHCIF_USBCMD_HCRST                      (1 << XHCIB_USBCMD_HCRST)
#define XHCIB_USBCMD_INTE                       2
#define XHCIF_USBCMD_INTE                       (1 << XHCIB_USBCMD_INTE)
#define XHCIB_USBCMD_HSEE                       3
#define XHCIF_USBCMD_HSEE                       (1 << XHCIB_USBCMD_HSEE)
#define XHCIB_USBCMD_LHCRST                     7
#define XHCIF_USBCMD_LHCRST                     (1 << XHCIB_USBCMD_LHCRST)
#define XHCIB_USBCMD_CSS                        8
#define XHCIF_USBCMD_CSS                        (1 << XHCIB_USBCMD_CSS)
#define XHCIB_USBCMD_CRS                        9
#define XHCIF_USBCMD_CRS                        (1 << XHCIB_USBCMD_CRS)
#define XHCIB_USBCMD_EWE                        10
#define XHCIF_USBCMD_EWE                        (1 << XHCIB_USBCMD_EWE)
#define XHCIB_USBCMD_EU3S                       11
#define XHCIF_USBCMD_EU3S                       (1 << XHCIB_USBCMD_EU3S)

#define XHCIB_USBSTS_HCH                        0
#define XHCIF_USBSTS_HCH                        (1 << XHCIB_USBSTS_HCH)
#define XHCIB_USBSTS_HSE                        2
#define XHCIF_USBSTS_HSE                        (1 << XHCIB_USBSTS_HSE)
#define XHCIB_USBSTS_EINT                       3
#define XHCIF_USBSTS_EINT                       (1 << XHCIB_USBSTS_EINT)
#define XHCIB_USBSTS_PCD                        4
#define XHCIF_USBSTS_PCD                        (1 << XHCIB_USBSTS_PCD)
#define XHCIB_USBSTS_SSS                        8
#define XHCIF_USBSTS_SSS                        (1 << XHCIB_USBSTS_SSS)
#define XHCIB_USBSTS_RSS                        9
#define XHCIF_USBSTS_RSS                        (1 << XHCIB_USBSTS_RSS)
#define XHCIB_USBSTS_SRE                        10
#define XHCIF_USBSTS_SRE                        (1 << XHCIB_USBSTS_SRE)
#define XHCIB_USBSTS_CNR                        11
#define XHCIF_USBSTS_CNR                        (1 << XHCIB_USBSTS_CNR)
#define XHCIB_USBSTS_HCE                        12
#define XHCIF_USBSTS_HCE                        (1 << XHCIB_USBSTS_HCE)

#define XHCIB_CRCR_RCS                          0
#define XHCIF_CRCR_RCS                          (1 << XHCIB_CRCR_RCS)
/*
 * Doorbell Registers
 */
struct xhci_dbr
{
    ULONG                       db;
}  __packed;

/*
 * Runtime Register Space
 */
struct xhci_rrs
{
    ULONG                       mfindex;
}  __packed;

/*
 * Port Registers
 * See eXtendible Host Controller Interface
 * for Universal Serial Bus (xHCI) Requirement Specification,
 * Chapter 5.4.8
 */
struct xhci_pr
{
    ULONG                       portsc;
    ULONG                       portpmsc;
    ULONG                       portli;
    ULONG                       porthlpmc;
}  __packed;

#define XHCIB_PR_PORTSC_CCS                     0
#define XHCIF_PR_PORTSC_CCS                     (1 << XHCIB_PR_PORTSC_CCS)
#define XHCIB_PR_PORTSC_PED                     1
#define XHCIF_PR_PORTSC_PED                     (1 << XHCIB_PR_PORTSC_PED)
#define XHCIB_PR_PORTSC_OCA                     3
#define XHCIF_PR_PORTSC_OCA                     (1 << XHCIB_PR_PORTSC_OCA)
#define XHCIB_PR_PORTSC_PR                      4
#define XHCIF_PR_PORTSC_PR                      (1 << XHCIB_PR_PORTSC_PR)
#define XHCIB_PR_PORTSC_PP                      9
#define XHCIF_PR_PORTSC_PP                      (1 << XHCIB_PR_PORTSC_PP)
#define XHCIB_PR_PORTSC_LWS                     16
#define XHCIF_PR_PORTSC_LWS                     (1 << XHCIB_PR_PORTSC_LWS)
#define XHCIB_PR_PORTSC_CSC                     17
#define XHCIF_PR_PORTSC_CSC                     (1 << XHCIB_PR_PORTSC_CSC)
#define XHCIB_PR_PORTSC_PEC                     18
#define XHCIF_PR_PORTSC_PEC                     (1 << XHCIB_PR_PORTSC_PEC)
#define XHCIB_PR_PORTSC_WRC                     19
#define XHCIF_PR_PORTSC_WRC                     (1 << XHCIB_PR_PORTSC_WRC)
#define XHCIB_PR_PORTSC_OCC                     20
#define XHCIF_PR_PORTSC_OCC                     (1 << XHCIB_PR_PORTSC_OCC)
#define XHCIB_PR_PORTSC_PRC                     21
#define XHCIF_PR_PORTSC_PRC                     (1 << XHCIB_PR_PORTSC_PRC)
#define XHCIB_PR_PORTSC_PLC                     22
#define XHCIF_PR_PORTSC_PLC                     (1 << XHCIB_PR_PORTSC_PLC)
#define XHCIB_PR_PORTSC_CEC                     23
#define XHCIF_PR_PORTSC_CEC                     (1 << XHCIB_PR_PORTSC_CEC)
#define XHCIB_PR_PORTSC_CAS                     24
#define XHCIF_PR_PORTSC_CAS                     (1 << XHCIB_PR_PORTSC_CAS)
#define XHCIB_PR_PORTSC_WCE                     25
#define XHCIF_PR_PORTSC_WCE                     (1 << XHCIB_PR_PORTSC_WCE)
#define XHCIB_PR_PORTSC_WDE                     26
#define XHCIF_PR_PORTSC_WDE                     (1 << XHCIB_PR_PORTSC_WDE)
#define XHCIB_PR_PORTSC_WOE                     27
#define XHCIF_PR_PORTSC_WOE                     (1 << XHCIB_PR_PORTSC_WOE)
#define XHCIB_PR_PORTSC_DR                      30
#define XHCIF_PR_PORTSC_DR                      (1 << XHCIB_PR_PORTSC_DR)
#define XHCIB_PR_PORTSC_WPR                     31
#define XHCIF_PR_PORTSC_WPR                     (1 << XHCIB_PR_PORTSC_WPR)
#define XHCIS_PR_PORTSC_PLS                     5
#define XHCI_PR_PORTSC_PLS_SMASK                0XF
#define XHCI_PR_PORTSC_PLS_MASK                 (XHCI_PR_PORTSC_PLS_SMASK << XHCIS_PR_PORTSC_PLS)
#define XHCIS_PR_PORTSC_SPEED                   10
#define XHCI_PR_PORTSC_SPEED_SMASK              0xF
#define XHCI_PR_PORTSC_SPEED_MASK               (XHCI_PR_PORTSC_SPEED_SMASK << XHCIS_PR_PORTSC_SPEED)
#define XHCIB_PR_PORTSC_FULLSPEED               1
#define XHCIF_PR_PORTSC_FULLSPEED               (XHCIB_PR_PORTSC_FULLSPEED << XHCIS_PR_PORTSC_SPEED)
#define XHCIB_PR_PORTSC_LOWSPEED                2
#define XHCIF_PR_PORTSC_LOWSPEED                (XHCIB_PR_PORTSC_LOWSPEED << XHCIS_PR_PORTSC_SPEED)
#define XHCIB_PR_PORTSC_HIGHSPEED               3
#define XHCIF_PR_PORTSC_HIGHSPEED               (XHCIB_PR_PORTSC_HIGHSPEED << XHCIS_PR_PORTSC_SPEED)
#define XHCIB_PR_PORTSC_SUPERSPEED              4
#define XHCIF_PR_PORTSC_SUPERSPEED              (XHCIB_PR_PORTSC_SUPERSPEED << XHCIS_PR_PORTSC_SPEED)
#define XHCIB_PR_PORTSC_SUPERSPEEDPLUS          5
#define XHCIF_PR_PORTSC_SUPERSPEEDPLUS          (XHCIB_PR_PORTSC_SUPERSPEEDPLUS << XHCIS_PR_PORTSC_SPEED)

/*
 * interrupter registers
 */
struct xhci_ir
{
    ULONG                       iman;
    ULONG                       imod;
    ULONG                       erstsz;
    ULONG                       rsvd1;
    // Command Ring Control Register
    struct xhci_address         erstba;
    struct xhci_address         erdp;
}  __packed;

#define XHCIB_IR_IMAN_IP                        0
#define XHCIF_IR_IMAN_IP                        (1 << XHCIB_IR_IMAN_IP)
#define XHCIB_IR_IMAN_IE                        1
#define XHCIF_IR_IMAN_IE                        (1 << XHCIB_IR_IMAN_IE)

#define XHCI_IR_ERDP_SI_MASK                    0x7
#define XHCIB_IR_ERDP_EHB                       3
#define XHCIF_IR_ERDP_EHB                       (1 << XHCIB_IR_ERDP_EHB)


/*
 * Extended Capabilities
 */

/* Extended Capability Header */
#define XHCIS_EXT_CAP_NEXT                      8
#define XHCI_EXT_CAP_NEXT_MASK                  0xFF

/* Extended Capability IDs */
#define XHCI_EXT_CAP_ID_LEGACY_SUPPORT          0x01
#define XHCI_EXT_CAP_ID_SUPPORTED_PROTOCOL      0x02

/*
 * USB Legacy Support Capability (USBLEGSUP)
 */
#define XHCIB_USBLEGSUP_BIOSOWNED               16
#define XHCIF_USBLEGSUP_BIOSOWNED               (1 << XHCIB_USBLEGSUP_BIOSOWNED)
#define XHCIB_USBLEGSUP_OSOWNED                 24
#define XHCIF_USBLEGSUP_OSOWNED                 (1 << XHCIB_USBLEGSUP_OSOWNED)

/* 
 * USB Legacy Support Control/Status (USBLEGCTLSTS)
 */
#define XHCIB_USBLEGCTLSTS_SMI_ENABLE           0
#define XHCIF_USBLEGCTLSTS_SMI_ENABLE           (1u << XHCIB_USBLEGCTLSTS_SMI_ENABLE)

#define XHCIB_USBLEGCTLSTS_SMI_ONHOSTERR_ENABLE 4
#define XHCIF_USBLEGCTLSTS_SMI_ONHOSTERR_ENABLE (1u << XHCIB_USBLEGCTLSTS_SMI_ONHOSTERR_ENABLE)

/*
 * Supported Protocol Capability
 */
#define XHCIS_XCP_NAMESTRING                    0
#define XHCI_XCP_NAMESTRING_MASK                0xFFFF
#define XHCIS_XCP_REV_MINOR                     16
#define XHCI_XCP_REV_MINOR_MASK                 0xFF
#define XHCIS_XCP_REV_MAJOR                     24
#define XHCI_XCP_REV_MAJOR_MASK                 0xFF

#define XHCIS_XCP_PORT_OFFSET                   0
#define XHCI_XCP_PORT_OFFSET_MASK               0xFF
#define XHCIS_XCP_PORT_COUNT                    8
#define XHCI_XCP_PORT_COUNT_MASK                0xFF

/*
 * Transfer Request Block
 * See eXtendible Host Controller Interface
 * for Universal Serial Bus (xHCI) Requirement Specification,
 * Chapter 6.3
 */
struct xhci_trb
{
    struct xhci_address         dbp;
    ULONG                       tparams;
    ULONG                       flags;
}  __packed;

#define TRBB_FLAG_C                             0
#define TRBF_FLAG_C                             (1 << TRBB_FLAG_C)
#define TRBB_FLAG_ENT                           1
#define TRBF_FLAG_ENT                           (1 << TRBB_FLAG_ENT)
/* Link TRBs reuse the ENT bit position as the Toggle Cycle flag */
#define TRBB_FLAG_TC                            TRBB_FLAG_ENT
#define TRBF_FLAG_TC                            TRBF_FLAG_ENT
#define TRBB_FLAG_ISP                           2
#define TRBF_FLAG_ISP                           (1 << TRBB_FLAG_ISP)
#define TRBB_FLAG_NS                            3
#define TRBF_FLAG_NS                            (1 << TRBB_FLAG_NS)
#define TRBB_FLAG_CH                            4
#define TRBF_FLAG_CH                            (1 << TRBB_FLAG_CH)
#define TRBB_FLAG_IOC                           5
#define TRBF_FLAG_IOC                           (1 << TRBB_FLAG_IOC)
#define TRBB_FLAG_IDT                           6
#define TRBF_FLAG_IDT                           (1 << TRBB_FLAG_IDT)
#define TRBB_FLAG_BEI                           9
#define TRBF_FLAG_BEI                           (1 << TRBB_FLAG_BEI)
#define TRBB_FLAG_SIA                           31
#define TRBF_FLAG_SIA                           (1u << TRBB_FLAG_SIA)

/* Transfer Burst Count (TBC) - 2 bits */
#define TRBS_FLAG_TBC                           16
#define TRB_FLAG_TBC_SMASK                      0x3
#define TRBF_FLAG_TBC(x)                        (((ULONG)(x) & TRB_FLAG_TBC_SMASK) << TRBS_FLAG_TBC)

/* Frame ID (11 bits) */
#define TRBS_FLAG_FRAMEID                       20
#define TRB_FLAG_FRAMEID_SMASK                  0x7FF
#define TRBF_FLAG_FRAMEID(x)                    (((ULONG)(x) & TRB_FLAG_FRAMEID_SMASK) << TRBS_FLAG_FRAMEID)

#define TRBS_FLAG_TYPE                          10
#define TRB_FLAG_TYPE_SMASK                     0x3F
#define TRB_FLAG_TYPE_MASK                      (TRB_FLAG_TYPE_SMASK << TRBS_FLAG_TYPE)

/* TRB Type Definitions
 * See eXtendible Host Controller Interface
 * for Universal Serial Bus (xHCI) Requirement Specification,
 * Table 6-91
 */
#define TRBB_FLAG_TYPE_RESERVED                 0
#define TRBF_FLAG_TYPE_RESERVED                 (TRBB_FLAG_TYPE_RESERVED << TRBS_FLAG_TYPE)
#define TRBB_FLAG_TRTYPE_NORMAL                 1
#define TRBF_FLAG_TRTYPE_NORMAL                 (TRBB_FLAG_TRTYPE_NORMAL << TRBS_FLAG_TYPE)
#define TRBB_FLAG_TRTYPE_SETUP                  2
#define TRBF_FLAG_TRTYPE_SETUP                  (TRBB_FLAG_TRTYPE_SETUP << TRBS_FLAG_TYPE)
#define TRBB_FLAG_TRTYPE_DATA                   3
#define TRBF_FLAG_TRTYPE_DATA                   (TRBB_FLAG_TRTYPE_DATA << TRBS_FLAG_TYPE)
#define TRBB_FLAG_TRTYPE_STATUS                 4
#define TRBF_FLAG_TRTYPE_STATUS                 (TRBB_FLAG_TRTYPE_STATUS << TRBS_FLAG_TYPE)
#define TRBB_FLAG_TRTYPE_ISOCH                  5
#define TRBF_FLAG_TRTYPE_ISOCH                  (TRBB_FLAG_TRTYPE_ISOCH << TRBS_FLAG_TYPE)
#define TRBB_FLAG_TRTYPE_LINK                   6
#define TRBF_FLAG_TRTYPE_LINK                   (TRBB_FLAG_TRTYPE_LINK << TRBS_FLAG_TYPE)
#define TRBB_FLAG_TRTYPE_EVDATA                 7
#define TRBF_FLAG_TRTYPE_EVDATA                 (TRBB_FLAG_TRTYPE_EVDATA << TRBS_FLAG_TYPE)
#define TRBB_FLAG_TRTYPE_NOOP                   8
#define TRBF_FLAG_TRTYPE_NOOP                   (TRBB_FLAG_TRTYPE_NOOP << TRBS_FLAG_TYPE)
#define TRBB_FLAG_CRTYPE_ENABLE_SLOT            9
#define TRBF_FLAG_CRTYPE_ENABLE_SLOT            (TRBB_FLAG_CRTYPE_ENABLE_SLOT << TRBS_FLAG_TYPE)
#define TRBB_FLAG_CRTYPE_DISABLE_SLOT           10
#define TRBF_FLAG_CRTYPE_DISABLE_SLOT           (TRBB_FLAG_CRTYPE_DISABLE_SLOT << TRBS_FLAG_TYPE)
#define TRBB_FLAG_CRTYPE_ADDRESS_DEVICE         11
#define TRBF_FLAG_CRTYPE_ADDRESS_DEVICE         (TRBB_FLAG_CRTYPE_ADDRESS_DEVICE << TRBS_FLAG_TYPE)
/*
 * Bit 9 is repurposed as BSR for ADDRESS_DEVICE command TRBs; it shares the
 * same bit position as BEI on transfer TRBs.
 */
#define TRBF_FLAG_ADDRDEV_BSR                   TRBF_FLAG_BEI
#define TRBB_FLAG_CRTYPE_CONFIGURE_ENDPOINT     12
#define TRBF_FLAG_CRTYPE_CONFIGURE_ENDPOINT     (TRBB_FLAG_CRTYPE_CONFIGURE_ENDPOINT << TRBS_FLAG_TYPE)
#define TRBB_FLAG_CRTYPE_EVALUATE_CONTEXT       13
#define TRBF_FLAG_CRTYPE_EVALUATE_CONTEXT       (TRBB_FLAG_CRTYPE_EVALUATE_CONTEXT << TRBS_FLAG_TYPE)
#define TRBB_FLAG_CRTYPE_RESET_ENDPOINT         14
#define TRBF_FLAG_CRTYPE_RESET_ENDPOINT         (TRBB_FLAG_CRTYPE_RESET_ENDPOINT << TRBS_FLAG_TYPE)
#define TRBB_FLAG_CRTYPE_STOP_ENDPOINT          15
#define TRBF_FLAG_CRTYPE_STOP_ENDPOINT          (TRBB_FLAG_CRTYPE_STOP_ENDPOINT << TRBS_FLAG_TYPE)
#define TRBB_FLAG_CRTYPE_SET_TR_DEQUEUE_PTR     16
#define TRBF_FLAG_CRTYPE_SET_TR_DEQUEUE_PTR     (TRBB_FLAG_CRTYPE_SET_TR_DEQUEUE_PTR << TRBS_FLAG_TYPE)
#define TRBB_FLAG_CRTYPE_RESET_DEVICE           17
#define TRBF_FLAG_CRTYPE_RESET_DEVICE           (TRBB_FLAG_CRTYPE_RESET_DEVICE << TRBS_FLAG_TYPE)
#define TRBB_FLAG_CRTYPE_FORCE_EVENT            18
#define TRBF_FLAG_CRTYPE_FORCE_EVENT            (TRBB_FLAG_CRTYPE_FORCE_EVENT << TRBS_FLAG_TYPE)
#define TRBB_FLAG_CRTYPE_NEGOTIATE_BANDWIDTH    19
#define TRBF_FLAG_CRTYPE_NEGOTIATE_BANDWIDTH    (TRBB_FLAG_CRTYPE_NEGOTIATE_BANDWIDTH << TRBS_FLAG_TYPE)
#define TRBB_FLAG_CRTYPE_SET_LATENCY_TOLERANCE  20
#define TRBF_FLAG_CRTYPE_SET_LATENCY_TOLERANCE  (TRBB_FLAG_CRTYPE_SET_LATENCY_TOLERANCE << TRBS_FLAG_TYPE)
#define TRBB_FLAG_CRTYPE_GET_PORT_BANDWIDTH     21
#define TRBF_FLAG_CRTYPE_GET_PORT_BANDWIDTH     (TRBB_FLAG_CRTYPE_GET_PORT_BANDWIDTH << TRBS_FLAG_TYPE)
#define TRBB_FLAG_CRTYPE_FORCE_HEADER           22
#define TRBF_FLAG_CRTYPE_FORCE_HEADER           (TRBB_FLAG_CRTYPE_FORCE_HEADER << TRBS_FLAG_TYPE)
#define TRBB_FLAG_CRTYPE_NOOP                   23
#define TRBF_FLAG_CRTYPE_NOOP                   (TRBB_FLAG_CRTYPE_NOOP << TRBS_FLAG_TYPE)

#define TRBB_FLAG_ERTYPE_TRANSFER               32
#define TRBF_FLAG_ERTYPE_TRANSFER               (TRBB_FLAG_ERTYPE_TRANSFER << TRBS_FLAG_TYPE)
#define TRBB_FLAG_ERTYPE_COMMAND_COMPLETE       33
#define TRBF_FLAG_ERTYPE_COMMAND_COMPLETE       (TRBB_FLAG_ERTYPE_COMMAND_COMPLETE << TRBS_FLAG_TYPE)
#define TRBB_FLAG_ERTYPE_PORT_STATUS_CHANGE     34
#define TRBF_FLAG_ERTYPE_PORT_STATUS_CHANGE     (TRBB_FLAG_ERTYPE_PORT_STATUS_CHANGE << TRBS_FLAG_TYPE)
#define TRBB_FLAG_ERTYPE_BANDWIDTH_REQUEST      35
#define TRBF_FLAG_ERTYPE_BANDWIDTH_REQUEST      (TRBB_FLAG_ERTYPE_BANDWIDTH_REQUEST << TRBS_FLAG_TYPE)
#define TRBB_FLAG_ERTYPE_DOORBELL               36
#define TRBF_FLAG_ERTYPE_DOORBELL               (TRBB_FLAG_ERTYPE_DOORBELL << TRBS_FLAG_TYPE)
#define TRBB_FLAG_ERTYPE_HOST_CONTROLLER        37
#define TRBF_FLAG_ERTYPE_HOST_CONTROLLER        (TRBB_FLAG_ERTYPE_HOST_CONTROLLER << TRBS_FLAG_TYPE)
#define TRBB_FLAG_ERTYPE_DEVICE_NOTIFICATION    38
#define TRBF_FLAG_ERTYPE_DEVICE_NOTIFICATION    (TRBB_FLAG_ERTYPE_DEVICE_NOTIFICATION << TRBS_FLAG_TYPE)
#define TRBB_FLAG_ERTYPE_MFINDEX_WRAP           39
#define TRBF_FLAG_ERTYPE_MFINDEX_WRAP           (TRBB_FLAG_ERTYPE_MFINDEX_WRAP << TRBS_FLAG_TYPE)

/* Data Stage TRB params & Flags */
#define TRBS_TPARAMS_DS_TRBLEN                  0
#define TRB_TPARAMS_DS_TRBLEN_SMASK             0x1FFFFu                                        /* 17 bits      */
#define TRBS_TPARAMS_DS_TDSIZE                  17
#define TRB_TPARAMS_DS_TDSIZE_SMASK             0x1Fu                                           /* 5 bits       */
#define TRB_TPARAMS_DS_TDSIZE_MASK              (TRB_TPARAMS_DS_TDSIZE_SMASK << TRBS_TPARAMS_DS_TDSIZE)
#define TRBB_FLAG_DS_DIR                        16
#define TRBF_FLAG_DS_DIR                        (1 << TRBB_FLAG_DS_DIR)

/*
 * Port Status Change TRB
 */
struct xhci_trb_port_status
{
    UBYTE                       rsvd1[3];
    UBYTE                       port;
    UBYTE                       rsvd2[7];
    UBYTE                       code;                                                           // Completion code
    UBYTE                       flags;                                                          // Flags
    UBYTE                       type;                                                           // Type
    UWORD                       rsvd3;
}  __packed;

/* completion event */
struct xhci_ce
{
    struct xhci_address         dbp;
    ULONG                       cparams;
    ULONG                       flags;
}  __packed;

/* TRB Completion Codes */
enum {
    TRB_CC_INVALID                        = 0,
    TRB_CC_SUCCESS                        = 1,
    TRB_CC_DATA_BUFFER_ERROR              = 2,
    TRB_CC_BABBLE_DETECTED_ERROR          = 3,
    TRB_CC_USB_TRANSACTION_ERROR          = 4,
    TRB_CC_TRB_ERROR                      = 5,
    TRB_CC_STALL_ERROR                    = 6,
    TRB_CC_RESOURCE_ERROR                 = 7,
    TRB_CC_BANDWIDTH_ERROR                = 8,
    TRB_CC_NO_SLOTS_AVAILABLE_ERROR       = 9,
    TRB_CC_INVALID_STREAM_TYPE_ERROR      = 10,
    TRB_CC_SLOT_NOT_ENABLED_ERROR         = 11,
    TRB_CC_ENDPOINT_NOT_ENABLED_ERROR     = 12,
    TRB_CC_SHORT_PACKET                   = 13,
    TRB_CC_RING_UNDERRUN                  = 14,
    TRB_CC_RING_OVERRUN                   = 15,
    TRB_CC_VF_EVENT_RING_FULL_ERROR       = 16,
    TRB_CC_PARAMETER_ERROR                = 17,
    TRB_CC_BANDWIDTH_OVERRUN_ERROR        = 18,
    TRB_CC_CONTEXT_STATE_ERROR            = 19,
    TRB_CC_NO_PING_RESPONSE_ERROR         = 20,
    TRB_CC_EVENT_RING_FULL_ERROR          = 21,
    TRB_CC_INCOMPATIBLE_DEVICE_ERROR      = 22,
    TRB_CC_MISSED_SERVICE_ERROR           = 23,
    TRB_CC_COMMAND_RING_STOPPED           = 24,
    TRB_CC_COMMAND_ABORTED                = 25,
    TRB_CC_STOPPED                        = 26,
    TRB_CC_STOPPED_LENGTH_INVALID         = 27,
    TRB_CC_STOPPED_SHORT_PACKET           = 28,
    TRB_CC_MAX_EXIT_LATENCY_TOO_LARGE     = 29,
    TRB_CC_ISOCH_BUFFER_OVERRUN           = 31,
    TRB_CC_EVENT_LOST                     = 32,
    TRB_CC_UNSUPPORTED_REQUEST_ERROR      = 33,
    TRB_CC_PREVIOUS_STREAM_STOPPED        = 34,
    TRB_CC_NEXT_STREAM_STOPPED            = 35,
    TRB_CC_STREAM_ID_ERROR                = 36,
    TRB_CC_SECONDARY_BANDWIDTH_ERROR      = 37,
    TRB_CC_SPLIT_TRANSACTION_ERROR        = 38,
    /*
     * Some host controllers report a distinct USB Disconnect completion
     * code when an endpoint stops because the device vanished. Map this
     * symbolic name to the generic USB Transaction Error value when the
     * explicit code is not defined in the specification so callers can
     * key on a stable identifier.
     */
    TRB_CC_USB_DISCONNECT_ERROR           = TRB_CC_USB_TRANSACTION_ERROR,
};

// Device related structures

/* slot context */
struct xhci_slot
{
    ULONG                       ctx[4];
    ULONG                       rsvd1[4];
}  __packed;


#define SLOTS_CTX_SPEED         20
#define SLOT_CTX_FULLSPEED      1
#define SLOTF_CTX_FULLSPEED     (SLOT_CTX_FULLSPEED << SLOTS_CTX_SPEED)
#define SLOT_CTX_LOWSPEED       2
#define SLOTF_CTX_LOWSPEED      (SLOT_CTX_LOWSPEED << SLOTS_CTX_SPEED)
#define SLOT_CTX_HIGHSPEED      3
#define SLOTF_CTX_HIGHSPEED     (SLOT_CTX_HIGHSPEED << SLOTS_CTX_SPEED)
#define SLOT_CTX_SUPERSPEED     4
#define SLOTF_CTX_SUPERSPEED    (SLOT_CTX_SUPERSPEED << SLOTS_CTX_SPEED)
#define SLOT_CTX_ROUTE_MASK     0xFFFFF
#define SLOTF_CTX_MTT           (1 << 25)
#define SLOT_CTX_TT_SLOT_SHIFT  0
#define SLOT_CTX_TT_PORT_SHIFT  8
#define SLOT_CTX_TTT_SHIFT      16

/* Slot Context DWORD3 fields (USB Device Address + Slot State) */
#define XHCI_SLOTCTX3_DEVADDR_SHIFT   0
#define XHCI_SLOTCTX3_DEVADDR_MASK    0xFFu
#define XHCI_SLOTCTX3_SLOTSTATE_SHIFT 27
#define XHCI_SLOTCTX3_SLOTSTATE_MASK  (0x1Fu << XHCI_SLOTCTX3_SLOTSTATE_SHIFT)

/*
 * Slot State field numeric encoding (Slot Context DW3[31:27]).
 *
 * The xHC maintains an internal "Enabled" slot state, but the Slot State
 * field does not encode it distinctly. A value of 0 means "Disabled or
 * Enabled" depending on controller internal state.
 *
 * Encoding:
 *   0 = Disabled or Enabled
 *   1 = Default
 *   2 = Addressed
 *   3 = Configured
 */
#define XHCI_SLOT_STATE_DISABLED_OR_ENABLED 0
#define XHCI_SLOT_STATE_DEFAULT             1
#define XHCI_SLOT_STATE_ADDRESSED           2
#define XHCI_SLOT_STATE_CONFIGURED          3

/* endpoint context */
struct xhci_ep
{
    ULONG                       ctx[2];
    struct xhci_address         deq;
    ULONG                       length;
    ULONG                       rsvd1[3];
}  __packed;

#define EPS_CTX_CERR            0   /* starting bit */
#define EP_CTX_CERR_MASK        0x3 /* value 3 => 3 retries */
#define EPS_CTX_TYPE            3
#define EPF_CTX_TYPE_ISOCH_O    (1 << EPS_CTX_TYPE)
#define EPF_CTX_TYPE_BULK_O     (2 << EPS_CTX_TYPE)
#define EPF_CTX_TYPE_INTR_O     (3 << EPS_CTX_TYPE)
#define EPF_CTX_TYPE_CONTROL    (4 << EPS_CTX_TYPE)
#define EPF_CTX_TYPE_ISOCH_I    (5 << EPS_CTX_TYPE)
#define EPF_CTX_TYPE_BULK_I     (6 << EPS_CTX_TYPE)
#define EPF_CTX_TYPE_INTR_I     (7 << EPS_CTX_TYPE)
#define EPS_CTX_PACKETMAX       16
#define EPS_CTX_MULT            8
#define EPF_CTX_MULT(x)         (((x) & 0x3) << EPS_CTX_MULT)

#define EPB_CTX_DEQ_DCS         0
#define EPF_CTX_DEQ_DCS         (1 << EPB_CTX_DEQ_DCS)

/* input context */
struct xhci_inctx
{
    ULONG                       dcf;
    ULONG                       acf;
    ULONG                       rsvd1[6];
}  __packed;

/* event ring segment */
struct xhci_er_seg {
    struct xhci_address         ptr;
    ULONG                       size;
    ULONG                       rsvd1;
} __packed;

#endif /* HARDWARE_XHCI_H */
