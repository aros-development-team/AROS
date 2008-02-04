#ifndef ASM_AMCC440_H
#define ASM_AMCC440_H

#include <inttypes.h>

typedef struct regs {
    uint32_t    gpr[32];
    uint32_t    srr0;
    uint32_t    srr1;
    uint32_t    ctr;
    uint32_t    lr;
    uint32_t    xer;
    uint32_t    dar;
    uint32_t    dsisr;
} regs_t;

/* DCR registers */

#define rddcr(reg) \
    ({ unsigned long val; asm volatile("mfdcr %0,%1":"=r"(val):"i"(reg)); val; })

#define wrdcr(reg, val) \
    do { asm volatile("mtdcr %0,%1"::"i"(reg),"r"(val)); } while(0)

/* System device control */
#define SDR0_CFGADDR    0x000E  /* R/W System DCR Configuration Address Register */
#define SDR0_CFGDATA    0x000F  /* R/W System DCR Configuration Data Register */
#define SDR0_SDSTP0     0x0020  /*  R  Serial Device Strap Register 0 */
#define SDR0_SDSTP1     0x0021  /*  R  Serial Device Strap Register 1 */
#define SDR0_PINSTP     0x0040  /*  R  Pin Strapping Register */
#define SDR0_SDCS0      0x0060  /* R/W Serial Device Controller Settings Register */
#define SDR0_ECID0      0x0080  /* R/W Electronic Chip ID Register 0 */
#define SDR0_ECID1      0x0081  /* R/W Electronic Chip ID Register 1 */
#define SDR0_ECID2      0x0082  /* R/W Electronic Chip ID Register 2 */
#define SDR0_ECID3      0x0083  /* R/W Electronic Chip ID Register 3 */
#define SDR0_JTAG       0x00C0  /* R/W JTAG ID Register */
#define SDR0_DDRDL0     0x00E0  /* R/W DDR Delay Line Register */
#define SDR0_EBC0       0x0100  /* R/W EBC Configuration Register */
#define SDR0_UART0      0x0120  /* R/W UART Configuration Register 0 */
#define SDR0_UART1      0x0121  /* R/W UART Configuration Register 1 */
#define SDR0_UART2      0x0122  /* R/W UART Configuration Register 2 */
#define SDR0_UART3      0x0123  /* R/W UART Configuration Register 3 */
#define SDR0_CP440      0x0180  /* R/W 440CPU Control Register */
#define SDR0_SRST0      0x0200  /* R/W Individual Core Reset Control Register 0 */
#define SDR0_SRST1      0x0201  /* R/W Individual Core Reset Control Register 1 */
#define SDR0_SLPIPE0    0x0220  /* R/W PLB Slave Address Pipeline Disabling Register */
#define SDR0_AMP0       0x0240  /* R/W Alternate PLB4 Master Priority Register */
#define SDR0_AMP1       0x0241  /* R/W Alternate PLB3 Master Priority Register */
#define SDR0_MIRQ0      0x0260  /* R/W Master Interrupt Request Register 0 (PLB3) */
#define SDR0_MALTBL     0x0280  /* R/W MAL Transmit Burst Length Register */
#define SDR0_MALRBL     0x02A0  /* R/W MAL Receive Burst Length Register */
#define SDR0_MALTBS     0x02C0  /* R/W Reserved */
#define SDR0_MALRBS     0x02E0  /* R/W Reserved */
#define SDR0_PCI0       0x0300  /* R/W PCI Control Register */
#define SDR0_USB0       0x0320  /*  R  Universal Serial Bus Register 0 */
#define SDR0_CUST0      0x4000  /* R/W Register0 Reserved for Customer Use */
#define SDR0_SDSTP2     0x4001  /*  R  Read Only Version of SDR0_CUST0 */
#define SDR0_CUST1      0x4002  /* R/W Register1 Reserved for Customer Use */
#define SDR0_SDSTP3     0x4003  /*  R  Read Only Version of SDR0_CUST1 */
#define SDR0_PFC0       0x4100  /* R/W Pin Function Control Register 0 */
#define SDR0_PFC1       0x4101  /* R/W Pin Function Control Register 1 */
#define SDR0_MFR        0x4300  /* R/W Miscellaneous Function Register */
#define SDR0_EMAC0RXST  0x4301  /* R/W EMAC0 RX Status Register */
#define SDR0_EMAC0TXST  0x4302  /* R/W EMAC0 TX Status Register */
#define SDR0_EMAC0REJCNT 0x4303 /*  R  EMAC0 RX Packet Reject Counter */
#define SDR0_EMAC1RXST  0x4304  /* R/W EMAC1 RX Status Register */
#define SDR0_EMAC1TXST  0x4305  /* R/W EMAC1 TX Status Register */
#define SDR0_EMAC1REJCNT 0x4306 /*  R  EMAC1 RX Packet Reject Counter */
#define SDR0_HSF        0x4400  /* R/W DDR Hardware Self Refresh Register */

/* Universal Interrupt Controller 0 */
#define UIC0_SR         0x00C0 /* R/Clear UIC 0 Status Register */
#define UIC0_SRS        0x00C1 /* W/Set   UIC 0 Status Register Set (reserved for debug only) */
#define UIC0_ER         0x00C2 /*  R/W    UIC 0 Enable Register */
#define UIC0_CR         0x00C3 /*  R/W    UIC 0 Critical Register */
#define UIC0_PR         0x00C4 /*  R/W    UIC 0 Polarity Register */
#define UIC0_TR         0x00C5 /*  R/W    UIC 0 Triggering Register */
#define UIC0_MSR        0x00C6 /*   R     UIC 0 Masked Status Register */
#define UIC0_VR         0x00C7 /*   R     UIC 0 Vector Register */
#define UIC0_VCR        0x00C8 /*   W     UIC 0 Vector Configuration Register */
/* Universal Interrupt Controller 1 */
#define UIC1_SR         0x00D0 /* R/Clear UIC 1 Status Register */
#define UIC1_SRS        0x00D1 /* W/Set   UIC 1 Status Register Set (reserved for debug only) */
#define UIC1_ER         0x00D2 /*  R/W    UIC 1 Enable Register */
#define UIC1_CR         0x00D3 /*  R/W    UIC 1 Critical Register */
#define UIC1_PR         0x00D4 /*  R/W    UIC 1 Polarity Register */
#define UIC1_TR         0x00D5 /*  R/W    UIC 1 Triggering Register */
#define UIC1_MSR        0x00D6 /*   R     UIC 1 Masked Status Register */
#define UIC1_VR         0x00D7 /*   R     UIC 1 Vector Register */
#define UIC1_VCR        0x00D8 /*   W     UIC 1 Vector Configuration Register */

/* UART registers */

/* UART0 */
#define UART0_RBR       0xEF600300
#define UART0_THR       0xEF600300
#define UART0_IER       0xEF600301
#define UART0_IIR       0xEF600302
#define UART0_FCR       0xEF600302
#define UART0_LCR       0xEF600303
#define UART0_MCR       0xEF600304
#define UART0_LSR       0xEF600305
#define UART0_MSR       0xEF600306
#define UART0_SCR       0xEF600307
#define UART0_DLL       0xEF600300
#define UART0_DLM       0xEF600301

/* UART1 */
#define UART1_RBR       0xEF600400
#define UART1_THR       0xEF600400
#define UART1_IER       0xEF600401
#define UART1_IIR       0xEF600402
#define UART1_FCR       0xEF600402
#define UART1_LCR       0xEF600403
#define UART1_MCR       0xEF600404
#define UART1_LSR       0xEF600405
#define UART1_MSR       0xEF600406
#define UART1_SCR       0xEF600407
#define UART1_DLL       0xEF600400
#define UART1_DLM       0xEF600401

/* UART2 */
#define UART2_RBR       0xEF600500
#define UART2_THR       0xEF600500
#define UART2_IER       0xEF600501
#define UART2_IIR       0xEF600502
#define UART2_FCR       0xEF600502
#define UART2_LCR       0xEF600503
#define UART2_MCR       0xEF600504
#define UART2_LSR       0xEF600505
#define UART2_MSR       0xEF600506
#define UART2_SCR       0xEF600507
#define UART2_DLL       0xEF600500
#define UART2_DLM       0xEF600501

/* UART3 */
#define UART3_RBR       0xEF600600
#define UART3_THR       0xEF600600
#define UART3_IER       0xEF600601
#define UART3_IIR       0xEF600602
#define UART3_FCR       0xEF600602
#define UART3_LCR       0xEF600603
#define UART3_MCR       0xEF600604
#define UART3_LSR       0xEF600605
#define UART3_MSR       0xEF600606
#define UART3_SCR       0xEF600607
#define UART3_DLL       0xEF600600
#define UART3_DLM       0xEF600601

#define UART_IER_EDSSI  0x08
#define UART_IER_ELSI   0x04
#define UART_IER_ETBEI  0x02
#define UART_IER_ERBFI  0x01

#define UART_IIR_FE             0xc0
#define UART_IIR_FE_ENABLED     0xc0
#define UART_IIR_FE_DISABLED    0x00
#define UART_IIR_INTID          0x0e
#define UART_IIR_INTID_4        0x00
#define UART_IIR_INTID_3        0x02
#define UART_IIR_INTID_2        0x04
#define UART_IIR_INITD_1        0x06
#define UART_IIR_INTID_0        0x0c
#define UART_IIR_INTP           0x01

#define UART_FCR_RFTL       0xc0
#define UART_FCR_RFTL_01    0x00
#define UART_FCR_RFTL_16    0x40
#define UART_FCR_RFTL_32    0x80
#define UART_FCR_RFTL_56    0xc0
#define UART_FCR_DMS        0x08
#define UART_FCR_TFR        0x04
#define UART_FCR_RFR        0x02
#define UART_FCR_FE         0x01

#define UART_LCR_DLAB       0x80
#define UART_LCR_SB         0x40
#define UART_LCR_SP         0x20
#define UART_LCR_EPS        0x10
#define UART_LCR_PEN        0x08
#define UART_LCR_SBS        0x04
#define UART_LCR_WLS        0x03
#define UART_LCR_WLS_5      0x00
#define UART_LCR_WLS_6      0x01
#define UART_LCR_WLS_7      0x02
#define UART_LCR_WLS_8      0x03

#define UART_MCR_AFC        0x20
#define UART_MCR_LOOP       0x10
#define UART_MCR_OUT2       0x08
#define UART_MCR_OUT1       0x04
#define UART_MCR_RTS        0x02
#define UART_MCR_DTR        0x01

#define UART_LSR_RFE    0x80
#define UART_LSR_TEMT   0x40
#define UART_LSR_THRE   0x20
#define UART_LSR_BI     0x10
#define UART_LSR_FE     0x08
#define UART_LSR_PE     0x04
#define UART_LSR_OE     0x02
#define UART_LSR_DR     0x01

#define UART_MSR_DCD    0x80
#define UART_LSR_RI     0x40
#define UART_LSR_DSR    0x20
#define UART_LSR_CTS    0x10
#define UART_LSR_DDCD   0x08
#define UART_LSR_TERI   0x04
#define UART_LSR_DDSR   0x02
#define UART_LSR_DCTS   0x01



#endif /*ASM_AMCC440_H*/
