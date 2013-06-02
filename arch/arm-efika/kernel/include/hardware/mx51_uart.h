/*
 * mx51_uart.h
 *
 *  Created on: Jun 2, 2013
 *      Author: michal
 */

#ifndef HARDWARE_MX51_UART_H
#define HARDWARE_MX51_UART_H

#ifndef _INTTYPES_H
#include <inttypes.h>
#endif

#define UART1_BASE_ADDR     0x73FBC000
#define UART2_BASE_ADDR     0x73FC0000

#define UART_URXD           0x00
#define UART_UTXD           0x40
#define UART_UCR1           0x80
#define UART_UCR2           0x84
#define UART_UCR3           0x88
#define UART_UCR4           0x8C
#define UART_UFCR           0x90
#define UART_USR1           0x94
#define UART_USR2           0x98
#define UART_UESC           0x9C
#define UART_UTIM           0xA0
#define UART_UBIR           0xA4
#define UART_UBMR           0xA8
#define UART_UBRC           0xAC
#define UART_ONEMS          0xB0
#define UART_UTS            0xB4

typedef struct {
    uint32_t        URXD;
    uint32_t        __reserved_1[15];
    uint32_t        UTXD;
    uint32_t        __reserved_2[15];
    uint32_t        UCR1;
    uint32_t        UCR2;
    uint32_t        UCR3;
    uint32_t        UCR4;
    uint32_t        USR1;
    uint32_t        USR2;
    uint32_t        UESC;
    uint32_t        UTIM;
    uint32_t        UBIR;
    uint32_t        UBRC;
    uint32_t        ONEMS;
    uint32_t        UTS;
} MX51_UART;

#define UART_URXD_RX_DATA_MASK      0x000000FF
#define UART_URXD_RX_DATA(x)        ((x) & UART_URXD_RX_DATA_MASK)
#define UART_URXD_PRERR             0x00000400
#define UART_URXD_BRK               0x00000800
#define UART_URXD_FRMERR            0x00001000
#define UART_URXD_OVRRUN            0x00002000
#define UART_URXD_ERR               0x00004000
#define UART_URXD_CHARRDY           0x00008000

#define UART_UTXD_TX_DATA_MASK      0x000000FF
#define UART_UTXD_TX_DATA(x)        ((x) & UART_UTXD_TX_DATA_MASK)

#define UART_UCR1_UARTEN            0x00000001
#define UART_UCR1_DOZE              0x00000002
#define UART_UCR1_ATDMAEN           0x00000004
#define UART_UCR1_TXDMAEN           0x00000008
#define UART_UCR1_SNDBRK            0x00000010
#define UART_UCR1_RTSDEN            0x00000020
#define UART_UCR1_TXMPTYEN          0x00000040
#define UART_UCR1_IREN              0x00000080
#define UART_UCR1_RXDMAEN           0x00000100
#define UART_UCR1_RRDYEN            0x00000200
#define UART_UCR1_ICD_MASK          0x00000c00
#define UART_UCR1_ICD_4F            0x00000000
#define UART_UCR1_ICD_8F            0x00000400
#define UART_UCR1_ICD_16F           0x00000800
#define UART_UCR1_ICD_32F           0x00000c00
#define UART_UCR1_IDEN              0x00001000
#define UART_UCR1_TRDYEN            0x00002000
#define UART_UCR1_ADBR              0x00004000
#define UART_UCR1_ADEN              0x00008000

#define UART_UCR2_SRST              0x00000001
#define UART_UCR2_RXEN              0x00000002
#define UART_UCR2_TXEN              0x00000004
#define UART_UCR2_ATEN              0x00000008
#define UART_UCR2_RTSEN             0x00000010
#define UART_UCR2_WS                0x00000020
#define UART_UCR2_STPB              0x00000040
#define UART_UCR2_PROE              0x00000080
#define UART_UCR2_PREN              0x00000100
#define UART_UCR2_RTEC_MASK         0x00000600
#define UART_UCR2_RTEC_RISING       0x00000000
#define UART_UCR2_RTEC_FALLING      0x00000200
#define UART_UCR2_RTEC_ANY          0x00000400
#define UART_UCR2_ESCEN             0x00000800
#define UART_UCR2_CTS               0x00001000
#define UART_UCR2_CTSC              0x00002000
#define UART_UCR2_IRTS              0x00004000
#define UART_UCR2_ESCI              0x00008000

#define UART_UCR3_ACIEN             0x00000001
#define UART_UCR3_INVT              0x00000002
#define UART_UCR3_RXDMUXSEL         0x00000004
#define UART_UCR3_DTRDEN            0x00000008
#define UART_UCR3_AWAKEN            0x00000010
#define UART_UCR3_AIRINTEN          0x00000020
#define UART_UCR3_RXDSEN            0x00000040
#define UART_UCR3_ADNIMP            0x00000080
#define UART_UCR3_RI                0x00000100
#define UART_UCR3_DCD               0x00000200
#define UART_UCR3_DSR               0x00000400
#define UART_UCR3_FRAERREN          0x00000800
#define UART_UCR3_PARERREN          0x00001000
#define UART_UCR3_DTREN             0x00002000
#define UART_UCR3_DPEC_MASK         0x0000c000
#define UART_UCR3_DPEC_RISING       0x00000000
#define UART_UCR3_DPEC_FALLING      0x00004000
#define UART_UCR3_DPEC_ANY          0x00008000

#define UART_UCR4_DREN              0x00000001
#define UART_UCR4_OREN              0x00000002
#define UART_UCR4_BKEN              0x00000004
#define UART_UCR4_TCEN              0x00000008
#define UART_UCR4_LPBYP             0x00000010
#define UART_UCR4_IRSC              0x00000020
#define UART_UCR4_IDDMAEN           0x00000040
#define UART_UCR4_WKEN              0x00000080
#define UART_UCR4_ENIRI             0x00000100
#define UART_UCR4_INVR              0x00000200
#define UART_UCR4_CTSTL_MASK        0x0000fc00
#define UART_UCR4_CTSTL_SHIFT       10
#define UART_UCR4_CTSTL(x)          ((x) & UART_UCR4_CTSTL_MASK)

#define UART_UFCR_RXTL_MASK         0x0000003f
#define UART_UFCR_RXTL_SHIFT        0
#define UART_UFCR_RXTL(x)           ((x) & UART_UFCR_RXTL_MASK)
#define UART_UFCR_DCEDTE            0x00000040
#define UART_UFCR_RFDIV_MASK        0x00000380
#define UART_UFCR_RFDIV_SHIFT       7
#define UART_UFCR_RFDIV(x)          ((x) & UART_UFCR_RFDIV_MASK)
#define UART_UFCR_TXTL_MASK         0x0000fc00
#define UART_UFCR_TXTL_SHIFT        10
#define UART_UFCR_TXTL(x)           ((x) & UART_UFCR_TXTL_MASK)

#define UART_USR1_AWAKE             0x00000010
#define UART_USR1_AIRINT            0x00000020
#define UART_USR1_RXDS              0x00000040
#define UART_USR1_DTRD              0x00000080
#define UART_USR1_AGTIM             0x00000100
#define UART_USR1_RRDY              0x00000200
#define UART_USR1_FRAMERR           0x00000400
#define UART_USR1_ESCF              0x00000800
#define UART_USR1_RTSD              0x00001000
#define UART_USR1_TRDY              0x00002000
#define UART_USR1_RTSS              0x00004000
#define UART_USR1_PARITYERR         0x00008000

#define UART_USR2_RDR               0x00000001
#define UART_USR2_ORE               0x00000002
#define UART_USR2_BRCD              0x00000004
#define UART_USR2_TXDC              0x00000008
#define UART_USR2_RTSF              0x00000010
#define UART_USR2_DCDIN             0x00000020
#define UART_USR2_DCDDELT           0x00000040
#define UART_USR2_WAKE              0x00000080
#define UART_USR2_IRINT             0x00000100
#define UART_USR2_RIIN              0x00000200
#define UART_USR2_RIDELT            0x00000400
#define UART_USR2_ACST              0x00000800
#define UART_USR2_IDLE              0x00001000
#define UART_USR2_DTRF              0x00002000
#define UART_USR2_TXFE              0x00004000
#define UART_USR2_ADET              0x00008000

#define UART_UESC_ESC_CHAR_MASK     0x000000ff
#define UART_UESC_ESC_CHAR_SHIFT    0
#define UART_UESC_ESC_CHAR(x)       ((x) & UART_UESC_ESC_CHAR_MASK)

#define UART_UTIM_TIM_MASK          0x00000fff
#define UART_UTIM_TIM_SHIFT         0
#define UART_UTIM_TIM(x)            ((x) & UART_UTIM_TIM_MASK)

#define UART_UBIR_INC_MASK          0x0000ffff
#define UART_UBIR_INC_SHIFT         0
#define UART_UBIR_INC(x)            ((x) & UART_UBIR_INC_MASK)

#define UART_UBMR_MOD_MASK          0x0000ffff
#define UART_UBMR_MOD_SHIFT         0
#define UART_UBMR_MOD(x)            ((x) & UART_UBMR_MOD_MASK)

#define UART_UBRC_BCNT_MASK         0x0000ffff
#define UART_UBRC_BCNT_SHIFT        0
#define UART_UBRC_BCNT(x)           ((x) & UART_UBRC_BCNT_MASK)

#define UART_ONEMS_MASK             0x00ffffff
#define UART_ONEMS_SHIFT            0
#define UART_ONEMS(x)               ((x) & UART_ONEMS_MASK)

#define UART_UTS_SOFTRST            0x00000001
#define UART_UTS_RXFULL             0x00000008
#define UART_UTS_TXFULL             0x00000010
#define UART_UTS_RXEMPTY            0x00000020
#define UART_UTS_TXEMPTY            0x00000040
#define UART_UTS_RXDBG              0x00000200
#define UART_UTS_LOOPIR             0x00000400
#define UART_UTS_DBGEN              0x00000800
#define UART_UTS_LOOP               0x00001000
#define UART_UTS_FRCPERR            0x00002000

#endif /* HARDWARE_MX51_UART_H */
