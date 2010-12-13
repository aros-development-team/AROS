/*
     emu10kx.audio - AHI driver for SoundBlaster Live! series
     Copyright (C) 2002-2005 Martin Blom <martin@blom.org>
     
     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.
     
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.
     
     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef AHI_Drivers_EMU10kx_linuxsupport_h
#define AHI_Drivers_EMU10kx_linuxsupport_h

#include <config.h>

#include <stdlib.h>
#include <proto/exec.h>

typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;

typedef signed int     s32;
typedef signed short   s16;
typedef signed char    s8;

typedef unsigned int   dma_addr_t;

#define __devinit

#define spin_lock_irqsave(lock,flags) Disable();
#define spin_unlock_irqrestore(lock,flags) Enable();

#define PAGE_SIZE      4096
#define GFP_KERNEL     0               // Never used; the value doesn't matter

#define KERN_NOTICE    ""
#define printk         printf

#ifndef __AMIGAOS4__
#define PCI_ANY_ID                     0xffff
#endif
#define PCI_VENDOR_ID_CREATIVE         0x1102
#define PCI_DEVICE_ID_CREATIVE_EMU10K1 0x0002
#define PCI_DEVICE_ID_CREATIVE_AUDIGY  0x0004
#define EMU_APS_SUBID                  0x40011102

#define SOUND_MIXER_NRDEVICES 25
#define SOUND_MIXER_VOLUME    0
#define SOUND_MIXER_PCM       4
#define SOUND_MIXER_IGAIN    12
#define SOUND_MIXER_OGAIN    13
#define SOUND_MIXER_DIGITAL1 17

#define SOUND_MASK_OGAIN     (1 << SOUND_MIXER_OGAIN)
#define SOUND_MASK_DIGITAL1  (1 << SOUND_MIXER_DIGITAL1)

#define AC97_RESET               0x0000
#define AC97_MASTER_VOL_STEREO   0x0002
#define AC97_HEADPHONE_VOL       0x0004
#define AC97_MASTER_VOL_MONO     0x0006
#define AC97_MASTER_TONE         0x0008
#define AC97_PCBEEP_VOL          0x000a
#define AC97_PHONE_VOL           0x000c
#define AC97_MIC_VOL             0x000e
#define AC97_LINEIN_VOL          0x0010
#define AC97_CD_VOL              0x0012
#define AC97_VIDEO_VOL           0x0014
#define AC97_AUX_VOL             0x0016
#define AC97_PCMOUT_VOL          0x0018
#define AC97_RECORD_SELECT       0x001a
#define AC97_RECORD_GAIN         0x001c

#define AC97_EXTENDED_ID         0x0028
#define AC97_SURROUND_MASTER     0x0038

#define AC97_MUTE                0x8000

#define AC97_RECMUX_MIC          0x0000
#define AC97_RECMUX_CD           0x0101
#define AC97_RECMUX_VIDEO        0x0202
#define AC97_RECMUX_AUX          0x0303
#define AC97_RECMUX_LINE         0x0404
#define AC97_RECMUX_STEREO_MIX   0x0505
#define AC97_RECMUX_MONO_MIX     0x0606
#define AC97_RECMUX_PHONE        0x0707


unsigned long
__get_free_page( unsigned int gfp_mask );
  
void
free_page( unsigned long addr );

void*
pci_alloc_consistent( void* pci_dev, size_t size, dma_addr_t* dma_handle );

void
pci_free_consistent( void* pci_dev, size_t size, void* arrd, dma_addr_t dma_handle );


static __inline__ int test_bit(int nr, const volatile void * addr)
{
  return ((1UL << (nr & 31)) & (((const volatile unsigned int *) addr)[nr >> 5])) != 0;
}

static __inline__ void set_bit(int nr, volatile void * addr)
{
  (((volatile unsigned int *) addr)[nr >> 5]) |= (1UL << (nr & 31));
}

#ifdef WORDS_BIGENDIAN
static __inline__ u32 cpu_to_le32( u32 x )
{
  u32 res = ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
	     (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24));

  return res;
}
#else
#define cpu_to_le32(x)  x
#endif

#endif /* AHI_Drivers_EMU10kx_linuxsupport_h */
