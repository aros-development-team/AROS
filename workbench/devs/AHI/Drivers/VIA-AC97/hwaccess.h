/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _HWACCESS_H
#define _HWACCESS_H

#define VIA_TABLE_SIZE  255

#define RECORD 0x10

/* common offsets */
#define VIA_REG_OFFSET_STATUS           0x00    /* byte - channel status */
#define   VIA_REG_STAT_ACTIVE           0x80    /* RO */
#define   VIA_REG_STAT_PAUSED           0x40    /* RO */
#define   VIA_REG_STAT_TRIGGER_QUEUED   0x08    /* RO */
#define   VIA_REG_STAT_STOPPED          0x04    /* RWC */
#define   VIA_REG_STAT_EOL              0x02    /* RWC */
#define   VIA_REG_STAT_FLAG             0x01    /* RWC */
#define VIA_REG_OFFSET_CONTROL          0x01    /* byte - channel control */
#define   VIA_REG_CTRL_START            0x80    /* WO */
#define   VIA_REG_CTRL_TERMINATE        0x40    /* WO */
#define   VIA_REG_CTRL_AUTOSTART        0x20
#define   VIA_REG_CTRL_PAUSE            0x08    /* RW */
#define   VIA_REG_CTRL_INT_STOP         0x04            
#define   VIA_REG_CTRL_INT_EOL          0x02
#define   VIA_REG_CTRL_INT_FLAG         0x01
#define   VIA_REG_CTRL_RESET            0x01    /* RW - probably reset? undocumented */
#define   VIA_REG_CTRL_INT (VIA_REG_CTRL_INT_FLAG | VIA_REG_CTRL_INT_EOL | VIA_REG_CTRL_AUTOSTART)
#define VIA_REG_OFFSET_TYPE             0x02    /* byte - channel type (686 only) */
#define   VIA_REG_TYPE_AUTOSTART        0x80    /* RW - autostart at EOL */
#define   VIA_REG_TYPE_16BIT            0x20    /* RW */
#define   VIA_REG_TYPE_STEREO           0x10    /* RW */
#define   VIA_REG_TYPE_INT_LLINE        0x00
#define   VIA_REG_TYPE_INT_LSAMPLE      0x04
#define   VIA_REG_TYPE_INT_LESSONE      0x08
#define   VIA_REG_TYPE_INT_MASK         0x0c
#define   VIA_REG_TYPE_INT_EOL          0x02
#define   VIA_REG_TYPE_INT_FLAG         0x01
#define VIA_REG_OFFSET_TABLE_PTR        0x04    /* dword - channel table pointer */
#define VIA_REG_OFFSET_CURR_PTR         0x04    /* dword - channel current pointer */
#define VIA_REG_OFFSET_STOP_IDX         0x08    /* dword - stop index, channel type, sample rate */
#define   VIA8233_REG_TYPE_16BIT        0x00200000      /* RW */
#define   VIA8233_REG_TYPE_STEREO       0x00100000      /* RW */
#define VIA_REG_OFFSET_CURR_COUNT       0x0c    /* dword - channel current count (24 bit) */
#define VIA_REG_OFFSET_CURR_INDEX       0x0f    /* byte - channel current index (for via8233 only) */




/* AC'97 */
#define VIA_REG_AC97                    0x80    /* dword */
#define   VIA_REG_AC97_CODEC_ID_MASK    (3<<30)
#define   VIA_REG_AC97_CODEC_ID_SHIFT   30
#define   VIA_REG_AC97_CODEC_ID_PRIMARY 0x00
#define   VIA_REG_AC97_CODEC_ID_SECONDARY 0x01
#define   VIA_REG_AC97_SECONDARY_VALID  (1<<27)
#define   VIA_REG_AC97_PRIMARY_VALID    (1<<25)
#define   VIA_REG_AC97_BUSY             (1<<24)
#define   VIA_REG_AC97_READ             (1<<23)
#define   VIA_REG_AC97_CMD_SHIFT        16
#define   VIA_REG_AC97_CMD_MASK         0x7e
#define   VIA_REG_AC97_DATA_SHIFT       0
#define   VIA_REG_AC97_DATA_MASK        0xffff

#define VIA_REG_SGD_SHADOW              0x84    /* dword */
/* via686 */
#define   VIA_REG_SGD_STAT_PB_FLAG      (1<<0)
#define   VIA_REG_SGD_STAT_CP_FLAG      (1<<1)
#define   VIA_REG_SGD_STAT_FM_FLAG      (1<<2)
#define   VIA_REG_SGD_STAT_PB_EOL       (1<<4)
#define   VIA_REG_SGD_STAT_CP_EOL       (1<<5)
#define   VIA_REG_SGD_STAT_FM_EOL       (1<<6)
#define   VIA_REG_SGD_STAT_PB_STOP      (1<<8)
#define   VIA_REG_SGD_STAT_CP_STOP      (1<<9)
#define   VIA_REG_SGD_STAT_FM_STOP      (1<<10)
#define   VIA_REG_SGD_STAT_PB_ACTIVE    (1<<12)
#define   VIA_REG_SGD_STAT_CP_ACTIVE    (1<<13)
#define   VIA_REG_SGD_STAT_FM_ACTIVE    (1<<14)
/* via8233 */
#define   VIA8233_REG_SGD_STAT_FLAG     (1<<0)
#define   VIA8233_REG_SGD_STAT_EOL      (1<<1)
#define   VIA8233_REG_SGD_STAT_STOP     (1<<2)
#define   VIA8233_REG_SGD_STAT_ACTIVE   (1<<3)
#define VIA8233_INTR_MASK(chan) ((VIA8233_REG_SGD_STAT_FLAG|VIA8233_REG_SGD_STAT_EOL) << ((chan) * 4))
#define   VIA8233_REG_SGD_CHAN_SDX      0
#define   VIA8233_REG_SGD_CHAN_MULTI    4
#define   VIA8233_REG_SGD_CHAN_REC      6
#define   VIA8233_REG_SGD_CHAN_REC1     7

#define VIA_REG_GPI_STATUS              0x88
#define VIA_REG_GPI_INTR                0x8c

/* multi-channel and capture registers for via8233 */
//DEFINE_VIA_REGSET(MULTPLAY, 0x40);
//DEFINE_VIA_REGSET(CAPTURE_8233, 0x60);

/* via8233-specific registers */
#define VIA_REG_OFS_PLAYBACK_VOLUME_L   0x02    /* byte */
#define VIA_REG_OFS_PLAYBACK_VOLUME_R   0x03    /* byte */
#define VIA_REG_OFS_MULTPLAY_FORMAT     0x02    /* byte - format and channels */
#define   VIA_REG_MULTPLAY_FMT_8BIT     0x00
#define   VIA_REG_MULTPLAY_FMT_16BIT    0x80
#define   VIA_REG_MULTPLAY_FMT_CH_MASK  0x70    /* # channels << 4 (valid = 1,2,4,6) */
#define VIA_REG_OFS_CAPTURE_FIFO        0x02    /* byte - bit 6 = fifo  enable */
#define   VIA_REG_CAPTURE_FIFO_ENABLE   0x40

#define VIA_DXS_MAX_VOLUME              31      /* max. volume (attenuation) of reg 0x32/33 */

#define VIA_REG_CAPTURE_CHANNEL         0x63    /* byte - input select */
#define   VIA_REG_CAPTURE_CHANNEL_MIC   0x4
#define   VIA_REG_CAPTURE_CHANNEL_LINE  0
#define   VIA_REG_CAPTURE_SELECT_CODEC  0x03    /* recording source codec (0 = primary) */

#define VIA_TBL_BIT_FLAG        0x40000000
#define VIA_TBL_BIT_EOL         0x80000000

/* pci space */
#define VIA_ACLINK_STAT         0x40
#define  VIA_ACLINK_C11_READY   0x20
#define  VIA_ACLINK_C10_READY   0x10
#define  VIA_ACLINK_C01_READY   0x04 /* secondary codec ready */
#define  VIA_ACLINK_LOWPOWER    0x02 /* low-power state */
#define  VIA_ACLINK_C00_READY   0x01 /* primary codec ready */
#define VIA_ACLINK_CTRL         0x41
#define  VIA_ACLINK_CTRL_ENABLE 0x80 /* 0: disable, 1: enable */
#define  VIA_ACLINK_CTRL_RESET  0x40 /* 0: assert, 1: de-assert */
#define  VIA_ACLINK_CTRL_SYNC   0x20 /* 0: release SYNC, 1: force SYNC hi */
#define  VIA_ACLINK_CTRL_SDO    0x10 /* 0: release SDO, 1: force SDO hi */
#define  VIA_ACLINK_CTRL_VRA    0x08 /* 0: disable VRA, 1: enable VRA */
#define  VIA_ACLINK_CTRL_PCM    0x04 /* 0: disable PCM, 1: enable PCM */
#define  VIA_ACLINK_CTRL_FM     0x02 /* via686 only */
#define  VIA_ACLINK_CTRL_SB     0x01 /* via686 only */
#define  VIA_ACLINK_CTRL_INIT   (VIA_ACLINK_CTRL_ENABLE|\
                                 VIA_ACLINK_CTRL_RESET|\
                                 VIA_ACLINK_CTRL_PCM|\
                                 VIA_ACLINK_CTRL_VRA)
#define VIA_FUNC_ENABLE         0x42
#define  VIA_FUNC_MIDI_PNP      0x80 /* FIXME: it's 0x40 in the datasheet! */
#define  VIA_FUNC_MIDI_IRQMASK  0x40 /* FIXME: not documented! */
#define  VIA_FUNC_RX2C_WRITE    0x20
#define  VIA_FUNC_SB_FIFO_EMPTY 0x10
#define  VIA_FUNC_ENABLE_GAME   0x08
#define  VIA_FUNC_ENABLE_FM     0x04
#define  VIA_FUNC_ENABLE_MIDI   0x02
#define  VIA_FUNC_ENABLE_SB     0x01
#define VIA_PNP_CONTROL         0x43
#define VIA_FM_NMI_CTRL         0x48
#define VIA8233_VOLCHG_CTRL     0x48
#define VIA8233_SPDIF_CTRL      0x49
#define  VIA8233_SPDIF_DX3      0x08
#define  VIA8233_SPDIF_SLOT_MASK        0x03
#define  VIA8233_SPDIF_SLOT_1011        0x00
#define  VIA8233_SPDIF_SLOT_34          0x01
#define  VIA8233_SPDIF_SLOT_78          0x02
#define  VIA8233_SPDIF_SLOT_69          0x03

/*
 */

#define VIA_DXS_AUTO    0
#define VIA_DXS_ENABLE  1
#define VIA_DXS_DISABLE 2
#define VIA_DXS_48K     3
#define VIA_DXS_NO_VRA  4
#define VIA_DXS_SRC     5


#define VOL_6BIT 0x40
#define VOL_5BIT 0x20
#define VOL_4BIT 0x10


#endif  /* _HWACCESS_H */
