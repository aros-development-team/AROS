#ifndef IRDA_IRLAP_H
#define IRDA_IRLAP_H
/*
**	$VER: irlap.h 1.0 (23.04.05)
**
**	IrDA Link Access Protocol definitions include file
**
**	(C) Copyright 2005 Chris Hodges
**	    All Rights Reserved
*/

#if defined(__GNUC__)
# pragma pack(1)
#endif

/* LAP Address Flags */
#define LAM_ADDRESS     0xfe
#define LAS_ADDRESS     1
#define LAF_COMMAND     0x01
#define LAF_RESPONSE    0x00

/* LAP Control Flag */
#define LCF_POLLFINAL   0x10
#define LCF_SUPERVISORY 0x01
#define LCM_SUPERVISORY 0x03
#define LCF_UNNUMBERED  0x03
#define LCM_UNNUMBERED  0x03
#define LCF_INFO        0x00
#define LCM_INFO        0x01

#define LCM_NR          0xe0
#define LCS_NR          5
#define LCM_NS          0x0e
#define LCS_NS          1

#define UNNUMBERED_MASK 0xec     // 7  6  5  4  3  2  1  0
#define UCMD_SNRM       0x80     // 1  0  0  P  0  0  1  1    SNRM    command
#define UCMD_DISC       0x40     // 0  1  0  P  0  0  1  1    DISC   command
#define UCMD_UI         0x00     // 0  0  0  P  0  0  1  1    UI command
#define UCMD_XID        0x2c     // 0  0  1  P  1  1  1  1    XID  command
#define UCMD_TEST       0xe0     // 1  1  1  P  0  0  1  1    TEST   command
#define URSP_RNRM       0x80     // 1  0  0  F  0  0  1  1    RNRM    response
#define URSP_UA         0x30     // 0  1  1  F  0  0  1  1    UA  response
#define URSP_FRMR       0x84     // 1  0  0  F  0  1  1  1    FRMR    response
#define URSP_DM         0x0c     // 0  0  0  F  1  1  1  1    DM   response
#define URSP_RD         0x40     // 0  1  0  F  0  0  1  1    RD  response
#define URSP_UI         0x00     // 0  0  0  F  0  0  1  1    UI response
#define URSP_XID        0xac     // 1  0  1  F  1  1  1  1    XID  response
#define URSP_TEST       0xe0     // 1  1  1  F  0  0  1  1    TEST   response

#define SUPERVISORY_MASK 0x0c    // 7  6  5  4  3  2  1  0
#define SCR_RR          0x00     // Nr Nr Nr PF 0  0  0  1    RR command/response
#define SCR_RNR         0x04     // Nr Nr Nr PF 0  1  0  1    RNR  command/response
#define SCR_REJ         0x08     // Nr Nr Nr PF 1  0  0  1    REJ command/response
#define SCR_SREJ        0x0c     // Nr Nr Nr PF 1  1  0  1    SREJ  command/response

/* negotiation parameters */

#define PI_BAUDRATE      0x01    // Supported baudrates (1/2 bytes)
#define PI_MAXTURNAROUND 0x82    // Maximum Turnaround time
#define PI_DATASIZE      0x83    // Maximum frame size
#define PI_WINDOWSIZE    0x84    // Maximum window size
#define PI_ADDBOFS       0x85    // Number of additional BOFs at 115200bps
#define PI_MINTURNAROUND 0x86    // Minimum turnaround time
#define PI_LINKDCTHRESH  0x08    // Link disconnect/threshold time

/* baudrates */
#define BRF_2400         0x0001
#define BRF_9600         0x0002
#define BRF_19200        0x0004
#define BRF_38400        0x0008
#define BRF_57600        0x0010
#define BRF_115200       0x0020
#define BRF_576000       0x0040
#define BRF_1152000      0x0080
#define BRF_4000000      0x0100

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* BLUETOOTH_HCI_H */
