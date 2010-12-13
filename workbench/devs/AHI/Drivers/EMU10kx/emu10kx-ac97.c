/*
     emu10kx.audio - AHI driver for SoundBlaster Live! series
     Copyright (C) 2002-2005 Martin Blom <martin@blom.org>

     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <config.h>

#include <clib/alib_protos.h>
#include <proto/utility.h>

#include "library.h"
#include "8010.h"
#include "hwaccess.h"

static ULONG dsp_register[] = {
  /* Input volume GPR */
  VOL_AHI_FRONT_L,
  VOL_AHI_FRONT_R,
  VOL_AHI_REAR_L,
  VOL_AHI_REAR_R,
  VOL_AHI_SURROUND_L,
  VOL_AHI_SURROUND_R,
  VOL_AHI_CENTER,
  VOL_AHI_LFE,

  VOL_SPDIF_CD_L,
  VOL_SPDIF_CD_R,
  VOL_SPDIF_IN_L,
  VOL_SPDIF_IN_R,

  /* Output volume GPR */
  VOL_SPDIF_FRONT_L,
  VOL_SPDIF_FRONT_R,
  VOL_SPDIF_REAR_L,
  VOL_SPDIF_REAR_R,
  VOL_SPDIF_SURROUND_L,
  VOL_SPDIF_SURROUND_R,
  VOL_SPDIF_CENTER,
  VOL_SPDIF_LFE,

  VOL_ANALOG_FRONT_L,
  VOL_ANALOG_FRONT_R,
  VOL_ANALOG_REAR_L,
  VOL_ANALOG_REAR_R,
  VOL_ANALOG_SURROUND_L,
  VOL_ANALOG_SURROUND_R,
  VOL_ANALOG_CENTER,
  VOL_ANALOG_LFE,

  /* AHI_FRONT-to-rear GPR */
  VOL_FRONT_REAR_L,
  VOL_FRONT_REAR_R,

  /* AHI_SURROUND-to-rear GPR */
  VOL_SURROUND_REAR_L,
  VOL_SURROUND_REAR_R,

  /* AHI_FRONT-to-center and AHI_FRONT-to-LFE GPRs */
  VOL_FRONT_CENTER,
  VOL_FRONT_LFE
};

/******************************************************************************
** Read from AC97 register ****************************************************
******************************************************************************/

ULONG
AC97GetFunc( struct Hook*           hook,
	     struct EMU10kxBase*    EMU10kxBase,
	     struct AC97GetMessage* msg )
{
  struct DriverBase*  AHIsubBase = (struct DriverBase*) EMU10kxBase;
  struct EMU10kxData* dd;

  if( msg->CardNum >= (ULONG) EMU10kxBase->cards_found ||
      EMU10kxBase->driverdatas[ msg->CardNum ] == NULL )
  {
    Req( "No valid EMU10kxData for AC97 card %ld.", msg->CardNum );
    return ~0UL;
  }

  dd = EMU10kxBase->driverdatas[ msg->CardNum ];

  if (msg->Register >= emu10kx_dsp_first &&
      msg->Register < emu10kx_dsp_last) {
    ULONG reg = dsp_register[msg->Register - emu10kx_dsp_first];

    // Reading is not supported yet ...
    return ~0UL;
  }
  else {
    return emu10k1_readac97( &dd->card, msg->Register );
  }
}


/******************************************************************************
** Write to AC97 register *****************************************************
******************************************************************************/

VOID
AC97SetFunc( struct Hook*           hook,
	     struct EMU10kxBase*    EMU10kxBase,
	     struct AC97SetMessage* msg )
{
  struct DriverBase*  AHIsubBase = (struct DriverBase*) EMU10kxBase;
  struct EMU10kxData* dd;

  if( msg->CardNum >= (ULONG) EMU10kxBase->cards_found ||
      EMU10kxBase->driverdatas[ msg->CardNum ] == NULL )
  {
    Req( "No valid EMU10kxData for AC97 card %ld.", msg->CardNum );
    return;
  }

  dd = EMU10kxBase->driverdatas[ msg->CardNum ];

  if (msg->Register >= emu10kx_dsp_first &&
      msg->Register < emu10kx_dsp_last) {
    ULONG reg = dsp_register[msg->Register - emu10kx_dsp_first];
    
    emu10k1_set_volume_gpr( &dd->card, reg, 100, VOL_5BIT);
  }
  else {
    emu10k1_writeac97( &dd->card, msg->Register, msg->Value );
  }
}
