#ifndef MAL_H_
#define MAL_H_

#include <inttypes.h>

struct EMACBase;

typedef struct {
    uint16_t    md_ctrl;
    uint16_t    md_length;
    char       *md_buffer;
} mal_descriptor_t;

typedef struct {
    mal_descriptor_t descr[4];
} mal_packet_t;

void EMAC_MAL_Init(struct EMACBase *EMACBase);

#define MAL_CTRL_TX_R   0x8000  /* Ready for transmission */
#define MAL_CTRL_TX_W   0x4000  /* Wrap */
#define MAL_CTRL_TX_CM  0x2000  /* Continous mode */
#define MAL_CTRL_TX_L   0x1000  /* Last */
#define MAL_CTRL_TX_I   0x0400  /* Interrupt */

#define EMAC_CTRL_TX_GFCS       0x0200
#define EMAC_CTRL_TX_GP         0x0100


#define MAL_CTRL_RX_E   0x8000  /* Empty */
#define MAL_CTRL_RX_W   0x4000  /* Wrap */
#define MAL_CTRL_RX_CM  0x2000  /* Continous mode */
#define MAL_CTRL_RX_L   0x1000  /* Last */
#define MAL_CTRL_RX_F   0x0800  /* First */
#define MAL_CTRL_RX_I   0x0400  /* Interrupt */

/* Configuration Reg  */
#define MAL_CR_MMSR     0x80000000
#define MAL_CR_PLBP_1   0x00400000   /* lowsest is 00 */
#define MAL_CR_PLBP_2   0x00800000
#define MAL_CR_PLBP_3   0x00C00000   /* highest       */
#define MAL_CR_GA       0x00200000
#define MAL_CR_OA       0x00100000
#define MAL_CR_PLBLE    0x00080000
#define MAL_CR_PLBLT_1  0x00040000
#define MAL_CR_PLBLT_2  0x00020000
#define MAL_CR_PLBLT_3  0x00010000
#define MAL_CR_PLBLT_4  0x00008000
#define MAL_CR_PLBLT_DEFAULT 0x00078000 /* ????? */
#define MAL_CR_PLBB     0x00004000
#define MAL_CR_OPBBL    0x00000080
#define MAL_CR_EOPIE    0x00000004
#define MAL_CR_LEA      0x00000002
#define MAL_CR_MSD      0x00000001

/* MAL DCR's */
#define MAL0_CFG        0x0180
#define MAL0_ESR        0x0181
#define MAL0_IER        0x0182
#define MAL0_TXCASR     0x0184
#define MAL0_TXCARR     0x0185
#define MAL0_TXEOBISR   0x0186
#define MAL0_TXDEIR     0x0187
#define MAL0_RXCASR     0x0190
#define MAL0_RXCARR     0x0191
#define MAL0_RXEOBISR   0x0192
#define MAL0_RXDEIR     0x0193
#define MAL0_TXCTP0R    0x01A0
#define MAL0_TXCTP1R    0x01A1
#define MAL0_TXCTP2R    0x01A2
#define MAL0_TXCTP3R    0x01A3
#define MAL0_RXCTP0R    0x01C0
#define MAL0_RXCTP1R    0x01C1
#define MAL0_RCBS0      0x01E0
#define MAL0_RCBS1      0x01E1

/* Configuration Reg  */
#define MAL_CR_MMSR       0x80000000
#define MAL_CR_PLBP_1     0x00400000   /* lowsest is 00 */
#define MAL_CR_PLBP_2     0x00800000
#define MAL_CR_PLBP_3     0x00C00000   /* highest       */
#define MAL_CR_GA         0x00200000
#define MAL_CR_OA         0x00100000
#define MAL_CR_PLBLE      0x00080000
#define MAL_CR_PLBLT_1  0x00040000
#define MAL_CR_PLBLT_2  0x00020000
#define MAL_CR_PLBLT_3  0x00010000
#define MAL_CR_PLBLT_4  0x00008000
#define MAL_CR_PLBLT_DEFAULT 0x00078000 /* ????? */
#define MAL_CR_PLBB       0x00004000
#define MAL_CR_OPBBL      0x00000080
#define MAL_CR_EOPIE      0x00000004
#define MAL_CR_LEA        0x00000002
#define MAL_CR_MSD        0x00000001

/* Error Status Reg    */
#define MAL_ESR_EVB       0x80000000
#define MAL_ESR_CID       0x40000000
#define MAL_ESR_DE        0x00100000
#define MAL_ESR_ONE       0x00080000
#define MAL_ESR_OTE       0x00040000
#define MAL_ESR_OSE       0x00020000
#define MAL_ESR_PEIN      0x00010000
   /* same bit position as the IER */
   /* VV                      VV   */
#define MAL_ESR_DEI       0x00000010
#define MAL_ESR_ONEI      0x00000008
#define MAL_ESR_OTEI      0x00000004
#define MAL_ESR_OSEI      0x00000002
#define MAL_ESR_PBEI      0x00000001
   /* ^^                      ^^   */
   /* Mal IER                      */
#define MAL_IER_DE        0x00000010
#define MAL_IER_NE        0x00000008
#define MAL_IER_TE        0x00000004
#define MAL_IER_OPBE      0x00000002
#define MAL_IER_PLBE      0x00000001

#define MAL_TXRX_CASR   (0x80000000)

#define MAL_TXRX_CASR_V(__x)  (__x)

#endif /*MAL_H_*/
