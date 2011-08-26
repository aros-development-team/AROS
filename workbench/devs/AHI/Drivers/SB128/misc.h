/*

The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

The Original Code is (C) Copyright 2004-2011 Ross Vumbaca.

The Initial Developer of the Original Code is Ross Vumbaca.

All Rights Reserved.

*/

#ifndef AHI_Drivers_misc_h
#define AHI_Drivers_h

//#include <config.h>

#include <devices/ahi.h>
#include <DriverData.h>
#include <stddef.h>

void codec_write(struct SB128_DATA *card, unsigned short reg, unsigned short val);
unsigned short codec_read(struct SB128_DATA *card, unsigned short reg);

void ak4531_ac97_write(struct SB128_DATA *card, unsigned short reg, unsigned short val);

struct SB128_DATA*
AllocDriverData( struct PCIDevice*    dev,
		 struct DriverBase* AHIsubBase );

void
FreeDriverData( struct SB128_DATA* card,
		struct DriverBase*  AHIsubBase );

void
SaveMixerState( struct SB128_DATA* card );

void
RestoreMixerState( struct SB128_DATA* card );

void
UpdateMonitorMixer( struct SB128_DATA* card );

Fixed
Linear2MixerGain( Fixed  linear,
		  UWORD* bits );

Fixed
Linear2RecordGain( Fixed  linear,
		   UWORD* bits );

ULONG
SamplerateToLinearPitch( ULONG samplingrate );

void *pci_alloc_consistent(size_t size, APTR *NonAlignedAddress, unsigned int boundary);

void pci_free_consistent(void* addr);

#endif /* AHI_Drivers_misc_h */
