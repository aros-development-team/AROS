/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AHI_Drivers_misc_h
#define AHI_Drivers_misc_h

#include <config.h>

#include <devices/ahi.h>
#include <DriverData.h>
#include <stddef.h>


#define udelay MicroDelay
#define snd_printk DebugPrintF
#ifdef __amigaos4__
#define outl(a, b) dev->OutLong(b, a)
#define outb(a, b) dev->OutByte(b, a)
#endif


struct CardData*
AllocDriverData( struct PCIDevice*    dev,
		 struct DriverBase* AHIsubBase );

void
FreeDriverData( struct CardData* dd,
		struct DriverBase*  AHIsubBase );

void
SaveMixerState( struct CardData* dd );

void
RestoreMixerState( struct CardData* dd );

void
UpdateMonitorMixer( struct CardData* dd );

Fixed
Linear2MixerGain( Fixed  linear,
		  UWORD* bits );

Fixed
Linear2RecordGain( Fixed  linear,
		   UWORD* bits );

ULONG
SamplerateToLinearPitch( ULONG samplingrate );

void *pci_alloc_consistent(size_t size, APTR *NonAlignedAddress);

void pci_free_consistent(void* addr);

void MicroDelay(unsigned int val);
void channel_reset(struct CardData *card);

unsigned int codec_xread(struct CardData *card);
void codec_xwrite(struct CardData *card, unsigned int val);
int codec_ready(struct CardData *card);
int codec_valid(struct CardData *card);
void codec_wait(struct CardData *card);
void codec_write(struct CardData *card, unsigned short reg, unsigned short val);
unsigned short codec_read(struct CardData *card, unsigned char reg);

BOOL ac97_read_reg(struct CardData *card, unsigned char reg, unsigned short *data );


#endif /* AHI_Drivers_misc_h */
