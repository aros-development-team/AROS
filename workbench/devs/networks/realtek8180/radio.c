/*

Copyright (C) 2001-2017 Neil Cafferkey

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

#include "radio_protos.h"
#include "eeprom_protos.h"
#include "timer_protos.h"
#include "realtek8187.h"


static UBYTE GetPowerPair(struct DevUnit *unit, UWORD index,
   struct DevBase *base);
static VOID WritePHY(struct DevUnit *unit, UWORD index, ULONG value,
   struct DevBase *base);


static const UWORD gain_table[] =
{
   0x0400, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0408, 0x0409,
   0x040a, 0x040b, 0x0502, 0x0503, 0x0504, 0x0505, 0x0540, 0x0541,
   0x0542, 0x0543, 0x0544, 0x0545, 0x0580, 0x0581, 0x0582, 0x0583,
   0x0584, 0x0585, 0x0588, 0x0589, 0x058a, 0x058b, 0x0643, 0x0644,
   0x0645, 0x0680, 0x0681, 0x0682, 0x0683, 0x0684, 0x0685, 0x0688,
   0x0689, 0x068a, 0x068b, 0x068c, 0x0742, 0x0743, 0x0744, 0x0745,
   0x0780, 0x0781, 0x0782, 0x0783, 0x0784, 0x0785, 0x0788, 0x0789,
   0x078a, 0x078b, 0x078c, 0x078d, 0x0790, 0x0791, 0x0792, 0x0793,
   0x0794, 0x0795, 0x0798, 0x0799, 0x079a, 0x079b, 0x079c, 0x079d,
   0x07a0, 0x07a1, 0x07a2, 0x07a3, 0x07a4, 0x07a5, 0x07a8, 0x07a9,
   0x03aa, 0x03ab, 0x03ac, 0x03ad, 0x03b0, 0x03b1, 0x03b2, 0x03b3,
   0x03b4, 0x03b5, 0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bb
};


static const UBYTE agc_table[] =
{
   0x5e, 0x5e, 0x5e, 0x5e, 0x5d, 0x5b, 0x59, 0x57,
   0x55, 0x53, 0x51, 0x4f, 0x4d, 0x4b, 0x49, 0x47,
   0x45, 0x43, 0x41, 0x3f, 0x3d, 0x3b, 0x39, 0x37,
   0x35, 0x33, 0x31, 0x2f, 0x2d, 0x2b, 0x29, 0x27,
   0x25, 0x23, 0x21, 0x1f, 0x1d, 0x1b, 0x19, 0x17,
   0x15, 0x13, 0x11, 0x0f, 0x0d, 0x0b, 0x09, 0x07,
   0x05, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19,
   0x19, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
   0x26, 0x27, 0x27, 0x28, 0x28, 0x29, 0x2a, 0x2a,
   0x2a, 0x2b, 0x2b, 0x2b, 0x2c, 0x2c, 0x2c, 0x2d,
   0x2d, 0x2d, 0x2d, 0x2e, 0x2e, 0x2e, 0x2e, 0x2f,
   0x2f, 0x2f, 0x30, 0x30, 0x31, 0x31, 0x31, 0x31,
   0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31,
   0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31
};


static const UBYTE ofdm_table[] =
{
   0x10, 0x0d, 0x01, 0x00, 0x14, 0xfb, 0xfb, 0x60,
   0x00, 0x60, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00,
   0x40, 0x00, 0x40, 0x00, 0x00, 0x00, 0xa8, 0x26,
   0x32, 0x33, 0x07, 0xa5, 0x6f, 0x55, 0xc8, 0xb3,
   0x0a, 0xe1, 0x2C, 0x8a, 0x86, 0x83, 0x34, 0x0f,
   0x4f, 0x24, 0x6f, 0xc2, 0x6b, 0x40, 0x80, 0x00,
   0xc0, 0xc1, 0x58, 0xf1, 0x00, 0xe4, 0x90, 0x3e,
   0x6d, 0x3c, 0xfb, 0x07
};


static UBYTE rtl8225_power_tables[][8] =
{
   {0x18, 0x17, 0x15, 0x11, 0x0c, 0x08, 0x04, 0x02},
   {0x1b, 0x1a, 0x17, 0x13, 0x0e, 0x09, 0x04, 0x02},
   {0x1f, 0x1e, 0x1a, 0x15, 0x10, 0x0a, 0x05, 0x02},
   {0x22, 0x21, 0x1d, 0x18, 0x11, 0x0b, 0x06, 0x02},
   {0x26, 0x25, 0x21, 0x1b, 0x14, 0x0d, 0x06, 0x03},
   {0x2b, 0x2a, 0x25, 0x1e, 0x16, 0x0e, 0x07, 0x03},
   {0x30, 0x2f, 0x29, 0x21, 0x19, 0x10, 0x08, 0x03},
   {0x36, 0x35, 0x2e, 0x25, 0x1c, 0x12, 0x09, 0x04}
};


static UBYTE rtl8225_ch14_power_tables[][8] =
{
   {0x18, 0x17, 0x15, 0x0c, 0x00, 0x00, 0x00, 0x00},
   {0x1b, 0x1a, 0x17, 0x0e, 0x00, 0x00, 0x00, 0x00},
   {0x1f, 0x1e, 0x1a, 0x0f, 0x00, 0x00, 0x00, 0x00},
   {0x22, 0x21, 0x1d, 0x11, 0x00, 0x00, 0x00, 0x00},
   {0x26, 0x25, 0x21, 0x13, 0x00, 0x00, 0x00, 0x00},
   {0x2b, 0x2a, 0x25, 0x15, 0x00, 0x00, 0x00, 0x00},
   {0x30, 0x2f, 0x29, 0x15, 0x00, 0x00, 0x00, 0x00},
   {0x36, 0x35, 0x2e, 0x1b, 0x00, 0x00, 0x00, 0x00}
};


static UBYTE rtl8225_ofdm_power[] = {0x80, 0x90, 0xa2, 0xb5, 0xcb, 0xe4};


static const UBYTE eeprom_power_locations_l[] =
{
   0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x7a, 0x7b,
   0x7c, 0x7d, 0x36, 0x37, 0x38, 0x39
};


static const UBYTE eeprom_power_locations_b[] =
{
   0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x7a, 0x7b,
   0x7c, 0x7d, 0x36, 0x14, 0x38, 0x39
};


/****i* realtek8180.device/InitialiseRadio *********************************
*
*   NAME
*       InitialiseRadio
*
*   SYNOPSIS
*       success = InitialiseRadio(unit, reinsertion)
*
*       BOOL InitialiseRadio(struct DevUnit *, BOOL);
*
*   FUNCTION
*
*   INPUTS
*       unit
*       reinsertion
*
*   RESULT
*       success - Success indicator.
*
****************************************************************************
*
*/

BOOL InitialiseRadio(struct DevUnit *unit, struct DevBase *base)
{
   BOOL success = TRUE;
   UWORD i;

   /* Get radio revision */

   if((LEWord(ReadEEPROM(unit, R8180ROM_RFCHIPID, base)) & 0xff) == 5)
   {
      unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT, 0x80);
      unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSSELECT, 0x80);
      unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSENABLE, 0x80);
      BusyMilliDelay(500, base);

      WriteRadio(unit, 0x0, 0x1b7, base);

      if(ReadRadio(unit, 0x8, base) != 0x588 ||
         ReadRadio(unit, 0x9, base) != 0x700)
         unit->radio_type = RTL8225_RADIO;
      else
         unit->radio_type = RTL8225V2_RADIO;
      WriteRadio(unit, 0x0, 0xb7, base);
   }
   else
      unit->radio_type = RTL8225V2B_RADIO;

   /* Initialise radio */

   WriteRadio(unit, 0x0, 0xb7, base);
   WriteRadio(unit, 0x1, 0xee0, base);
   WriteRadio(unit, 0x2, 0x44d, base);
   WriteRadio(unit, 0x3, 0x441, base);
   WriteRadio(unit, 0x4, 0x8c3, base);
   WriteRadio(unit, 0x5, 0xc72, base);
   WriteRadio(unit, 0x6, 0xe6, base);
   WriteRadio(unit, 0x7, 0x82a, base);
   WriteRadio(unit, 0x8, 0x3f, base);
   WriteRadio(unit, 0x9, 0x335, base);
   WriteRadio(unit, 0xa, 0x9d4, base);
   WriteRadio(unit, 0xb, 0x7bb, base);
   WriteRadio(unit, 0xc, 0x850, base);
   WriteRadio(unit, 0xd, 0xcdf, base);
   WriteRadio(unit, 0xe, 0x2b, base);
   WriteRadio(unit, 0xf, 0x114, base);
   WriteRadio(unit, 0x0, 0x1b7, base);

   for(i = 0; i < sizeof(gain_table) / sizeof(UWORD); i++)
   {
      WriteRadio(unit, 0x1, i + 1, base);
      WriteRadio(unit, 0x2, gain_table[i], base);
   }

   WriteRadio(unit, 0x3, 0x80, base);
   WriteRadio(unit, 0x5, 0x4, base);
   WriteRadio(unit, 0x0, 0xb7, base);
   BusyMilliDelay(100, base);

   WriteRadio(unit, 0x2, 0xc4d, base);
   WriteRadio(unit, 0x2, 0x44d, base);
   WriteRadio(unit, 0x0, 0x2bf, base);

   unit->ByteOut(unit->card, 0x100 + R8180REG_TXGAINCCK, 0x3);
   unit->ByteOut(unit->card, 0x100 + R8180REG_TXGAINOFDM, 0x7);
   unit->ByteOut(unit->card, 0x100 + R8180REG_TXANTENNA, 0x3);

   WriteOFDM(unit, 0x80, 0x12, base);
   for(i = 0; i < sizeof(agc_table); i++)
   {
      WriteOFDM(unit, 0xf, agc_table[i], base);
      WriteOFDM(unit, 0xe, 0x80 + i, base);
      WriteOFDM(unit, 0xe, 0, base);
   }
   WriteOFDM(unit, 0x80, 0x10, base);

   for(i = 0; i < sizeof(ofdm_table); i++)
      WriteOFDM(unit, i, ofdm_table[i], base);

   unit->LELongOut(unit->card, 0x100 + R8180REG_ACVO, (7 << 12) | (3 << 8) | 0x1c);
   unit->LELongOut(unit->card, 0x100 + R8180REG_ACVI, (7 << 12) | (3 << 8) | 0x1c);
   unit->LELongOut(unit->card, 0x100 + R8180REG_ACBE, (7 << 12) | (3 << 8) | 0x1c);
   unit->LELongOut(unit->card, 0x100 + R8180REG_ACBK, (7 << 12) | (3 << 8) | 0x1c);

   WriteOFDM(unit, 0x97, 0x46, base);
   WriteOFDM(unit, 0xa4, 0xb6, base);
   WriteOFDM(unit, 0x85, 0xfc, base);
   WriteCCK(unit, 0xc1, 0x88, base);

   /* Return */

   return success;
}



VOID GetPower(struct DevUnit *unit, struct DevBase *base)
{
   const UBYTE *locations;
   UBYTE value;
   UWORD i;

   if(unit->generation == RTL8187L_GEN)
      locations = eeprom_power_locations_l;
   else
      locations = eeprom_power_locations_b;

   for(i = 0; i < 14; i++)
   {
      value = GetPowerPair(unit, locations[i], base);
      unit->cck_power[i] = value & 0xf;
      unit->ofdm_power[i] = value >> 4;
   }
}



static UBYTE GetPowerPair(struct DevUnit *unit, UWORD index,
   struct DevBase *base)
{
   UWORD value;

   value = LEWord(ReadEEPROM(unit, index >> 1, base));
   if((index & 0x1) != 0)
      value >>= 8;

   return (UBYTE)value;
}



VOID SetPower(struct DevUnit *unit, struct DevBase *base)
{
   UBYTE power_level, tx_gain, table_no, n;
   const UBYTE *power_table;
   UWORD i;
   ULONG tx_conf, loop_tx_conf, channel_code;

   /* Get CCK power level and gain */

   power_level = unit->cck_power[unit->channel];
   if(unit->radio_type == RTL8225_RADIO)
   {
      if(power_level > 11)
         power_level = 11;
      table_no = power_level % 6;
      tx_gain = (2 << power_level / 6) - 1;
   }
   else
   {
      if(power_level > 15)
         power_level = 15;
      if(unit->generation != RTL8187B1_GEN)
         power_level += 7;
      power_level += unit->base_cck_power;
      if(power_level > 35)
         power_level = 35;

      table_no = 7;
      tx_gain = power_level << 1;
   }

   /* Adjust power table number for RTL8225v2b */

   if(unit->radio_type == RTL8225V2B_RADIO)
   {
      if(unit->generation == RTL8187B1_GEN)
      {
         if(power_level > 11 && unit->channel != 14)
            table_no = 5;
         else if(power_level > 6)
            table_no = 6;
         else
            table_no = 7;
      }
      else
      {
         if(power_level > 5)
            table_no = 6;
         else
            table_no = 7;
         if(unit->channel != 14)
         {
            if(power_level > 17)
               table_no = 4;
            else if(power_level > 11)
               table_no = 5;
         }
      }
   }

   /* Get CCK power table and write it to chip */

   if(unit->channel == 14)
      power_table = rtl8225_ch14_power_tables[table_no];
   else
      power_table = rtl8225_power_tables[table_no];

   for(i = 0; i < 8; i++)
      WriteCCK(unit, 0x44 + i, power_table[i], base);

   /* Set CCK gain */

   unit->ByteOut(unit->card, 0x100 + R8180REG_TXGAINCCK, tx_gain);
   BusyMilliDelay(1, base);

   /* Get OFDM power level */

   power_level = unit->ofdm_power[unit->channel];
   if(power_level > 15)
      power_level = 15;
   if(unit->radio_type <= RTL8225V2_RADIO)
   {
      power_level += 10;
   }
   else
   {
      if(unit->generation != RTL8187B1_GEN)
         power_level += 10;
      else
         power_level += 2;
   }
   if(unit->radio_type >= RTL8225V2_RADIO)
      power_level += unit->base_ofdm_power;
   if(power_level > 35)
      power_level = 35;

   /* Enable analogue parameter 2 */

   if(unit->radio_type <= RTL8225V2_RADIO)
   {
      unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM, R8180ECMD_CONFIG);
      unit->ByteOut(unit->card, 0x100 + R8180REG_CONFIG3,
         unit->ByteIn(unit->card, 0x100 + R8180REG_CONFIG3)
         | R8180REG_CONFIG3F_ANAPARAMWRITE);
      unit->LELongOut(unit->card, 0x100 + R8180REG_ANAPARAM2, 0x860c7312);
      unit->ByteOut(unit->card, 0x100 + R8180REG_CONFIG3,
         unit->ByteIn(unit->card, 0x100 + R8180REG_CONFIG3)
         & ~R8180REG_CONFIG3F_ANAPARAMWRITE);
      unit->ByteOut(unit->card, 0x100 + R8180REG_EEPROM, 0);
   }

   /* Set OFDM gain */

   if(unit->radio_type == RTL8225_RADIO)
      tx_gain = (2 << power_level / 6) - 1;
   else
      tx_gain = power_level << 1;

   unit->ByteOut(unit->card, 0x100 + R8180REG_TXGAINOFDM, tx_gain);

   /* Set up OFDM power */

   if(unit->radio_type == RTL8225V2B_RADIO)
   {
      if(unit->generation == RTL8187B1_GEN)
      {
         if(power_level > 11)
            n = 0x5c;
         else
            n = 0x60;
      }
      else
      {
         if(power_level > 17)
            n = 0x50;
         else if(power_level > 11)
            n = 0x54;
         else
            n = 0x5c;
      }
      WriteOFDM(unit, 0x87, n, base);
      WriteOFDM(unit, 0x89, n, base);
   }
   else if(unit->radio_type == RTL8225V2_RADIO)
   {
      WriteOFDM(unit, 0x2, 0x42, base);
      WriteOFDM(unit, 0x5, 0, base);
      WriteOFDM(unit, 0x6, 0x40, base);
      WriteOFDM(unit, 0x7, 0, base);
      WriteOFDM(unit, 0x8, 0x40, base);
   }
   else
   {
      n = rtl8225_ofdm_power[power_level % 6];
      WriteOFDM(unit, 0x2, 0x42, base);
      WriteOFDM(unit, 0x5, n, base);
      WriteOFDM(unit, 0x6, 0, base);
      WriteOFDM(unit, 0x7, n, base);
      WriteOFDM(unit, 0x8, 0, base);
   }

   BusyMilliDelay(1, base);

   /* Set channel */

   tx_conf = unit->LELongIn(unit->card, 0x100 + R8180REG_TXCONF);
   loop_tx_conf = tx_conf & ~R8180REG_TXCONFF_LOOPBACK
      | 1 << R8180REG_TXCONFB_LOOPBACK;
   unit->LELongOut(unit->card, 0x100 + R8180REG_TXCONF,
      loop_tx_conf);
   if(unit->channel == 14)
      channel_code = 0xf72;
   else
      channel_code = 0x7dc + unit->channel * 0x80;
   WriteRadio(unit, 0x7, channel_code, base);
   BusyMilliDelay(10, base);
   unit->LELongOut(unit->card, 0x100 + R8180REG_TXCONF, tx_conf);

   return;
}



/****i* realtek8180.device/WriteCCK ****************************************
*
*   NAME
*	WriteCCK -- Write to a CCK PHY register.
*
*   SYNOPSIS
*	WriteCCK(unit, index, value)
*
*	VOID WriteCCK(struct DevUnit *, UWORD, UWORD);
*
*   INPUTS
*	unit - A unit of this device.
*	index - Offset of register.
*	value - Value to write to register.
*
*   RESULT
*	None
*
****************************************************************************
*
*/

VOID WriteCCK(struct DevUnit *unit, UWORD index, UWORD value,
   struct DevBase *base)
{
   WritePHY(unit, index, value | 1 << 16, base);

   return;
}



/****i* realtek8180.device/WriteOFDM ***************************************
*
*   NAME
*	WriteOFDM -- Write to an OFDM PHY register.
*
*   SYNOPSIS
*	WriteOFDM(unit, index, value)
*
*	VOID WriteOFDM(struct DevUnit *, UWORD, UWORD);
*
*   INPUTS
*	unit - A unit of this device.
*	index - Offset of register.
*	value - Value to write to register.
*
*   RESULT
*	None
*
****************************************************************************
*
*/

VOID WriteOFDM(struct DevUnit *unit, UWORD index, UWORD value,
   struct DevBase *base)
{
   WritePHY(unit, index, value, base);

   return;
}



/****i* realtek8180.device/WritePHY ****************************************
*
*   NAME
*	WritePHY -- Write to a CCK PHY register.
*
*   SYNOPSIS
*	WritePHY(unit, index, value)
*
*	VOID WritePHY(struct DevUnit *, UWORD, ULONG);
*
*   INPUTS
*	unit - A unit of this device.
*	index - Offset of register.
*	value - Value to write to register.
*
*   RESULT
*	None
*
****************************************************************************
*
*/

static VOID WritePHY(struct DevUnit *unit, UWORD index, ULONG value,
   struct DevBase *base)
{
   ULONG data;

   data = value << 8 | index | 0x80;
   unit->ByteOut(unit->card, 0x100 + R8180REG_PHY + 3, data >> 24);
   unit->ByteOut(unit->card, 0x100 + R8180REG_PHY + 2, data >> 16);
   unit->ByteOut(unit->card, 0x100 + R8180REG_PHY + 1, data >> 8);
   unit->ByteOut(unit->card, 0x100 + R8180REG_PHY, data);

   return;
}



/****i* realtek8180.device/ReadRadio ***************************************
*
*   NAME
*	ReadRadio -- Read a radio register.
*
*   SYNOPSIS
*	value = ReadRadio(unit, index)
*
*	UWORD ReadRadio(struct DevUnit *, UWORD);
*
*   INPUTS
*	unit - A unit of this device.
*	index - Offset of register.
*
*   RESULT
*	value - Value read from radio register.
*
****************************************************************************
*
*/

UWORD ReadRadio(struct DevUnit *unit, UWORD index,
   struct DevBase *base)
{
   UWORD value = 0, value2, output_reg, enable_reg, select_reg;
   WORD i, j;

   output_reg = unit->LEWordIn(unit->card, 0x100 + R8180REG_RFPINSOUTPUT)
      & 0xfff0;
   enable_reg = unit->LEWordIn(unit->card, 0x100 + R8180REG_RFPINSENABLE);
   select_reg = unit->LEWordIn(unit->card, 0x100 + R8180REG_RFPINSSELECT);
   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSENABLE,
      enable_reg | 0xf);
   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSSELECT,
      select_reg | 0xf);
   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT,
      output_reg | 0x4);
   BusyMicroDelay(4, base);

   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT, output_reg);
   BusyMicroDelay(5, base);

   for(i = 4; i >= 0; i--)
   {
      value2 = output_reg | index >> i & 1;

      if((i & 0x1) == 0)
      {
         unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT, value2);
         BusyMicroDelay(1, base);
      }

      unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT, value2 | 0x2);
      BusyMicroDelay(2, base);

      unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT, value2 | 0x2);
      BusyMicroDelay(2, base);

      if((i & 0x1) != 0)
      {
         unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT, value2);
         BusyMicroDelay(1, base);
      }
   }

   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT,
      output_reg | 0xa);
   BusyMicroDelay(2, base);

   for(i = 0; i < 2; i++)
   {
      unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT,
         output_reg | 0x8);
      BusyMicroDelay(2, base);
   }

   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSENABLE,
      (enable_reg | 0xe) & ~1);

   for(i = 11; i >= 0; i--)
   {
      unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT,
         output_reg | 0x8);
      BusyMicroDelay(1, base);

      for(j = 0; j < 3; j++)
      {
         unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT,
            output_reg | 0xa);
         BusyMicroDelay(2, base);
      }

      if((unit->LEWordIn(unit->card, 0x100 + R8180REG_RFPINSINPUT) & 0x2)
         != 0)
         value |= 1 << i;

      unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT,
         output_reg | 0x8);
      BusyMicroDelay(2, base);
   }

   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT, output_reg | 0xc);
   BusyMicroDelay(2, base);

   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSENABLE, enable_reg);
   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSSELECT, select_reg);
   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT, 0x3a0);

   return value;
}



/****i* realtek8180.device/WriteRadio **************************************
*
*   NAME
*	WriteRadio -- Write to a radio register.
*
*   SYNOPSIS
*	WriteRadio(unit, index, value)
*
*	VOID WriteRadio(struct DevUnit *, UWORD, UWORD);
*
*   INPUTS
*	unit - A unit of this device.
*	index - Offset of register.
*	value - Value to write to register.
*
*   RESULT
*	None
*
****************************************************************************
*
*/

VOID WriteRadio(struct DevUnit *unit, UWORD index, UWORD value,
   struct DevBase *base)
{
   UWORD output_reg, enable_reg, select_reg, value2;
   ULONG data;
   WORD i;

   data = (value << 4) | (index & 0xf);

   output_reg = unit->LEWordIn(unit->card, 0x100 + R8180REG_RFPINSOUTPUT)
      & 0xfff3;
   enable_reg = unit->LEWordIn(unit->card, 0x100 + R8180REG_RFPINSENABLE);
   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSENABLE,
      enable_reg | 0x7);
   select_reg = unit->LEWordIn(unit->card, 0x100 + R8180REG_RFPINSSELECT);
   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSSELECT,
      select_reg | 0x7);
   BusyMicroDelay(10, base);

   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT,
      output_reg | 0x4);
   BusyMicroDelay(2, base);

   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT, output_reg);
   BusyMicroDelay(10, base);

   if(unit->bus == USB_BUS && unit->generation >= RTL8187B0_GEN)
      unit->LEWordOut(unit->card, 0x8225 << 16 | 0x200 + index, value);
   else
   {
      for(i = 15; i >= 0; i--)
      {
         value2 = output_reg | (data & (1 << i)) >> i;

         if((i & 1) != 0)
            unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT,
               value2);
         unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT,
            value2 | 0x2);
         unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT,
            value2 | 0x2);
         if((i & 1) == 0)
            unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT,
               value2);
      }
   }

   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT,
      output_reg | 0x4);
   BusyMicroDelay(10, base);

   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSOUTPUT,
      output_reg | 0x4);
   unit->LEWordOut(unit->card, 0x100 + R8180REG_RFPINSSELECT, select_reg);

   return;
}



