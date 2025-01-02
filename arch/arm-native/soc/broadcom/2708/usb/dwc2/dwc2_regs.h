#ifndef DWC2_REGS_H
#define DWC2_REGS_H

#define BIT(x) (1 << (x))

#define DWC2_BASE ((uintptr_t)ARM_PERIIOBASE + 0x980000)

/*** Core Global Registers (for both device and host modes) ***/

/* OTG Control and Status Register */
#define GOTGCTL ((volatile uint32_t *)(DWC2_BASE + 0x000))

#define GOTGCTL_CHIRPEN                BIT(27)
#define GOTGCTL_MULT_VALID_BC_MASK    (0x1f << 22)
#define GOTGCTL_MULT_VALID_BC_SHIFT    22
#define GOTGCTL_CURMODE_HOST        BIT(21)
#define GOTGCTL_OTGVER                BIT(20)
#define GOTGCTL_BSESVLD                BIT(19)
#define GOTGCTL_ASESVLD                BIT(18)
#define GOTGCTL_DBNC_SHORT            BIT(17)
#define GOTGCTL_CONID_B                BIT(16)
#define GOTGCTL_DBNCE_FLTR_BYPASS    BIT(15)
#define GOTGCTL_DEVHNPEN            BIT(11)
#define GOTGCTL_HSTSETHNPEN            BIT(10)
#define GOTGCTL_HNPREQ                BIT(9)
#define GOTGCTL_HSTNEGSCS            BIT(8)
#define GOTGCTL_BVALOVAL            BIT(7)
#define GOTGCTL_BVALOEN                BIT(6)
#define GOTGCTL_AVALOVAL            BIT(5)
#define GOTGCTL_AVALOEN                BIT(4)
#define GOTGCTL_VBVALOVAL            BIT(3)
#define GOTGCTL_VBVALOEN            BIT(2)
#define GOTGCTL_SESREQ                BIT(1)
#define GOTGCTL_SESREQSCS            BIT(0)

/* OTG Interrupt Register */
#define GOTGINT ((volatile uint32_t *)(DWC2_BASE + 0x004))

#define GOTGINT_DBNCE_DONE        BIT(19)
#define GOTGINT_A_DEV_TOUT_CHG        BIT(18)
#define GOTGINT_HST_NEG_DET        BIT(17)
#define GOTGINT_HST_NEG_SUC_STS_CHNG    BIT(9)
#define GOTGINT_SES_REQ_SUC_STS_CHNG    BIT(8)
#define GOTGINT_SES_END_DET        BIT(2)

/* Core AHB Configuration Register */
#define GAHBCFG ((volatile uint32_t *)(DWC2_BASE + 0x008))

#define GAHBCFG_AHB_SINGLE        BIT(23)
#define GAHBCFG_NOTI_ALL_DMA_WRIT    BIT(22)
#define GAHBCFG_REM_MEM_SUPP        BIT(21)
#define GAHBCFG_P_TXF_EMP_LVL        BIT(8)
#define GAHBCFG_NP_TXF_EMP_LVL        BIT(7)
#define GAHBCFG_DMA_EN            BIT(5)
#define GAHBCFG_HBSTLEN_MASK        (0xf << 1)
#define GAHBCFG_HBSTLEN_SHIFT        1
#define GAHBCFG_HBSTLEN_SINGLE        0
#define GAHBCFG_HBSTLEN_INCR        1
#define GAHBCFG_HBSTLEN_INCR4        3
#define GAHBCFG_HBSTLEN_INCR8        5
#define GAHBCFG_HBSTLEN_INCR16        7
#define GAHBCFG_GLBL_INTR_EN        BIT(0)
#define GAHBCFG_CTRL_MASK        (GAHBCFG_P_TXF_EMP_LVL | \
                     GAHBCFG_NP_TXF_EMP_LVL | \
                     GAHBCFG_DMA_EN | \
                     GAHBCFG_GLBL_INTR_EN)

/* Core USB Configuration Register */
#define GUSBCFG ((volatile uint32_t *)(DWC2_BASE + 0x00C))

#define GUSBCFG_FORCEDEVMODE        BIT(30)
#define GUSBCFG_FORCEHOSTMODE        BIT(29)
#define GUSBCFG_TXENDDELAY        BIT(28)
#define GUSBCFG_ICTRAFFICPULLREMOVE    BIT(27)
#define GUSBCFG_ICUSBCAP        BIT(26)
#define GUSBCFG_ULPI_INT_PROT_DIS    BIT(25)
#define GUSBCFG_INDICATORPASSTHROUGH    BIT(24)
#define GUSBCFG_INDICATORCOMPLEMENT    BIT(23)
#define GUSBCFG_TERMSELDLPULSE        BIT(22)
#define GUSBCFG_ULPI_INT_VBUS_IND    BIT(21)
#define GUSBCFG_ULPI_EXT_VBUS_DRV    BIT(20)
#define GUSBCFG_ULPI_CLK_SUSP_M        BIT(19)
#define GUSBCFG_ULPI_AUTO_RES        BIT(18)
#define GUSBCFG_ULPI_FS_LS        BIT(17)
#define GUSBCFG_OTG_UTMI_FS_SEL        BIT(16)
#define GUSBCFG_PHY_LP_CLK_SEL        BIT(15)
#define GUSBCFG_USBTRDTIM_MASK        (0xf << 10)
#define GUSBCFG_USBTRDTIM_SHIFT        10
#define GUSBCFG_HNPCAP            BIT(9)
#define GUSBCFG_SRPCAP            BIT(8)
#define GUSBCFG_DDRSEL            BIT(7)
#define GUSBCFG_PHYSEL            BIT(6)
#define GUSBCFG_FSINTF            BIT(5)
#define GUSBCFG_ULPI_UTMI_SEL        BIT(4)
#define GUSBCFG_PHYIF16            BIT(3)
#define GUSBCFG_PHYIF8            (0 << 3)
#define GUSBCFG_TOUTCAL_MASK        (0x7 << 0)
#define GUSBCFG_TOUTCAL_SHIFT        0
#define GUSBCFG_TOUTCAL_LIMIT        0x7
#define GUSBCFG_TOUTCAL(_x)        ((_x) << 0)

/* Core Reset Register */
#define GRSTCTL ((volatile uint32_t *)(DWC2_BASE + 0x010))

#define GRSTCTL_AHBIDLE            BIT(31)
#define GRSTCTL_DMAREQ            BIT(30)
#define GRSTCTL_CSFTRST_DONE        BIT(29)
#define GRSTCTL_TXFNUM_MASK        (0x1f << 6)
#define GRSTCTL_TXFNUM_SHIFT        6
#define GRSTCTL_TXFNUM_LIMIT        0x1f
#define GRSTCTL_TXFNUM(_x)        ((_x) << 6)
#define GRSTCTL_TXFFLSH            BIT(5)
#define GRSTCTL_RXFFLSH            BIT(4)
#define GRSTCTL_IN_TKNQ_FLSH        BIT(3)
#define GRSTCTL_FRMCNTRRST        BIT(2)
#define GRSTCTL_HSFTRST            BIT(1)
#define GRSTCTL_CSFTRST            BIT(0)

/* Core Interrupt Register */
#define GINTSTS ((volatile uint32_t *)(DWC2_BASE + 0x014))
/* Core Interrupt Mask Register */
#define GINTMSK ((volatile uint32_t *)(DWC2_BASE + 0x018))

#define GINTSTS_WKUPINT            BIT(31)
#define GINTSTS_SESSREQINT        BIT(30)
#define GINTSTS_DISCONNINT        BIT(29)
#define GINTSTS_CONIDSTSCHNG        BIT(28)
#define GINTSTS_LPMTRANRCVD        BIT(27)
#define GINTSTS_PTXFEMP            BIT(26)
#define GINTSTS_HCHINT            BIT(25)
#define GINTSTS_PRTINT            BIT(24)
#define GINTSTS_RESETDET        BIT(23)
#define GINTSTS_FET_SUSP        BIT(22)
#define GINTSTS_INCOMPL_IP        BIT(21)
#define GINTSTS_INCOMPL_SOOUT        BIT(21)
#define GINTSTS_INCOMPL_SOIN        BIT(20)
#define GINTSTS_OEPINT            BIT(19)
#define GINTSTS_IEPINT            BIT(18)
#define GINTSTS_EPMIS            BIT(17)
#define GINTSTS_RESTOREDONE        BIT(16)
#define GINTSTS_EOPF            BIT(15)
#define GINTSTS_ISOUTDROP        BIT(14)
#define GINTSTS_ENUMDONE        BIT(13)
#define GINTSTS_USBRST            BIT(12)
#define GINTSTS_USBSUSP            BIT(11)
#define GINTSTS_ERLYSUSP        BIT(10)
#define GINTSTS_I2CINT            BIT(9)
#define GINTSTS_ULPI_CK_INT        BIT(8)
#define GINTSTS_GOUTNAKEFF        BIT(7)
#define GINTSTS_GINNAKEFF        BIT(6)
#define GINTSTS_NPTXFEMP        BIT(5)
#define GINTSTS_RXFLVL            BIT(4)
#define GINTSTS_SOF            BIT(3)
#define GINTSTS_OTGINT            BIT(2)
#define GINTSTS_MODEMIS            BIT(1)
#define GINTSTS_CURMODE_HOST        BIT(0)

/* Receive Status Debug Read/Status Read and Pop Registers */
#define GRXSTSR ((volatile uint32_t *)(DWC2_BASE + 0x01C))
#define GRXSTSP ((volatile uint32_t *)(DWC2_BASE + 0x020))

#define GRXSTS_FN_MASK            (0x7f << 25)
#define GRXSTS_FN_SHIFT            25
#define GRXSTS_PKTSTS_MASK        (0xf << 17)
#define GRXSTS_PKTSTS_SHIFT        17
#define GRXSTS_PKTSTS_GLOBALOUTNAK    1
#define GRXSTS_PKTSTS_OUTRX        2
#define GRXSTS_PKTSTS_HCHIN        2
#define GRXSTS_PKTSTS_OUTDONE        3
#define GRXSTS_PKTSTS_HCHIN_XFER_COMP    3
#define GRXSTS_PKTSTS_SETUPDONE        4
#define GRXSTS_PKTSTS_DATATOGGLEERR    5
#define GRXSTS_PKTSTS_SETUPRX        6
#define GRXSTS_PKTSTS_HCHHALTED        7
#define GRXSTS_HCHNUM_MASK        (0xf << 0)
#define GRXSTS_HCHNUM_SHIFT        0
#define GRXSTS_DPID_MASK        (0x3 << 15)
#define GRXSTS_DPID_SHIFT        15
#define GRXSTS_BYTECNT_MASK        (0x7ff << 4)
#define GRXSTS_BYTECNT_SHIFT        4
#define GRXSTS_EPNUM_MASK        (0xf << 0)
#define GRXSTS_EPNUM_SHIFT        0

/* Receive FIFO Size Register */
#define GRXFSIZ ((volatile uint32_t *)(DWC2_BASE + 0x024))

#define GRXFSIZ_DEPTH_MASK        (0xffff << 0)
#define GRXFSIZ_DEPTH_SHIFT        0

/* Non-Periodic Transmit FIFO Size Register */
#define GNPTXFSIZ ((volatile uint32_t *)(DWC2_BASE + 0x028))

/* Non-Periodic Transmit FIFO/Queue Status Register */
#define GNPTXSTS ((volatile uint32_t *)(DWC2_BASE + 0x02C))

#define GNPTXSTS_NP_TXQ_TOP_MASK        (0x7f << 24)
#define GNPTXSTS_NP_TXQ_TOP_SHIFT        24
#define GNPTXSTS_NP_TXQ_SPC_AVAIL_MASK        (0xff << 16)
#define GNPTXSTS_NP_TXQ_SPC_AVAIL_SHIFT        16
#define GNPTXSTS_NP_TXQ_SPC_AVAIL_GET(_v)    (((_v) >> 16) & 0xff)
#define GNPTXSTS_NP_TXF_SPC_AVAIL_MASK        (0xffff << 0)
#define GNPTXSTS_NP_TXF_SPC_AVAIL_SHIFT        0
#define GNPTXSTS_NP_TXF_SPC_AVAIL_GET(_v)    (((_v) >> 0) & 0xffff)

/* I2C Access Register */
#define GI2CCTL ((volatile uint32_t *)(DWC2_BASE + 0x030))

#define GI2CCTL_BSYDNE            BIT(31)
#define GI2CCTL_RW            BIT(30)
#define GI2CCTL_I2CDATSE0        BIT(28)
#define GI2CCTL_I2CDEVADDR_MASK        (0x3 << 26)
#define GI2CCTL_I2CDEVADDR_SHIFT    26
#define GI2CCTL_I2CSUSPCTL        BIT(25)
#define GI2CCTL_ACK            BIT(24)
#define GI2CCTL_I2CEN            BIT(23)
#define GI2CCTL_ADDR_MASK        (0x7f << 16)
#define GI2CCTL_ADDR_SHIFT        16
#define GI2CCTL_REGADDR_MASK        (0xff << 8)
#define GI2CCTL_REGADDR_SHIFT        8
#define GI2CCTL_RWDATA_MASK        (0xff << 0)
#define GI2CCTL_RWDATA_SHIFT        0

/* PHY Vendor Control Register */
#define GPVNDCTL ((volatile uint32_t *)(DWC2_BASE + 0x034))

/* General Purpose Input/Output Register */
#define GGPIO ((volatile uint32_t *)(DWC2_BASE + 0x038))

#define GGPIO_STM32_OTG_GCCFG_PWRDWN    BIT(16)
#define GGPIO_STM32_OTG_GCCFG_VBDEN    BIT(21)
#define GGPIO_STM32_OTG_GCCFG_IDEN    BIT(22)

/* User ID Register */
#define GUID ((volatile uint32_t *)(DWC2_BASE + 0x03C))

/* Synopsys ID Register */
#define GSNPSID ((volatile uint32_t *)(DWC2_BASE + 0x040))

/* User HW Config1 Register */
#define GHWCFG1 ((volatile uint32_t *)(DWC2_BASE + 0x044))

/* User HW Config2 Register */
#define GHWCFG2 ((volatile uint32_t *)(DWC2_BASE + 0x048))

#define GHWCFG2_OTG_ENABLE_IC_USB        BIT(31)
#define GHWCFG2_DEV_TOKEN_Q_DEPTH_MASK        (0x1f << 26)
#define GHWCFG2_DEV_TOKEN_Q_DEPTH_SHIFT        26
#define GHWCFG2_HOST_PERIO_TX_Q_DEPTH_MASK    (0x3 << 24)
#define GHWCFG2_HOST_PERIO_TX_Q_DEPTH_SHIFT    24
#define GHWCFG2_NONPERIO_TX_Q_DEPTH_MASK    (0x3 << 22)
#define GHWCFG2_NONPERIO_TX_Q_DEPTH_SHIFT    22
#define GHWCFG2_MULTI_PROC_INT            BIT(20)
#define GHWCFG2_DYNAMIC_FIFO            BIT(19)
#define GHWCFG2_PERIO_EP_SUPPORTED        BIT(18)
#define GHWCFG2_NUM_HOST_CHAN_MASK        (0xf << 14)
#define GHWCFG2_NUM_HOST_CHAN_SHIFT        14
#define GHWCFG2_NUM_DEV_EP_MASK            (0xf << 10)
#define GHWCFG2_NUM_DEV_EP_SHIFT        10
#define GHWCFG2_FS_PHY_TYPE_MASK        (0x3 << 8)
#define GHWCFG2_FS_PHY_TYPE_SHIFT        8
#define GHWCFG2_FS_PHY_TYPE_NOT_SUPPORTED    0
#define GHWCFG2_FS_PHY_TYPE_DEDICATED        1
#define GHWCFG2_FS_PHY_TYPE_SHARED_UTMI        2
#define GHWCFG2_FS_PHY_TYPE_SHARED_ULPI        3
#define GHWCFG2_HS_PHY_TYPE_MASK        (0x3 << 6)
#define GHWCFG2_HS_PHY_TYPE_SHIFT        6
#define GHWCFG2_HS_PHY_TYPE_NOT_SUPPORTED    0
#define GHWCFG2_HS_PHY_TYPE_UTMI        1
#define GHWCFG2_HS_PHY_TYPE_ULPI        2
#define GHWCFG2_HS_PHY_TYPE_UTMI_ULPI        3
#define GHWCFG2_POINT2POINT            BIT(5)
#define GHWCFG2_ARCHITECTURE_MASK        (0x3 << 3)
#define GHWCFG2_ARCHITECTURE_SHIFT        3
#define GHWCFG2_SLAVE_ONLY_ARCH            0
#define GHWCFG2_EXT_DMA_ARCH            1
#define GHWCFG2_INT_DMA_ARCH            2
#define GHWCFG2_OP_MODE_MASK            (0x7 << 0)
#define GHWCFG2_OP_MODE_SHIFT            0
#define GHWCFG2_OP_MODE_HNP_SRP_CAPABLE        0
#define GHWCFG2_OP_MODE_SRP_ONLY_CAPABLE    1
#define GHWCFG2_OP_MODE_NO_HNP_SRP_CAPABLE    2
#define GHWCFG2_OP_MODE_SRP_CAPABLE_DEVICE    3
#define GHWCFG2_OP_MODE_NO_SRP_CAPABLE_DEVICE    4
#define GHWCFG2_OP_MODE_SRP_CAPABLE_HOST    5
#define GHWCFG2_OP_MODE_NO_SRP_CAPABLE_HOST    6
#define GHWCFG2_OP_MODE_UNDEFINED        7

/* User HW Config3 Register */
#define GHWCFG3 ((volatile uint32_t *)(DWC2_BASE + 0x04C))

#define GHWCFG3_DFIFO_DEPTH_MASK        (0xffff << 16)
#define GHWCFG3_DFIFO_DEPTH_SHIFT        16
#define GHWCFG3_OTG_LPM_EN            BIT(15)
#define GHWCFG3_BC_SUPPORT            BIT(14)
#define GHWCFG3_OTG_ENABLE_HSIC            BIT(13)
#define GHWCFG3_ADP_SUPP            BIT(12)
#define GHWCFG3_SYNCH_RESET_TYPE        BIT(11)
#define GHWCFG3_OPTIONAL_FEATURES        BIT(10)
#define GHWCFG3_VENDOR_CTRL_IF            BIT(9)
#define GHWCFG3_I2C                BIT(8)
#define GHWCFG3_OTG_FUNC            BIT(7)
#define GHWCFG3_PACKET_SIZE_CNTR_WIDTH_MASK    (0x7 << 4)
#define GHWCFG3_PACKET_SIZE_CNTR_WIDTH_SHIFT    4
#define GHWCFG3_XFER_SIZE_CNTR_WIDTH_MASK    (0xf << 0)
#define GHWCFG3_XFER_SIZE_CNTR_WIDTH_SHIFT    0

/* User HW Config4 Register */
#define GHWCFG4 ((volatile uint32_t *)(DWC2_BASE + 0x050))

#define GHWCFG4_DESC_DMA_DYN            BIT(31)
#define GHWCFG4_DESC_DMA            BIT(30)
#define GHWCFG4_NUM_IN_EPS_MASK            (0xf << 26)
#define GHWCFG4_NUM_IN_EPS_SHIFT        26
#define GHWCFG4_DED_FIFO_EN            BIT(25)
#define GHWCFG4_DED_FIFO_SHIFT        25
#define GHWCFG4_SESSION_END_FILT_EN        BIT(24)
#define GHWCFG4_B_VALID_FILT_EN            BIT(23)
#define GHWCFG4_A_VALID_FILT_EN            BIT(22)
#define GHWCFG4_VBUS_VALID_FILT_EN        BIT(21)
#define GHWCFG4_IDDIG_FILT_EN            BIT(20)
#define GHWCFG4_NUM_DEV_MODE_CTRL_EP_MASK    (0xf << 16)
#define GHWCFG4_NUM_DEV_MODE_CTRL_EP_SHIFT    16
#define GHWCFG4_UTMI_PHY_DATA_WIDTH_MASK    (0x3 << 14)
#define GHWCFG4_UTMI_PHY_DATA_WIDTH_SHIFT    14
#define GHWCFG4_UTMI_PHY_DATA_WIDTH_8        0
#define GHWCFG4_UTMI_PHY_DATA_WIDTH_16        1
#define GHWCFG4_UTMI_PHY_DATA_WIDTH_8_OR_16    2
#define GHWCFG4_ACG_SUPPORTED            BIT(12)
#define GHWCFG4_IPG_ISOC_SUPPORTED        BIT(11)
#define GHWCFG4_SERVICE_INTERVAL_SUPPORTED      BIT(10)
#define GHWCFG4_XHIBER                BIT(7)
#define GHWCFG4_HIBER                BIT(6)
#define GHWCFG4_MIN_AHB_FREQ            BIT(5)
#define GHWCFG4_POWER_OPTIMIZ            BIT(4)
#define GHWCFG4_NUM_DEV_PERIO_IN_EP_MASK    (0xf << 0)
#define GHWCFG4_NUM_DEV_PERIO_IN_EP_SHIFT    0

/* Host Periodic Transmit FIFO Size Register */
#define HPTXFSIZ ((volatile uint32_t *)(DWC2_BASE + 0x100))

/*** Host Mode Registers ***/

/* Host Configuration Register */
#define HCFG ((volatile uint32_t *)(DWC2_BASE + 0x400))

#define HCFG_MODECHTIMEN        BIT(31)
#define HCFG_PERSCHEDENA        BIT(26)
#define HCFG_FRLISTEN_MASK        (0x3 << 24)
#define HCFG_FRLISTEN_SHIFT        24
#define HCFG_FRLISTEN_8                (0 << 24)
#define FRLISTEN_8_SIZE                8
#define HCFG_FRLISTEN_16            BIT(24)
#define FRLISTEN_16_SIZE            16
#define HCFG_FRLISTEN_32            (2 << 24)
#define FRLISTEN_32_SIZE            32
#define HCFG_FRLISTEN_64            (3 << 24)
#define FRLISTEN_64_SIZE            64
#define HCFG_DESCDMA            BIT(23)
#define HCFG_RESVALID_MASK        (0xff << 8)
#define HCFG_RESVALID_SHIFT        8
#define HCFG_ENA32KHZ            BIT(7)
#define HCFG_FSLSSUPP            BIT(2)
#define HCFG_FSLSPCLKSEL_MASK        (0x3 << 0)
#define HCFG_FSLSPCLKSEL_SHIFT        0
#define HCFG_FSLSPCLKSEL_30_60_MHZ    0
#define HCFG_FSLSPCLKSEL_48_MHZ        1
#define HCFG_FSLSPCLKSEL_6_MHZ        2

/* Host Frame Interval Register */
#define HFIR ((volatile uint32_t *)(DWC2_BASE + 0x404))

#define HFIR_FRINT_MASK            (0xffff << 0)
#define HFIR_FRINT_SHIFT        0
#define HFIR_RLDCTRL            BIT(16)

/* Host Frame Number/Frame Time Remaining Register */
#define HFNUM ((volatile uint32_t *)(DWC2_BASE + 0x408))

#define HFNUM_FRREM_MASK        (0xffff << 16)
#define HFNUM_FRREM_SHIFT        16
#define HFNUM_FRNUM_MASK        (0xffff << 0)
#define HFNUM_FRNUM_SHIFT        0
#define HFNUM_MAX_FRNUM            0x3fff

/* Host Periodic Transmit FIFO/Queue Status Register */
#define HPTXSTS ((volatile uint32_t *)(DWC2_BASE + 0x410))

#define TXSTS_QTOP_ODD            BIT(31)
#define TXSTS_QTOP_CHNEP_MASK        (0xf << 27)
#define TXSTS_QTOP_CHNEP_SHIFT        27
#define TXSTS_QTOP_TOKEN_MASK        (0x3 << 25)
#define TXSTS_QTOP_TOKEN_SHIFT        25
#define TXSTS_QTOP_TERMINATE        BIT(24)
#define TXSTS_QSPCAVAIL_MASK        (0xff << 16)
#define TXSTS_QSPCAVAIL_SHIFT        16
#define TXSTS_FSPCAVAIL_MASK        (0xffff << 0)
#define TXSTS_FSPCAVAIL_SHIFT        0

/* Host All Channels Interrupt Register */
#define HAINT ((volatile uint32_t *)(DWC2_BASE + 0x414))

/* Host All Channels Interrupt Mask Register */
#define HAINTMSK ((volatile uint32_t *)(DWC2_BASE + 0x418))

/* Host Port Control and Status Register */
#define HPRT0 ((volatile uint32_t *)(DWC2_BASE + 0x440))

#define HPRT0_SPD_MASK            (0x3 << 17)
#define HPRT0_SPD_SHIFT            17
#define HPRT0_SPD_HIGH_SPEED        0
#define HPRT0_SPD_FULL_SPEED        1
#define HPRT0_SPD_LOW_SPEED        2
#define HPRT0_TSTCTL_MASK        (0xf << 13)
#define HPRT0_TSTCTL_SHIFT        13
#define HPRT0_PWR            BIT(12)
#define HPRT0_LNSTS_MASK        (0x3 << 10)
#define HPRT0_LNSTS_SHIFT        10
#define HPRT0_RST            BIT(8)
#define HPRT0_SUSP            BIT(7)
#define HPRT0_RES            BIT(6)
#define HPRT0_OVRCURRCHG        BIT(5)
#define HPRT0_OVRCURRACT        BIT(4)
#define HPRT0_ENACHG            BIT(3)
#define HPRT0_ENA            BIT(2)
#define HPRT0_CONNDET            BIT(1)
#define HPRT0_CONNSTS            BIT(0)

// Bits in HPRT0 that are cleared by writing a 1
// When writing to HPRT0, these bits must be 0 unless you are attempting to clear them.
#define HPRT0_WRITE_CLEAR_BITS (HPRT0_CONNDET|HPRT0_ENA|HPRT0_ENACHG|HPRT0_OVRCURRCHG)

/* Host Channel-n Characteristics Register */
#define HCCHAR(chan) ((volatile uint32_t *)(DWC2_BASE + 0x500 + 0x20 * (chan)))

#define HCCHAR_CHENA			BIT(31)
#define HCCHAR_CHDIS			BIT(30)
#define HCCHAR_ODDFRM			BIT(29)
#define HCCHAR_DEVADDR_MASK		(0x7f << 22)
#define HCCHAR_DEVADDR_SHIFT		22
#define HCCHAR_MULTICNT_MASK		(0x3 << 20)
#define HCCHAR_MULTICNT_SHIFT		20
#define HCCHAR_EPTYPE_MASK		(0x3 << 18)
#define HCCHAR_EPTYPE_SHIFT		18
#define HCCHAR_LSPDDEV			BIT(17)
#define HCCHAR_EPDIR			BIT(15)
#define HCCHAR_EPNUM_MASK		(0xf << 11)
#define HCCHAR_EPNUM_SHIFT		11
#define HCCHAR_MPS_MASK			(0x7ff << 0)
#define HCCHAR_MPS_SHIFT		0

enum HCCHAR_EPTYPE
{
    HCCHAR_EPTYPE_CONTROL     = 0,
    HCCHAR_EPTYPE_ISOCHRONOUS = 1,
    HCCHAR_EPTYPE_BULK        = 2,
    HCCHAR_EPTYPE_INTERRUPT   = 3,
};

/* Host Channel-n Split Control Register */
#define HCSPLT(chan) ((volatile uint32_t *)(DWC2_BASE + 0x504 + 0x20 * (chan)))

#define HCSPLT_SPLTENA			BIT(31)
#define HCSPLT_COMPSPLT			BIT(16)
#define HCSPLT_XACTPOS_MASK		(0x3 << 14)
#define HCSPLT_XACTPOS_SHIFT		14
#define HCSPLT_XACTPOS_MID		0
#define HCSPLT_XACTPOS_END		1
#define HCSPLT_XACTPOS_BEGIN		2
#define HCSPLT_XACTPOS_ALL		3
#define HCSPLT_HUBADDR_MASK		(0x7f << 7)
#define HCSPLT_HUBADDR_SHIFT		7
#define HCSPLT_PRTADDR_MASK		(0x7f << 0)
#define HCSPLT_PRTADDR_SHIFT		0

/* Host Channel-n Interrupt Register */
#define HCINT(chan) ((volatile uint32_t *)(DWC2_BASE + 0x508 + 0x20 * (chan)))
/* Host Channel-n Interrupt Mask Register */
#define HCINTMSK(chan) ((volatile uint32_t *)(DWC2_BASE + 0x50C + 0x20 * (chan)))

#define HCINTMSK_RESERVED14_31		(0x3ffff << 14)
#define HCINTMSK_FRM_LIST_ROLL		BIT(13)
#define HCINTMSK_XCS_XACT		BIT(12)
#define HCINTMSK_BNA			BIT(11)
#define HCINTMSK_DATATGLERR		BIT(10)
#define HCINTMSK_FRMOVRUN		BIT(9)
#define HCINTMSK_BBLERR			BIT(8)
#define HCINTMSK_XACTERR		BIT(7)
#define HCINTMSK_NYET			BIT(6)
#define HCINTMSK_ACK			BIT(5)
#define HCINTMSK_NAK			BIT(4)
#define HCINTMSK_STALL			BIT(3)
#define HCINTMSK_AHBERR			BIT(2)
#define HCINTMSK_CHHLTD			BIT(1)
#define HCINTMSK_XFERCOMPL		BIT(0)

/* Host Channel-n Transfer Size Register */
#define HCTSIZ(chan) ((volatile uint32_t *)(DWC2_BASE + 0x510 + 0x20 * (chan)))

#define TSIZ_DOPNG			BIT(31)
#define TSIZ_SC_MC_PID_MASK		(0x3 << 29)
#define TSIZ_SC_MC_PID_SHIFT		29
#define TSIZ_SC_MC_PID_DATA0		0
#define TSIZ_SC_MC_PID_DATA2		1
#define TSIZ_SC_MC_PID_DATA1		2
#define TSIZ_SC_MC_PID_MDATA		3
#define TSIZ_SC_MC_PID_SETUP		3
#define TSIZ_PKTCNT_MASK		(0x3ff << 19)
#define TSIZ_PKTCNT_SHIFT		19
#define TSIZ_NTD_MASK			(0xff << 8)
#define TSIZ_NTD_SHIFT			8
#define TSIZ_SCHINFO_MASK		(0xff << 0)
#define TSIZ_SCHINFO_SHIFT		0
#define TSIZ_XFERSIZE_MASK		(0x7ffff << 0)
#define TSIZ_XFERSIZE_SHIFT		0

/* Host Channel-n DMA Address Register */
#define HCDMA(chan) ((volatile uint32_t *)(DWC2_BASE + 0x514 + 0x20 * (chan)))

/*** Power and Clock Gating Registers ***/

/* Power and Clock Gating Control Register */
#define PCGCCTL ((volatile uint32_t *)(DWC2_BASE + 0xE00))

#define PCGCTL_IF_DEV_MODE        BIT(31)
#define PCGCTL_P2HD_PRT_SPD_MASK    (0x3 << 29)
#define PCGCTL_P2HD_PRT_SPD_SHIFT    29
#define PCGCTL_P2HD_DEV_ENUM_SPD_MASK    (0x3 << 27)
#define PCGCTL_P2HD_DEV_ENUM_SPD_SHIFT    27
#define PCGCTL_MAC_DEV_ADDR_MASK    (0x7f << 20)
#define PCGCTL_MAC_DEV_ADDR_SHIFT    20
#define PCGCTL_MAX_TERMSEL        BIT(19)
#define PCGCTL_MAX_XCVRSELECT_MASK    (0x3 << 17)
#define PCGCTL_MAX_XCVRSELECT_SHIFT    17
#define PCGCTL_PORT_POWER        BIT(16)
#define PCGCTL_PRT_CLK_SEL_MASK        (0x3 << 14)
#define PCGCTL_PRT_CLK_SEL_SHIFT   14
#define PCGCTL_ESS_REG_RESTORED    BIT(13)
#define PCGCTL_EXTND_HIBER_SWITCH  BIT(12)
#define PCGCTL_EXTND_HIBER_PWRCLMP BIT(11)
#define PCGCTL_ENBL_EXTND_HIBER    BIT(10)
#define PCGCTL_RESTOREMODE         BIT(9)
#define PCGCTL_RESETAFTSUSP        BIT(8)
#define PCGCTL_DEEP_SLEEP          BIT(7)
#define PCGCTL_PHY_IN_SLEEP        BIT(6)
#define PCGCTL_ENBL_SLEEP_GATING   BIT(5)
#define PCGCTL_RSTPDWNMODULE       BIT(3)
#define PCGCTL_PWRCLMP             BIT(2)
#define PCGCTL_GATEHCLK            BIT(1)
#define PCGCTL_STOPPCLK            BIT(0)

#endif
