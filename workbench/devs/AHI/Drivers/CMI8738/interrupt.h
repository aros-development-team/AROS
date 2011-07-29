/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License");
you may not use this file except in compliance with the License. You may obtain a copy of the License at 
http://www.aros.org/license.html 
Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF 
ANY KIND, either express or implied. See the License for the specific language governing rights and 
limitations under the License. 

The Original Code is written by Davy Wentzler.
*/

#ifndef AHI_Drivers_interrupt_h
#define AHI_Drivers_interrupt_h

//#include <config.h>

#include "DriverData.h"

LONG
CardInterrupt( struct CMI8738_DATA* dd );

void
PlaybackInterrupt( struct CMI8738_DATA* dd );

void
RecordInterrupt( struct CMI8738_DATA* dd );

#endif /* AHI_Drivers_interrupt_h */
