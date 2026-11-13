/*

Copyright (C) 2001-2011 Neil Cafferkey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/


#include "device.h"

#include "eeprom_protos.h"
#include "timer_protos.h"
#include "realtek8187.h"


static ULONG ReadEEPROMBits(struct DevUnit *unit, UBYTE count,
   struct DevBase *base);
static VOID WriteEEPROMBits(struct DevUnit *unit, ULONG value, UBYTE count,
   struct DevBase *base);
static BOOL ReadEEPROMBit(struct DevUnit *unit, struct DevBase *base);
static BOOL WriteEEPROMBit(struct DevUnit *unit, BOOL is_one,
   struct DevBase *base);


/****i* realtek8180.device/GetEEPROMAddressSize ****************************
*
*   NAME
*	GetEEPROMAddressSize
*
*   SYNOPSIS
*	size = GetEEPROMAddressSize(unit)
*
*	UWORD GetEEPROMAddressSize(struct DevUnit *);
*
*   INPUTS
*	unit - A unit of this device.
*
*   RESULT
*	size - Width of EEPROM addresses.
*
****************************************************************************
*
*/

#if 0
static UWORD GetEEPROMAddressSize(struct DevUnit *unit,
   struct DevBase *base)
{
   UWORD size;

   unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM, R8180REG_EEPROMF_SELECT);
   WriteEEPROMBits(unit, 0x6, 3, base);
   for(size = 1; WriteEEPROMBit(unit, FALSE, base); size++);
   ReadEEPROMBits(unit, 16, base);
   unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM, 0);
   BusyMicroDelay(1, base);

   return size;
}
#endif



/****i* realtek8180.device/ReadEEPROM **************************************
*
*   NAME
*	ReadEEPROM -- Read an EEPROM location.
*
*   SYNOPSIS
*	value = ReadEEPROM(unit, index)
*
*	UWORD ReadEEPROM(struct DevUnit *, UWORD);
*
*   INPUTS
*	unit - A unit of this device.
*	index - Offset within EEPROM.
*
*   RESULT
*	value - Contents of specified EEPROM location.
*
****************************************************************************
*
*/

UWORD ReadEEPROM(struct DevUnit *unit, UWORD index,
   struct DevBase *base)
{
   UWORD value;

   unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM,
      R8180ECMD_PROGRAM | R8180REG_EEPROMF_SELECT);
   WriteEEPROMBits(unit, 0x6, 3, base);
   WriteEEPROMBits(unit, index, unit->eeprom_addr_size, base);
   value = ReadEEPROMBits(unit, 16, base);
   unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM, R8180ECMD_PROGRAM);
   BusyMicroDelay(1, base);

   return value;
}



/****i* realtek8180.device/ReadEEPROMBits **********************************
*
*   NAME
*	ReadEEPROMBits -- Read a stream of bits from the EEPROM.
*
*   SYNOPSIS
*	value = ReadEEPROMBits(unit, count)
*
*	ULONG ReadEEPROMBits(struct DevUnit *, UBYTE);
*
*   INPUTS
*	unit - A unit of this device.
*	count - Number of bits to be read.
*
*   RESULT
*	value - The bits read from the EEPROM, right-justified.
*
****************************************************************************
*
*/

static ULONG ReadEEPROMBits(struct DevUnit *unit, UBYTE count,
   struct DevBase *base)
{
   UBYTE i;
   ULONG value = 0;

   for(i = 0; i < count; i++)
   {
      value <<= 1;
      if(ReadEEPROMBit(unit, base))
         value |= 0x1;
   }

   return value;
}


/****i* realtek8180.device/WriteEEPROMBits *********************************
*
*   NAME
*	WriteEEPROMBits -- Write a stream of bits to the EEPROM.
*
*   SYNOPSIS
*	WriteEEPROMBits(unit, value, count)
*
*	VOID WriteEEPROMBits(struct DevUnit *, ULONG, UBYTE);
*
*   INPUTS
*	unit - A unit of this device.
*	value - The bits to write to the EEPROM, right-justified.
*	count - Number of bits to be Write.
*
*   RESULT
*	None.
*
****************************************************************************
*
*/

static VOID WriteEEPROMBits(struct DevUnit *unit, ULONG value, UBYTE count,
   struct DevBase *base)
{
   ULONG mask;

   for(mask = 1 << (count - 1); mask != 0; mask >>= 1)
      WriteEEPROMBit(unit, (value & mask) != 0, base);

   return;
}



/****i* realtek8180.device/ReadEEPROMBit ***********************************
*
*   NAME
*	ReadEEPROMBit -- Read a bit from the EEPROM.
*
*   SYNOPSIS
*	value = ReadEEPROMBit(unit)
*
*	BOOL ReadEEPROMBit(struct DevUnit *);
*
*   INPUTS
*	unit - A unit of this device.
*
*   RESULT
*	value - True for one, false for zero.
*
****************************************************************************
*
*/

static BOOL ReadEEPROMBit(struct DevUnit *unit, struct DevBase *base)
{
   BOOL is_one;

   unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM,
      R8180ECMD_PROGRAM | R8180REG_EEPROMF_SELECT | R8180REG_EEPROMF_CLK);
   BusyMicroDelay(2, base);
   is_one =
      (unit->ByteIn(unit->card, 0x100 + R8180REG_EEPROM)
      & R8180REG_EEPROMF_DATAIN) != 0;
   unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM,
      R8180ECMD_PROGRAM | R8180REG_EEPROMF_SELECT);
   BusyMicroDelay(2, base);

   return is_one;
}



/****i* realtek8180.device/WriteEEPROMBit **********************************
*
*   NAME
*	WriteEEPROMBit -- Write a bit to the EEPROM.
*
*   SYNOPSIS
*	data_in = WriteEEPROMBit(unit, is_one)
*
*	BOOL WriteEEPROMBit(struct DevUnit *, BOOL);
*
*   INPUTS
*	unit - A unit of this device.
*	is_one - True if a set bit should be written.
*
*   RESULT
*	data_in - True if data-in bit is set.
*
****************************************************************************
*
*/

static BOOL WriteEEPROMBit(struct DevUnit *unit, BOOL is_one,
   struct DevBase *base)
{
   UWORD data_out;

   if(is_one)
      data_out = R8180REG_EEPROMF_DATAOUT;
   else
      data_out = 0;

   unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM,
      R8180ECMD_PROGRAM | R8180REG_EEPROMF_SELECT | data_out);
   unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM, R8180ECMD_PROGRAM
      | R8180REG_EEPROMF_SELECT | R8180REG_EEPROMF_CLK | data_out);
   BusyMicroDelay(2, base);
   unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM,
      R8180ECMD_PROGRAM | R8180REG_EEPROMF_SELECT | data_out);
   BusyMicroDelay(2, base);

   return (unit->ByteIn(unit->card, 0x100 + R8180REG_EEPROM)
      & R8180REG_EEPROMF_DATAIN) != 0;
}



