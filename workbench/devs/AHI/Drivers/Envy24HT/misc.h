/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    $Id$
*/

#ifndef AHI_Drivers_Envy24HT_misc_h
#define AHI_Drivers_Envy24HT_misc_h

#include <config.h>

#include <devices/ahi.h>
#include <DriverData.h>
#include <stddef.h>

unsigned long GetGPIOData(struct CardData *card, unsigned long base);
void SetGPIOData(struct CardData *card, unsigned long base, unsigned long data);
void SetGPIOMask(struct CardData *card, unsigned long base, unsigned long data);

void ClearMask8(struct CardData *card, unsigned long base, unsigned char reg, unsigned char mask);
void WriteMask8(struct CardData *card, unsigned long base, unsigned char reg, unsigned char mask);


void WritePartialMask(struct CardData *card, unsigned long base, unsigned char reg, unsigned long shift, unsigned long mask, unsigned long val);
void MicroDelay(unsigned int val);

void revo_i2s_mclk_changed(struct CardData *card);
void codec_write(struct CardData *card, unsigned short reg, unsigned short val);
unsigned short codec_read(struct CardData *card, unsigned short reg);
void wm_put(struct CardData *card, unsigned long base, unsigned short reg, unsigned short val);
void update_spdif_bits(struct CardData *card, unsigned short val);
void update_spdif_rate(struct CardData *card, unsigned short rate);

void WriteI2C(struct PCIDevice *dev, struct CardData *card, unsigned chip_address, unsigned char reg, unsigned char data);

void SaveGPIO(struct PCIDevice *dev, struct CardData* card);
void RestoreGPIO(struct PCIDevice *dev, struct CardData* card);
void SetGPIODir(struct PCIDevice *dev, struct CardData* card, unsigned long data);


struct CardData*
AllocDriverData( struct PCIDevice*    dev,
		 struct DriverBase* AHIsubBase );

void
FreeDriverData( struct CardData* card,
		struct DriverBase*  AHIsubBase );

void
SaveMixerState( struct CardData* card );

void
RestoreMixerState( struct CardData* card );

void
UpdateMonitorMixer( struct CardData* card );

Fixed
Linear2MixerGain( Fixed  linear,
		  UWORD* bits );

Fixed
Linear2AKMGain( Fixed  linear,
		  UWORD* bits );

Fixed
Linear2RecordGain( Fixed  linear,
		   UWORD* bits );

ULONG
SamplerateToLinearPitch( ULONG samplingrate );

void *pci_alloc_consistent(size_t size, APTR *NonAlignedAddress, unsigned int boundary);

void pci_free_consistent(void* addr);

#endif /* AHI_Drivers_Envy24HT_misc_h */
