/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License");
you may not use this file except in compliance with the License. You may obtain a copy of the License at 
http://www.aros.org/license.html 
Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF 
ANY KIND, either express or implied. See the License for the specific language governing rights and 
limitations under the License. 

The Original Code is written by Davy Wentzler.
*/

#ifndef AHI_Drivers_misc_h
#define AHI_Drivers_misc_h

#include <config.h>

#include <devices/ahi.h>
#include <DriverData.h>
#include <stddef.h>

void WritePartialMask(struct PCIDevice *dev, struct CardData* card, unsigned long reg, unsigned long shift, unsigned long mask, unsigned long val);
void ClearMask(struct PCIDevice *dev, struct CardData* card, unsigned long reg, unsigned long mask);
void WriteMask(struct PCIDevice *dev, struct CardData* card, unsigned long reg, unsigned long mask);
void cmimix_wr(struct PCIDevice *dev, struct CardData* card, unsigned char port, unsigned char val);
unsigned char cmimix_rd(struct PCIDevice *dev, struct CardData* card, unsigned char port);


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
Linear2RecordGain( Fixed  linear,
		   UWORD* bits );

ULONG
SamplerateToLinearPitch( ULONG samplingrate );

void *pci_alloc_consistent(size_t size, APTR *NonAlignedAddress);

void pci_free_consistent(void* addr);

#endif /* AHI_Drivers_misc_h */
