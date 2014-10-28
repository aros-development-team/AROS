/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: sun4i clock control module
    Lang: english
*/

#ifndef HARDWARE_SUN4I_CCM_H
#define HARDWARE_SUN4I_CCM_H

#ifndef _INTTYPES_H
#include <inttypes.h>
#endif

#define SUN4I_CCM_BASE			0x01c28000

struct CCM {
	uint32_t PLL1_CFG;
	uint32_t PLL1_TUN;
	uint32_t PLL2_CFG;
	uint32_t PLL2_TUN;
	uint32_t PLL3_CFG;
	uint32_t CCM_RESERVED_1;
	uint32_t PLL4_CFG;
	uint32_t CCM_RESERVED_2;
	uint32_t PLL5_CFG;
	uint32_t PLL5_TUN;
	uint32_t PLL6_CFG;
	uint32_t PLL6_TUN;
	uint32_t PLL7_CFG;
	uint32_t CCM_RESERVED_3;
	uint32_t PLL1_TUN2;
	uint32_t PLL5_TUN2;
	uint8_t  CCM_RESERVED_4[12]; 	
	uint32_t PLL_LOCK_DBG;
	uint32_t OSC24M_CFG;
	uint32_t CPU_AHB_APB0_CFG;
	uint32_t APB1_CLK_DIV;
	uint32_t AXI_GATING;
	uint32_t AHB_GATING0;
	uint32_t AHB_GATING1;
	uint32_t APB0_GATING;
	uint32_t APB1_GATING;
	uint8_t  CCM_RESERVED_5[16]; 	
	uint32_t NAND_SCLK_CFG;
	uint32_t MS_SCLK_CFG;
	uint32_t MMC0_SCLK_CFG;
	uint32_t MMC1_SCLK_CFG;
	uint32_t MMC2_SCLK_CFG;
	uint32_t MMC3_SCLK_CFG;
	uint32_t TS_CLK;
	uint32_t SS_CLK;
	uint32_t SPI0_CLK;
	uint32_t SPI1_CLK;
	uint32_t SPI2_CLK;
	uint32_t PATA_CLK;
	uint32_t IR0_CLK;
	uint32_t IR1_CLK;
	uint32_t IIS_CLK;
	uint32_t AC97_CLK;
	uint32_t SPDIF_CLK;
	uint32_t KEYPAD_CLK;
	uint32_t SATA_CLK;
	uint32_t USB_CLK;
	uint32_t GPS_CLK;
	uint32_t SPI3_CLK;
	uint8_t  CCM_RESERVED_6[40]; 	
	uint32_t DRAM_CLK;
	uint32_t BE0_SCLK;
	uint32_t BE1_SCLK;
	uint32_t FE0_CLK;
	uint32_t FE1_CLK;
	uint32_t MP_CLK;
	uint32_t LCD0_CH0_CLK;
	uint32_t LCD1_CH0_CLK;
	uint32_t CSI_ISP_CLK;
	uint32_t CCM_RESERVED_7[12]; 	
	uint32_t TVD_CLK;
	uint32_t LCD0_CH1_CLK;
	uint32_t LCD1_CH1_CLK;
	uint32_t CS0_CLK;
	uint32_t CS1_CLK;
	uint32_t VE_CLK;
	uint32_t AUDIO_CODEC_CLK;
	uint32_t AVS_CLK;
	uint32_t ACE_CLK;
	uint32_t LVDS_CLK;
	uint32_t HDMI_CLK;
	uint32_t MALI400_CLK;
	uint32_t MBUS_CLK;
	uint32_t GMAC_CLK;
	uint32_t HDMI1_RST_CLK;
	uint32_t HDMI1_CTRL_CLK;
	uint32_t HDMI1_SLOW_CLK;	
	uint32_t HDMI1_REPEAT_CLK;
	uint32_t OUTA_CLK;
	uint32_t OUTB_CLK;	
}__attribute__((__packed__));

#endif
