/*
    Copyright � 2004-2014, Davy Wentzler. All rights reserved.
    $Id$
*/

#ifndef AHI_Drivers_misc_h
#define AHI_Drivers_misc_h

#include <config.h>

#include <devices/ahi.h>
#include <DriverData.h>
#include <stddef.h>

void ClearMask8(struct CardData *card, unsigned char reg, unsigned char mask);
void WriteMask8(struct CardData *card, unsigned char reg, unsigned char mask);

void MicroDelay(unsigned int val);

unsigned char ReadCCI(struct CardData *card, unsigned char address);
void WriteCCI(struct CardData *card, unsigned char address, unsigned char data);
unsigned char GetGPIOData(struct CardData *card);
void SetGPIOData(struct CardData *card, unsigned char data);
void SaveGPIOStatus(struct CardData *card);
void RestoreGPIOStatus(struct CardData *card);

void codec_write(struct CardData *card, unsigned short reg, unsigned short val);
unsigned short codec_read(struct CardData *card, unsigned short reg);


struct CardData*
AllocDriverData( struct PCIDevice*    dev,
		 struct DriverBase* AHIsubBase );

void
FreeDriverData( struct CardData* card,
		struct DriverBase*  AHIsubBase );

Fixed
Linear2MixerGain( Fixed  linear,
		  UWORD* bits );

Fixed
Linear2RecordGain( Fixed  linear,
		   UWORD* bits );

ULONG
SamplerateToLinearPitch( ULONG samplingrate );

void *pci_alloc_consistent(size_t size, APTR *NonAlignedAddress,
  struct DriverBase* AHIsubBase);
void pci_free_consistent(void* addr, struct DriverBase* AHIsubBase);

#endif /* AHI_Drivers_misc_h */
