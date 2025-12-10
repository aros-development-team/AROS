#ifndef ISP1760_H
#define ISP1760_H

/*
 *----------------------------------------------------------------------------
 *             Includes for EHCI USB Controller
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <hodges@in.tum.de>
 *
 * History
 *
 *  18-11-2005 - Initial
 *
 *
 */

#include <exec/types.h>
#include "hccommon.h"

/*
 * --------------------- EHCI registers ------------------------
 * Warning: These are LONG offsets!
 */

#define EHCI_CAPLENHCIVERS  0x000 /* Offset for operational registers (upper word, lower word HCIVERSION */
#define EHCI_HCSPARAMS      0x001 /* HC Structural Parameters */
#define EHCI_HCCPARAMS      0x002 /* HC Capability Parameters */
#define EHCI_USBCMD         0x008 /* USB Command (r/w) */
#define EHCI_USBSTS         0x009 /* USB Status (r/wc) */
#define EHCI_USBINTR        0x00a /* USB Interrupt request (r/w) */
#define EHCI_FRINDEX        0x00b /* Frame Number (r/w) */
#define EHCI_CONFIGFLAG     0x018 /* Configure flag (r/w) */
#define EHCI_PORTSC1        0x019 /* Port Status & Control 1 (r/w) */

/* ISP1760 specific stuff (based on offset, not caplength) */
#define ISP_ISOPTDDONEMAP   0x04c /* ISO PTD Done Map (rc) */
#define ISP_ISOPTDSKIPMAP   0x04d /* ISO PTD Skip Map (r/w) */
#define ISP_ISOPTDLASTPTD   0x04e /* ISO PTD Last PTD (Bit indicating last ISO transfer) (r/w) */
#define ISP_INTPTDDONEMAP   0x050 /* INT PTD Done Map (rc) */
#define ISP_INTPTDSKIPMAP   0x051 /* INT PTD Skip Map (r/w) */
#define ISP_INTPTDLASTPTD   0x052 /* INT PTD Last PTD (Bit indicating last INT transfer) (r/w) */
#define ISP_ATLPTDDONEMAP   0x054 /* ATL PTD Done Map (rc) */
#define ISP_ATLPTDSKIPMAP   0x055 /* ATL PTD Skip Map (r/w) */
#define ISP_ATLPTDLASTPTD   0x056 /* ATL PTD Last PTD (Bit indicating last ATL transfer) (r/w) */
#define ISP_DCMODE          0x083 /* Peripheral Mode (r/w) */
#define ISP_HWMODECTRL      0x0c0 /* HW Mode Control (r/w) */
#define ISP_CHIPID          0x0c1 /* ChipID = 0x00011760 (r) */
#define ISP_SCRATCH         0x0c2 /* Dummy (r/w) */
#define ISP_SWRESET         0x0c3 /* SW Reset (r/w) */
#define ISP_INTR            0x0c4 /* Interrupt pending (r/wc) */
#define ISP_INTEN           0x0c5
#define ISP_ISOIRQMASKOR    0x0c6
#define ISP_INTIRQMASKOR    0x0c7
#define ISP_ATLIRQMASKOR    0x0c8
#define ISP_ISOIRQMASKAND   0x0c9
#define ISP_INTIRQMASKAND   0x0ca
#define ISP_ATLIRQMASKAND   0x0cb
#define ISP_DMACONFIG       0x0cc /* DMA Configuration (r/w) */
#define ISP_BUFFERSTATUS    0x0cd /* Buffer Status (r/w) */
#define ISP_ATLDONETIMEOUT  0x0ce /* ATL Done Timeout in MS (r/w) */
#define ISP_MEMORY          0x0cf /* Memory start register for PIO reads */
#define ISP_EDGEINTCOUNT    0x0d0 /* Edge Interrupt Count (r/w) */
#define ISP_DMASTART        0x0d1 /* DMA Start Address (w) */
#define ISP_PWDOWNCTRL      0x0d5 /* Power Down Control (r/w) */
#define ISP_OTGCTRL         0x0dd /* OTG Control register (r/w) */
#define ISP_OTGSTATUS       0x0de /* OTG Status register (ro, low 16 bits) */

/* ISP_HWMODE_CTRL defines */
#define IHWCB_GLOBAL_INT_EN  0    /* enable interrupts */
#define IHWCB_INT_LVL        1    /* 0=level int, 1=edge int */
#define IHWCB_INT_POL        2    /* 1=INT HIGH active */
#define IHWCB_DREQ_POL       5    /* 1=DREQ output HIGH active */
#define IHWCB_DACK_POL       6    /* 1=DACK input HIGH active */
#define IHWCB_DATABUS_WIDTH  8    /* 0=16 Bit, 1=32 Bit */
#define IHWCB_ANALOG_OC     15    /* Analog overcurrent */
#define IHWCB_ALL_ATX_RST   31    /* Debugging only */

#define IHWCF_GLOBAL_INT_EN (1UL<<IHWCB_GLOBAL_INT_EN)
#define IHWCF_INT_LVL       (1UL<<IHWCB_INT_LVL)
#define IHWCF_INT_POL       (1UL<<IHWCB_INT_POL)
#define IHWCF_DREQ_POL      (1UL<<IHWCB_DREQ_POL)
#define IHWCF_DACK_POL      (1UL<<IHWCB_DACK_POL)
#define IHWCF_DATABUS_WIDTH (1UL<<IHWCB_DATABUS_WIDTH)
#define IHWCF_ANALOG_OC     (1UL<<IHWCB_ANALOG_OC)
#define IHWCF_ALL_ATX_RST   (1UL<<IHWCB_ALL_ATX_RST)

#define IHWCF_DENEB         (IHWCF_DATABUS_WIDTH|IHWCF_INT_POL|IHWCF_ANALOG_OC|(1UL<<10))

/* ISP_SWRESET defines */
#define ISWRB_RESET_ALL      0    /* Reset all the HC and CPU regs */
#define ISWRB_RESET_HC       1    /* Reset only HC registers (below 0x300) */

#define ISWRF_RESET_ALL     (1UL<<ISWRB_RESET_ALL)
#define ISWRF_RESET_HC      (1UL<<ISWRB_RESET_HC)

/* ISP_INTR and ISP_INTEN defines */
#define IINTB_SOF            1    /* Start of Frame */
#define IINTB_DMA_DONE       3    /* DMA transfer complete */
#define IINTB_HC_SUSP        5    /* Host Controller Suspend */
#define IINTB_CLKREADY       6    /* Clock Ready */
#define IINTB_INT_DONE       7    /* INT PTD completed */
#define IINTB_ATL_DONE       8    /* ATL PTD completed */
#define IINTB_ISO_DONE       9    /* ISO PTD completed */

#define IINTF_SOF           (1UL<<IINTB_SOF)
#define IINTF_DMA_DONE      (1UL<<IINTB_DMA_DONE)
#define IINTF_HC_SUSP       (1UL<<IINTB_HC_SUSP)
#define IINTF_CLKREADY      (1UL<<IINTB_CLKREADY)
#define IINTF_INT_DONE      (1UL<<IINTB_INT_DONE)
#define IINTF_ATL_DONE      (1UL<<IINTB_ATL_DONE)
#define IINTF_ISO_DONE      (1UL<<IINTB_ISO_DONE)

#define IINTM_ALL (IINTF_SOF|IINTF_DMA_DONE|IINTF_HC_SUSP|IINTF_CLKREADY|IINTF_INT_DONE|IINTF_ATL_DONE|IINTF_ISO_DONE)

/* Memory Map (Long word offsets) for CPU/DMA access */
#ifndef ZORRO_II
#define MMAP_BANK0          0x0000 // long offset into bank 0 (A16:17 = 00)
#define MMAP_BANK1          0x4000 // long offset into bank 1 (A16:17 = 01)
#define MMAP_BANK2          0x8000 // long offset into bank 2 (A16:17 = 10)
#define MMAP_BANK3          0xc000 // long offset into bank 3 (A16:17 = 11)
#endif

#define MMAP_ISO_PTDS       0x0100
#define MMAP_INT_PTDS       0x0200
#define MMAP_ATL_PTDS       0x0300
#define MMAP_PAYLOAD        0x0400

#if 0

#define MMAP_LARGEAREA      0x0400 // 25*1 KB for larger transfers (dynamically allocated)
#define MMAP_SETUPAREA      0x1D00 // 32*8 bytes for setup data (PTD to memory mapping)
#define MMAP_SMALLINT       0x1D40 // 32*16 bytes for small interrupt packets (PTD to memory mapping)
#define MMAP_SMALLATL       0x1E00 // 32*64 bytes (2KB) for small data transfers (PTD to memory mapping)
#define MMAP_BULKDMA_LOW    0x2000 // 16 KB low buffer for large bulk transfers (static)
#define MMAP_BULKDMA_HIGH   0x3000 // 16 KB high buffer for large bulk transfers (static)
#define MMAP_END            0x4000

#define MMAP_GRANULE_SIZE     1024 // size of a memory granule in large area
#define MMAP_GRANULE_BITS       10
#define MMAP_TOTAL_GRANULES     25
#define MMAP_MAX_GRANULES        8
#define MMAP_SMALLINT_SIZE      16 // in bytes
#define MMAP_SMALLINT_BITS       4
#define MMAP_SMALLATL_SIZE      64 // in bytes
#define MMAP_SMALLATL_BITS       6

#define MMAP_BULKDMA_SIZE    16384 // in bytes
#define MMAP_DMATHRES_SIZE     256 // DMA needs at least 256 bytes, otherwise use PIO.

#else

#define MMAP_LARGEAREA      0x0400 // 9*1 KB for larger transfers (dynamically allocated)
#define MMAP_SETUPAREA      0x0D00 // 32*8 bytes for setup data (PTD to memory mapping)
#define MMAP_SMALLINT       0x0D40 // 32*16 bytes for small interrupt packets (PTD to memory mapping)
#define MMAP_SMALLATL       0x0E00 // 32*64 bytes (2KB) for small data transfers (PTD to memory mapping)
#define MMAP_BULKDMA_LOW    0x1000 // 24 KB low buffer for large bulk transfers (static)
#define MMAP_BULKDMA_HIGH   0x2800 // 24 KB high buffer for large bulk transfers (static)
#define MMAP_END            0x4000

#define MMAP_GRANULE_SIZE     1024 // size of a memory granule in large area
#define MMAP_GRANULE_BITS       10
#define MMAP_TOTAL_GRANULES      9
#define MMAP_MAX_GRANULES        3
#define MMAP_SMALLINT_SIZE      16 // in bytes
#define MMAP_SMALLINT_BITS       4
#define MMAP_SMALLATL_SIZE      64 // in bytes
#define MMAP_SMALLATL_BITS       6

#define MMAP_BULKDMA_SIZE    24576 // in bytes
#define MMAP_DMATHRES_SIZE     256 // DMA needs at least 256 bytes, otherwise use PIO.

#endif

#define MMAP2IMAP(x) (((x) - MMAP_ISO_PTDS)>>1)
#define IMAP2MMAP(x) (((x)<<1) + MMAP_ISO_PTDS)

/* Memory Map (8 byte DW) for ISP internal addressing */
#define IMAP_ISO_PTDS       MMAP2IMAP(MMAP_ISO_PTDS)
#define IMAP_INT_PTDS       MMAP2IMAP(MMAP_INT_PTDS)
#define IMAP_ATL_PTDS       MMAP2IMAP(MMAP_ATL_PTDS)
#define IMAP_PAYLOAD        MMAP2IMAP(MMAP_PAYLOAD)
#define IMAP_LARGEDATA      MMAP2IMAP(MMAP_LARGEDATA)
#define IMAP_SETUPAREA      MMAP2IMAP(MMAP_SETUPAREA)
#define IMAP_SMALLINT       MMAP2IMAP(MMAP_SMALLINT)
#define IMAP_SMALLATL       MMAP2IMAP(MMAP_SMALLATL)
#define IMAP_BULKDMA        MMAP2IMAP(MMAP_BULKDMA)
#define IMAP_BULKDMA_LOW    MMAP2IMAP(MMAP_BULKDMA_LOW)
#define IMAP_BULKDMA_HIGH   MMAP2IMAP(MMAP_BULKDMA_HIGH)

/* ISP_DMACONFIG defines */
#define IDMAB_WRITETORAM     0    /* 0=Copy from Zorro Memory into ISP, 1=Copy from ISP into Zorro Memory */
#define IDMAB_ENABLE         1    /* 1=Enable DMA */
#define IDMAS_LENGTH         8    /* DMA length (in bytes) */

#define IDMAF_WRITETORAM    (1UL<<IDMAB_WRITETORAM)
#define IDMAF_ENABLE        (1UL<<IDMAB_ENABLE)
#define IDMAF_BURST1        0x00  /* Single DMA burst */
#define IDMAF_BURST4        0x04  /* 4-cycle DMA burst */
#define IDMAF_BURST8        0x08  /* 8-cycle DMA burst */
#define IDMAF_BURST16       0x0c  /* 16-cycle DMA burst */
#define IDMAM_LENGTH        (((1UL<<24)-1)<<IDMAS_LENGTH)

/* ISP_BUFFERSTATUS defines */
#define IBSB_ATL_ENABLE      0    /* At least one ATL PTD is available */
#define IBSB_INT_ENABLE      1    /* At least one INT PTD is available */
#define IBSB_ISO_ENABLE      2    /* At least one ISO PTD is available */

#define IBSF_ATL_ENABLE     (1UL<<IBSB_ATL_ENABLE)
#define IBSF_INT_ENABLE     (1UL<<IBSB_INT_ENABLE)
#define IBSF_ISO_ENABLE     (1UL<<IBSB_ISO_ENABLE)

/* ISP_MEMORY defines */
#define IMSF_BANK0          (0UL<<16)
#define IMSF_BANK1          (1UL<<16)
#define IMSF_BANK2          (2UL<<16)
#define IMSF_BANK3          (3UL<<16)

/* ISP_PWDOWNCTRL defines */
#define IPDCB_HC_CLK_EN      0    /* Host Controller Clock Enabled during suspend */
#define IPDCB_OC1_PWR        1    /* OC detection disabled during suspend */
#define IPDCB_OC2_PWR        2    /* OC detection disabled during suspend */
#define IPDCB_OC3_PWR        3    /* OC detection disabled during suspend */
#define IPDCB_VREG_ON        4    /* Vreg powered during suspend */
#define IPDCB_BIASEN         5    /* Internal bias circuits powered during suspend */
#define IPDCB_VBATDET_PWR   10    /* Vbat Detector is not powered during suspend */
#define IPDCB_PORT2_PD      11    /* Port 2 pull down are not connected during suspend */
#define IPDCB_PORT3_PD      12    /* Port 3 pull down are not connected during suspend */
#define IPDCS_CLOCKOFFCNT   16    /* Time before going to into suspend (0x3e8 default) */

#define IPDCF_HC_CLK_EN     (1UL<<IPDCB_HC_CLK_EN)
#define IPDCF_OC1_PWR       (1UL<<IPDCB_OC1_PWR)
#define IPDCF_OC2_PWR       (1UL<<IPDCB_OC2_PWR)
#define IPDCF_OC3_PWR       (1UL<<IPDCB_OC3_PWR)
#define IPDCF_VREG_ON       (1UL<<IPDCB_VREG_ON)
#define IPDCF_BIASEN        (1UL<<IPDCB_BIASEN)
#define IPDCF_VBATDET_PWR   (1UL<<IPDCB_VBATDET_PWR)
#define IPDCF_PORT2_PD      (1UL<<IPDCB_PORT2_PD)
#define IPDCF_PORT3_PD      (1UL<<IPDCB_PORT3_PD)

#define IPDCM_CLOCKOFFCNT   (((1UL<<16)-1)<<IPDCS_CLOCKOFFCNT)

/* ISP_OTGCTRL defines */
#define OTGCB_SET            0    /* OTG Control Set (lower word) */
#define OTGCB_CLR           16    /* OTG Control Clear (upper word) */
#define OTGCB_OTG_DISABLE   10    /* 0 - OTG functionality enabled, 1 - OTG disabled; pure host or peripheral */
#define OTGCB_OTG_SE0_EN     9    /* 0 - No SE0 sent on remote connect detection, 1 - SE0 (bus reset) sent on remote connect detection */
#define OTGCB_BDIS_ACON_EN   8    /* Enables the A-device to connect if the B-device disconnect is detected */
#define OTGCB_SW_SEL_HC_DC   7    /* 0 - Host controller connected to ATX, 1 - Peripheral controller connected to ATX */
#define OTGCB_VBUS_CHRG      6    /* Connect VBUS to VCC(I/O) through a resistor */
#define OTGCB_VBUS_DISCHRG   5    /* Discharge VBUS to ground through a resistor */
#define OTGCB_VBUS_DRV       4    /* Drive VBUS to 5 V using the charge pump */
#define OTGCB_SEL_CP_EXT     3    /* 0 - Internal charge pump selected, 1 - External charge pump selected */
#define OTGCB_DM_PULLDOWN    2    /* DM pull-down: 0 - Disable, 1 - Enable */
#define OTGCB_DP_PULLDOWN    1    /* DP pull-down: 0 - Disable, 1 - Enable */
#define OTGCB_DP_PULLUP      0    /* 0 - no pull-up resistor, 1 - internal 1.5k pull-up resistor is present */

#define OTGCF_OTG_RESERVED	(0xF800UL) /* bits 15 to 11 - reserved for future use */
#define OTGCF_OTG_CLR_MASK  (~OTGCF_OTG_RESERVED << OTGCB_CLR)
#define OTGCF_OTG_SET_MASK  (~OTGCF_OTG_RESERVED << OTGCB_SET)
#define OTGCF_OTG_DISABLE   (1UL<<OTGCB_OTG_DISABLE)
#define OTGCF_OTG_SE0_EN    (1UL<<OTGCB_OTG_SE0_EN)
#define OTGCF_BDIS_ACON_EN  (1UL<<OTGCB_BDIS_ACON_EN)
#define OTGCF_SW_SEL_HC_DC  (1UL<<OTGCB_SW_SEL_HC_DC)
#define OTGCF_VBUS_CHRG     (1UL<<OTGCB_VBUS_CHRG)
#define OTGCF_VBUS_DISCHRG  (1UL<<OTGCB_VBUS_DISCHRG)
#define OTGCF_VBUS_DRV      (1UL<<OTGCB_VBUS_DRV)
#define OTGCF_SEL_CP_EXT    (1UL<<OTGCB_SEL_CP_EXT)
#define OTGCF_DM_PULLDOWN   (1UL<<OTGCB_DM_PULLDOWN)
#define OTGCF_DP_PULLDOWN   (1UL<<OTGCB_DP_PULLDOWN)
#define OTGCF_DP_PULLUP     (1UL<<OTGCB_DP_PULLUP)

/* OTG Control - Force HOST mode (set) */
#define OTGCF_FORCE_HOST    (OTGCF_OTG_DISABLE | OTGCF_VBUS_DRV | OTGCF_DM_PULLDOWN | OTGCF_DP_PULLDOWN)

/* EHCI_USBCMD defines */
#define EHCB_RUNSTOP         0    /* 1=Run, 0=Stop */
#define EHCB_HCRESET         1    /* Host Controller Reset */
#define EHCB_LIGHTHCRESET    7    /* Light Host Controller Reset */
#define EHCS_INTTHRESHOLD   16    /* Interrupt threshold control */

#define EHCF_RUNSTOP        (1UL<<EHCB_RUNSTOP)
#define EHCF_HCRESET        (1UL<<EHCB_HCRESET)
#define EHCF_LIGHTHCRESET   (1UL<<EHCB_LIGHTHCRESET)
#define EHCM_INTTHRESHOLD   (((1UL<<8)-1)<<EHCS_INTTHRESHOLD)

/* EHCI_USBSTS defines */
#define EHSB_TDDONE          0    /* Transfer descriptor done */
#define EHSB_PORTCHANGED     2    /* Port Change detected */
#define EHSB_FRAMECOUNTOVER  3    /* Frame List Rollover */

#define EHSF_TDDONE         (1UL<<EHSB_TDDONE)
#define EHSF_PORTCHANGED    (1UL<<EHSB_PORTCHANGED)
#define EHSF_FRAMECOUNTOVER (1UL<<EHSB_FRAMECOUNTOVER)

/* EHCI_CONFIGFLAG defines */
#define EHCB_CONFIGURED      0
#define EHCF_CONFIGURED     (1UL<<EHCB_CONFIGURED)

/* EHCI_PORTSC defines */
#define EHPB_PORTCONNECTED   0    /* Port Connection status */
#define EHPB_CONNECTCHANGE   1    /* Port Connection change */
#define EHPB_PORTENABLE      2    /* Enable Port */
#define EHPB_RESUMEDTX       6    /* Resume detected */
#define EHPB_PORTSUSPENDED   7    /* Port is suspended */
#define EHPB_PORTRESET       8    /* Port is in reset state */
#define EHPB_LINESTATUS_DM  10    /* Line Status D- */
#define EHPB_LINESTATUS_DP  11    /* Line Stauts D+ */
#define EHPB_PORTPOWER      12    /* Depends on PortPowerControl */
#define EHPB_NOTPORTOWNER   13    /* Inverse of CONFIGURED (0=Owner) */

#define EHPF_PORTCONNECTED  (1UL<<EHPB_PORTCONNECTED)
#define EHPF_CONNECTCHANGE  (1UL<<EHPB_CONNECTCHANGE)
#define EHPF_PORTENABLE     (1UL<<EHPB_PORTENABLE)
#define EHPF_RESUMEDTX      (1UL<<EHPB_RESUMEDTX)
#define EHPF_PORTSUSPENDED  (1UL<<EHPB_PORTSUSPENDED)
#define EHPF_PORTRESET      (1UL<<EHPB_PORTRESET)
#define EHPF_LINESTATUS_DM  (1UL<<EHPB_LINESTATUS_DM)
#define EHPF_LINESTATUS_DP  (1UL<<EHPB_LINESTATUS_DP)
#define EHPF_PORTPOWER      (1UL<<EHPB_PORTPOWER)
#define EHPF_NOTPORTOWNER   (1UL<<EHPB_NOTPORTOWNER)

/* Queue Head Asynchonous (QHA) */

/* DW 0 */
#define QHA0B_VALID          0    /* PTD is valid */
#define QHA0S_TRANSLEN       3    /* Number of bytes to transferred (32KB max) */
#define QHA0S_MAXPKTLEN     18    /* Max Packet Length 0-2047 */
#define QHA0S_MULTIPLIER    29    /* Multiplier, how many successive packets are sent */
#define QHA0B_ENDPOINT0     31    /* Endpoint bit 0 */

#define QHA0F_VALID         (1UL<<QHA0B_VALID)
#define QHA0F_MULTI_1       (1UL<<QHA0S_MULTIPLIER)
#define QHA0F_MULTI_2       (2UL<<QHA0S_MULTIPLIER)
#define QHA0F_MULTI_3       (3UL<<QHA0S_MULTIPLIER)

#define QHA0F_DATA0         (1UL<<QHA0S_MULTIPLIER)
#define QHA0F_DATA1         (2UL<<QHA0S_MULTIPLIER)
#define QHA0F_DATA2         (3UL<<QHA0S_MULTIPLIER)

#define QHA0F_ENDPOINT0     (1UL<<QHA0B_ENDPOINT0)

#define QHA0M_TRANSLEN      (((1UL<<15)-1)<<QHA0S_TRANSLEN)
#define QHA0M_MAXPKTLEN     (((1UL<<11)-1)<<QHA0S_MAXPKTLEN)
#define QHA0M_MULTIPLIER    (((1UL<<2)-1)<<QHA0S_MULTIPLIER)

/* DW 1 */
#define QHA1S_ENDPOINT1_3    0    /* Endpoint bits 1-3 */
#define QHA1S_DEVADDR        3    /* Device Address */
#define QHA1S_TOKEN         10    /* 00=OUT, 01=IN, 10=SETUP */
#define QHA1S_TRANSTYPE     12    /* 00=Control, 10=Bulk, 11=Iso, 01=Int */
#define QHA1B_SPLITTRANS    14    /* Split Transaction */
#define QHA1B_LOWSPEED      17    /* Low Speed for Split Transaction */
#define QHA1S_PORTNUMBER    18    /* Port Number of hub for Split Transaction */
#define QHA1S_HUBADDRESS    25    /* Hub Device Address for Split Transaction */

#define QHA1F_TOKEN_OUT     (0UL<<QHA1S_TOKEN)
#define QHA1F_TOKEN_IN      (1UL<<QHA1S_TOKEN)
#define QHA1F_TOKEN_SETUP   (2UL<<QHA1S_TOKEN)
#define QHA1F_TOKEN_PING    (3UL<<QHA1S_TOKEN)

#define QHA1F_TT_CONTROL    (0UL<<QHA1S_TRANSTYPE)
#define QHA1F_TT_ISO        (1UL<<QHA1S_TRANSTYPE)
#define QHA1F_TT_BULK       (2UL<<QHA1S_TRANSTYPE)
#define QHA1F_TT_INTERRUPT  (3UL<<QHA1S_TRANSTYPE)

#define QHA1M_ENDPOINT1_3   (((1UL<<3)-1)<<QHA1S_ENDPOINT1_3)
#define QHA1M_DEVADDR       (((1UL<<7)-1)<<QHA1S_DEVADDR)
#define QHA1M_TOKEN         (((1UL<<2)-1)<<QHA1S_TOKEN)
#define QHA1M_TRANSTYPE     (((1UL<<2)-1)<<QHA1S_TRANSTYPE)

#define QHA1F_SPLITTRANS    (1UL<<QHA1B_SPLITTRANS)
#define QHA1F_LOWSPEED      (1UL<<QHA1B_LOWSPEED)

/* DW 2 */
#define QHA2S_MUFRAME        0    /* µFrame for ISO sending, for INT it's the ms interval - 1 (up to 32 ms) */
#define QHA2S_DATAADDR       8    /* Data Start Address (cpuaddr - 0x400)>>3 */
#define QHA2S_RELOAD        25    /* 0=Ignore NAKCOUNT, NAKCOUNT is loaded with this */

#define QHA2M_MUFRAME       (((1UL<<8)-1)<<QHA2S_MUFRAME)
#define QHA2M_DATAADDR      (((1UL<<16)-1)<<QHA2S_DATAADDR)
#define QHA2M_RELOAD        (((1UL<<4)-1)<<QHA2S_RELOAD)

/* DW 3 */
#define QHA3S_TRANSCOUNT     0    /* Amount of bytes transferred */
#define QHA3S_NAKCOUNT      19    /* NAK Counter, when reaching 0, VALID is cleared */
#define QHA3S_RETRYCOUNT    23    /* Error Retry Counter */
#define QHA3B_DATA1         25    /* Data Toggle 1 */
#define QHA3B_PING          26    /* Ping State */
#define QHA3B_COMPLETESPLIT 27    /* Complete Split (Split transfer) */
#define QHA3B_PID_ERROR     28    /* PID Error */
#define QHA3B_BABBLE_ERROR  29    /* Babble Detected */
#define QHA3B_HALT          30    /* Halt of QH */
#define QHA3B_ACTIVE        31    /* Active, same as Valid */

#define QHA3M_TRANSCOUNT    (((1UL<<15)-1)<<QHA3S_TRANSCOUNT)
#define QHA3M_TRANSCOUNT_LS (((1UL<<12)-1)<<QHA3S_TRANSCOUNT)
#define QHA3M_NAKCOUNT      (((1UL<<4)-1)<<QHA3S_NAKCOUNT)
#define QHA3M_RETRYCOUNT    (((1UL<<2)-1)<<QHA3S_RETRYCOUNT)

#define QHA3F_DATA0         (0UL<<QHA3B_DATA1)
#define QHA3F_DATA1         (1UL<<QHA3B_DATA1)
#define QHA3F_PING          (1UL<<QHA3B_PING)
#define QHA3F_COMPLETESPLIT (1UL<<QHA3B_COMPLETESPLIT)
#define QHA3F_PID_ERROR     (1UL<<QHA3B_PID_ERROR)
#define QHA3F_BABBLE_ERROR  (1UL<<QHA3B_BABBLE_ERROR)
#define QHA3F_HALT          (1UL<<QHA3B_HALT)
#define QHA3F_ACTIVE        (1UL<<QHA3B_ACTIVE)

/* DW 4 for ATL */
#define QHA4S_NEXTPTD        0    /* Next PTD Pointer */
#define QHA4B_JUMP           5    /* Enable NextPTD Jumping */

#define QHA4M_NEXTPTD       (((1UL<<5)-1)<<QHA4S_NEXTPTD)
#define QHA4F_JUMP          (1UL<<QHA4B_JUMP)

/* DW 4 for ISO/INT */
#define QHA4S_MUSOFACTIVE    0    /* µSOF Active */
#define QHA4S_FIRSTSTATUS    8    /* First status bits */

#define QHA4M_MUSOFACTIVE   (((1UL<<8)-1)<<QHA4S_MUSOFACTIVE)
#define QHA4M_FIRSTSTATUS   (((1UL<<3)-1)<<QHA4S_FIRSTSTATUS)

// bits 8 to 31 are triples of status bits
#define QHASB_TRANS_ERROR    0    /* Transaction error (IN or OUT) */
#define QHASB_BABBLE_ERROR   1    /* Babble (IN) */
#define QHASB_UNDERRUN       2    /* Underrun (OUT) */

#define QHASF_TRANS_ERROR   (1UL<<QHASB_TRANS_ERROR)
#define QHASF_BABBLE_ERROR  (1UL<<QHASB_BABBLE_ERROR)
#define QHASF_UNDERRUN      (1UL<<QHASB_UNDERRUN)

/* DW 5-7 for ISO/INT contain 12 bits of transfer length */
#define QHA5S_MUSOFCSPLIT    0    /* When to send the complete split */

#define QHA5M_MUSOFCSPLIT   (((1UL<<8)-1)<<QHA5S_MUSOFCSPLIT)

#endif /* ISP1760_H */

