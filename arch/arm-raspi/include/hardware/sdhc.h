/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _SDHC_H
#define _SDHC_H

#define SDHCI_MAKE_BLCKSIZE(bnd, size)  (((bnd & 0x7) << 12) | (size & 0xFFF))
#define SDHCI_MAKE_CMD(cmd, flags)      (((cmd) << 8) | (flags & 0xFF))
#define SDHCI_GET_CMD(x)                ((x >> 8) & 0x3F)

#define SDHCI_TIMEOUT_MAX               0xE

/* SDHC Registers and Attributes */

#define SDHCI_DMA_ADDRESS               0x00
#define SDHCI_BLOCK_SIZE                0x04
#define SDHCI_BLOCK_COUNT               0x06
#define SDHCI_ARGUMENT                  0x08
#define SDHCI_TRANSFER_MODE             0x0C
#define SDHCI_TRANSMOD_DMA              (1 << 0)
#define SDHCI_TRANSMOD_BLK_CNT_EN       (1 << 1)
#define SDHCI_TRANSMOD_ACMD12           (1 << 2)
#define SDHCI_TRANSMOD_READ             (1 << 4)
#define SDHCI_TRANSMOD_MULTI            (1 << 5)
#define SDHCI_COMMAND                   0x0E
#define SDHCI_CMD_RESP_NONE             (0)
#define SDHCI_CMD_RESP_LONG             (1 << 0)
#define SDHCI_CMD_RESP_SHORT            (1 << 1)
#define SDHCI_CMD_RESP_SHORT_BUSY       (SDHCI_CMD_RESP_LONG|SDHCI_CMD_RESP_SHORT)
#define SDHCI_CMD_RESP_MASK             (SDHCI_CMD_RESP_SHORT_BUSY)
#define SDHCI_CMD_CRC   	        (1 << 3)
#define SDHCI_CMD_INDEX	                (1 << 4)
#define SDHCI_CMD_DATA		        (1 << 5)
#define SDHCI_CMD_ABORTCMD	        (3 << 6)
#define SDHCI_RESPONSE                  0x10
#define SDHCI_BUFFER                    0x20
#define SDHCI_PRESENT_STATE             0x24
#define SDHCI_PS_CMD_INHIBIT            (1 << 0)
#define SDHCI_PS_DATA_INHIBIT           (1 << 1)
#define SDHCI_PS_DOING_WRITE            (1 << 8)
#define SDHCI_PS_DOING_READ             (1 << 9)
#define SDHCI_PS_SPACE_AVAILABLE        (1 << 10)
#define SDHCI_PS_DATA_AVAILABLE         (1 << 11)
#define SDHCI_PS_CARD_PRESENT           (1 << 16)
#define SDHCI_PS_CARD_STATE_STABLE      (1 << 17)
#define SDHCI_PS_CARD_DETECT_PIN_LVL    (1 << 18)
#define SDHCI_PS_WRITE_PROTECT          (1 << 19)
#define SDHCI_HOST_CONTROL              0x28
#define SDHCI_HCTRL_LED                 (1 << 0)
#define SDHCI_HCTRL_4BITBUS             (1 << 1)
#define SDHCI_HCTRL_HISPD               (1 << 2)
#define SDHCI_HCTRL_DMA_MASK	        (3 << 3)
#define SDHCI_HCTRL_SDMA                (0 << 3)
#define SDHCI_HCTRL_ADMA1               (1 << 3)
#define SDHCI_HCTRL_ADMA32	        (1 << 4)
#define SDHCI_HCTRL_ADMA64	        (SDHCI_HCTRL_ADMA1|SDHCI_HCTRL_ADMA32)
#define SDHCI_HCTRL_8BITBUS	        (1 << 5)
#define SDHCI_HCTRL_CD_TEST_INS	        (1 << 6)
#define SDHCI_HCTRL_CD_TEST	        (1 << 7)
#define SDHCI_POWER_CONTROL             0x29
#define SDHCI_POWER_ON                  (1 << 0)
#define SDHCI_POWER_180                 (1 << 3) | (1 << 1)
#define SDHCI_POWER_300                 (1 << 3) | (1 << 2)
#define SDHCI_POWER_330                 (1 << 3) | (1 << 2) | (1 << 1)
#define SDHCI_BLOCK_GAP_CONTROL         0x2A
#define SDHCI_WAKE_UP_CONTROL           0x2B
#define SDHCI_WAKE_ONINT	        (1 << 0)
#define SDHCI_WAKE_ONINSERT             (1 << 1)
#define SDHCI_WAKE_ONREMOVE             (1 << 2)
#define SDHCI_CLOCK_CONTROL	        0x2C
#define SDHCI_CLOCK_INT_EN              (1 << 0)
#define SDHCI_CLOCK_INT_STABLE          (1 << 1)
#define SDHCI_CLOCK_CARD_EN             (1 << 2)
#define SDHCI_DIVIDER_SHIFT             (8)
#define SDHCI_DIVIDER_HI_SHIFT	        (6)
#define SDHCI_DIV_MASK                  (0xFF)
#define SDHCI_DIV_MASK_LEN              (8)
#define SDHCI_DIV_HI_MASK               (3 << 8)
#define SDHCI_TIMEOUT_CONTROL	        0x2E
#define SDHCI_RESET	                0x2F
#define SDHCI_RESET_ALL                 (1 << 0)
#define SDHCI_RESET_CMD                 (1 << 1)
#define SDHCI_RESET_DATA                (1 << 2)
#define SDHCI_INT_STATUS	        0x30
#define SDHCI_INT_ENABLE	        0x34
#define SDHCI_SIGNAL_ENABLE	        0x38
#define SDHCI_INT_RESPONSE              (1 << 0)
#define SDHCI_INT_DATA_END              (1 << 2)
#define SDHCI_INT_DMA_END               (1 << 3)
#define SDHCI_INT_SPACE_AVAIL           (1 << 4)
#define SDHCI_INT_DATA_AVAIL            (1 << 5)
#define SDHCI_INT_CARD_INSERT           (1 << 6)
#define SDHCI_INT_CARD_REMOVE           (1 << 7)
#define SDHCI_INT_CARD_INT              (1 << 8)
#define SDHCI_INT_ERROR                 (1 << 15)
#define SDHCI_INT_TIMEOUT               (1 << 16)
#define SDHCI_INT_CRC                   (1 << 17)
#define SDHCI_INT_END_BIT               (1 << 18)
#define SDHCI_INT_INDEX                 (1 << 19)
#define SDHCI_INT_DATA_TIMEOUT          (1 << 20)
#define SDHCI_INT_DATA_CRC              (1 << 21)
#define SDHCI_INT_DATA_END_BIT          (1 << 22)
#define SDHCI_INT_BUS_POWER             (1 << 23)
#define SDHCI_INT_ACMD12ERR             (1 << 24)
#define SDHCI_INT_ADMA_ERROR            (1 << 25)
#define SDHCI_INT_CMD_MASK              (SDHCI_INT_RESPONSE|SDHCI_INT_TIMEOUT|SDHCI_INT_CRC|SDHCI_INT_END_BIT|SDHCI_INT_INDEX)
#define SDHCI_INT_DATA_MASK             (SDHCI_INT_DATA_END|SDHCI_INT_DMA_END|SDHCI_INT_DATA_AVAIL|SDHCI_INT_SPACE_AVAIL|SDHCI_INT_DATA_TIMEOUT|SDHCI_INT_DATA_CRC|SDHCI_INT_DATA_END_BIT|SDHCI_INT_ADMA_ERROR)
#define SDHCI_INT_ALL_MASK              (0xFFFFFFFF)
#define SDHCI_INT_NORMAL_MASK           (0x00007FFF)
#define SDHCI_INT_ERROR_MASK            (0xFFFF8000)
#define SDHCI_ACMD12_ERR                0x3C
#define SDHCI_CAPABILITIES              0x40
#define SDHCI_TIMEOUT_CLK_MASK	        (0x3F)
#define SDHCI_TIMEOUT_CLK_SHIFT         (0)
#define SDHCI_TIMEOUT_CLK_UNIT	        (1 << 7)
#define SDHCI_CLOCK_BASE_SHIFT          (8)
#define SDHCI_CLOCK_BASE_MASK	        (0x3F << SDHCI_CLOCK_BASE_SHIFT)
#define SDHCI_CLOCK_V3_BASE_MASK        (0xFF << SDHCI_CLOCK_BASE_SHIFT)
#define SDHCI_MAX_BLOCK_SHIFT           (16)
#define SDHCI_MAX_BLOCK_MASK            (0x03 << SDHCI_MAX_BLOCK_SHIFT)
#define SDHCI_CAN_DO_8BIT               (1 << 18)
#define SDHCI_CAN_DO_ADMA2              (1 << 19)
#define SDHCI_CAN_DO_ADMA1              (1 << 20)
#define SDHCI_CAN_DO_HISPD              (1 << 21)
#define SDHCI_CAN_DO_SDMA               (1 << 22)
#define SDHCI_CAN_VDD_330               (1 << 24)
#define SDHCI_CAN_VDD_300               (1 << 25)
#define SDHCI_CAN_VDD_180               (1 << 26)
#define SDHCI_CAN_64BIT                 (1 << 27)
#define SDHCI_CAPABILITIES1             0x44
#define SDHCI_MAX_CURRENT               0x48
#define SDHCI_SET_ACMD12_ERROR          0x50
#define SDHCI_SET_INT_ERROR             0x52
#define SDHCI_ADMA_ERROR                0x54
#define SDHCI_ADMA_ADDRESS              0x58
#define SDHCI_SLOT_INT_STATUS           0xFC
#define SDHCI_HOST_VERSION              0xFE
#define SDHCI_HVERS_SPEC_SHIFT          (0)
#define SDHCI_HVERS_SPEC_MASK           (0xFF)
#define SDHCI_HVERS_VENDVER_SHIFT       (8)
#define SDHCI_HVERS_VENDVER_MASK        (SDHCI_HVERS_SPEC_MASK << SDHCI_HVERS_VENDVER_SHIFT)
#define SDHCI_HVERS_SPEC100             (0)
#define SDHCI_HVERS_SPEC200             (1 << 0)
#define SDHCI_HVERS_SPEC300             (1 << 2)

/* SDCard OCR Values */
#define OCR_S18R                        (1 << 24) // 1.8v switching
#define OCR_XPC                         (1 << 28) // SDXC power control
#define OCR_FASTBOOT                    (1 << 29)
#define OCR_HCS                         (1 << 30) // High-Capacity support
#define OCR_BUSY                        (1 << 31)

#define SD_SCR_HIGHSPEED                (1 << 17)
#define SD_SCR_DATA4BIT                 (1 << 18)

/* SDCard Commands */
#define SD_CMD_SEND_RELATIVE_ADDR       3
#define SD_CMD_SWITCH_FUNC              6
#define SD_CMD_SEND_IF_COND             8

#define SD_CMD_APP_SET_BUS_WIDTH        6
#define SD_CMD_ERASE_WR_BLK_START       32
#define SD_CMD_ERASE_WR_BLK_END         33
#define SD_CMD_APP_SEND_OP_COND         41
#define SD_CMD_APP_SEND_SCR             51

#endif /* _SDHC_H */
