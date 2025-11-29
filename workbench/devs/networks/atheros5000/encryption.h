/*

Copyright (C) 2010-2020 Neil Cafferkey

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

#ifndef ENCRYPTION_H
#define ENCRYPTION_H


#include <exec/types.h>


#define IV_SIZE 4
#define EIV_SIZE 8
#define ICV_SIZE 4
#define MIC_SIZE 8
#define FCS_SIZE 4


struct KeyUnion
{
   UWORD type;
   union
   {
      struct WEPKey
      {
         UWORD length;
         UBYTE key[13];
         ULONG tx_iv;
      }
      wep;
      struct TKIPKey
      {
         UWORD key[8];
         ULONG tx_mic_key[2];
         ULONG rx_mic_key[2];
         UWORD tx_iv_low;
         ULONG tx_iv_high;
         UWORD rx_iv_low;
         ULONG rx_iv_high;
         UWORD tx_ttak[5];
         BOOL tx_ttak_set;
         UWORD rx_ttak[5];
         BOOL rx_ttak_set;
      }
      tkip;
      struct CCMPKey
      {
         UBYTE key[16];
         BOOL stream_set;
         ULONG stream[44];
         UWORD tx_iv_low;
         ULONG tx_iv_high;
         UWORD rx_iv_low;
         ULONG rx_iv_high;
      }
      ccmp;
   }
   u;
};


#endif
