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

#ifndef AHI_Drivers_SB128_interrupt_h
#define AHI_Drivers_SB128_interrupt_h

//#include <config.h>

#include "DriverData.h"

LONG
CardInterrupt(  struct SB128_DATA* dd );

void
PlaybackInterrupt(  struct SB128_DATA* dd );

void
RecordInterrupt(  struct SB128_DATA* dd );

#endif /* AHI_Drivers_SB128_interrupt_h */
