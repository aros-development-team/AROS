/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2005 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#ifndef ahi_addroutines_h
#define ahi_addroutines_h

#include <config.h>

#include "ahi_def.h"

/*
** Samples          Number of samples to calculate.
** ScaleLeft        Left volume multiplier.
** ScaleRight       Right volume multiplier (not used for mono sounds).
** StartPointLeft   Sample value from last session, for interpolation. Update!
** StartPointRight  Sample value from last session, for interpolation. Update!
** Src              Pointer to source samples.
** Dst              Pointer to pointer to destination buffer. Update!
** FirstOffsetI     The offset value of the first sample (when StartPoint* 
**                  should be used).
** Offset           The offset (fix-point). Update!
** Add              Add value (fix-point).
** StopAtZero       If true, abort at next zero-crossing.
*/

#define ADDARGS LONG      Samples,\
                LONG      ScaleLeft,\
                LONG      ScaleRight,\
                LONG	 *StartPointLeft,\
                LONG	 *StartPointRight,\
                void     *Src,\
                void    **Dst,\
                LONG	  FirstOffsetI,\
                Fixed64   Add,\
                Fixed64  *Offset,\
                BOOL      StopAtZero

#define ADDARGS71 LONG      Samples,\
                LONG      ScaleLeft,\
                LONG      ScaleRight,\
                LONG	 *StartPoints,\
                LONG	 *Unused,\
                void     *Src,\
                void    **Dst,\
                LONG	  FirstOffsetI,\
                Fixed64   Add,\
                Fixed64  *Offset,\
                BOOL      StopAtZero

typedef LONG (ADDFUNC)(ADDARGS);

LONG AddByteMono( ADDARGS );
LONG AddByteStereo( ADDARGS );
LONG AddBytesMono( ADDARGS );
LONG AddBytesStereo( ADDARGS );
LONG AddWordMono( ADDARGS );
LONG AddWordStereo( ADDARGS );
LONG AddWordsMono( ADDARGS );
LONG AddWordsStereo( ADDARGS );
LONG AddLongMono( ADDARGS );
LONG AddLongStereo( ADDARGS );
LONG AddLongsMono( ADDARGS );
LONG AddLongsStereo( ADDARGS );
LONG Add71Mono( ADDARGS );
LONG Add71Stereo( ADDARGS );

LONG AddByte71( ADDARGS );
LONG AddBytes71( ADDARGS );
LONG AddWord71( ADDARGS );
LONG AddWords71( ADDARGS );
LONG AddLong71( ADDARGS );
LONG AddLongs71( ADDARGS );
LONG Add7171( ADDARGS71 );

LONG AddByteMonoB( ADDARGS );
LONG AddByteStereoB( ADDARGS );
LONG AddBytesMonoB( ADDARGS );
LONG AddBytesStereoB( ADDARGS );
LONG AddWordMonoB( ADDARGS );
LONG AddWordStereoB( ADDARGS );
LONG AddWordsMonoB( ADDARGS );
LONG AddWordsStereoB( ADDARGS );
LONG AddLongMonoB( ADDARGS );
LONG AddLongStereoB( ADDARGS );
LONG AddLongsMonoB( ADDARGS );
LONG AddLongsStereoB( ADDARGS );
LONG Add71MonoB( ADDARGS );
LONG Add71StereoB( ADDARGS );

LONG AddByte71B( ADDARGS );
LONG AddBytes71B( ADDARGS );
LONG AddWord71B( ADDARGS );
LONG AddWords71B( ADDARGS );
LONG AddLong71B( ADDARGS );
LONG AddLongs71B( ADDARGS );
LONG Add7171B( ADDARGS71 );

LONG AddLofiByteMono( ADDARGS );
LONG AddLofiByteStereo( ADDARGS );
LONG AddLofiBytesMono( ADDARGS );
LONG AddLofiBytesStereo( ADDARGS );
LONG AddLofiWordMono( ADDARGS );
LONG AddLofiWordStereo( ADDARGS );
LONG AddLofiWordsMono( ADDARGS );
LONG AddLofiWordsStereo( ADDARGS );
LONG AddLofiLongMono( ADDARGS );
LONG AddLofiLongStereo( ADDARGS );
LONG AddLofiLongsMono( ADDARGS );
LONG AddLofiLongsStereo( ADDARGS );

LONG AddLofiByteMonoB( ADDARGS );
LONG AddLofiByteStereoB( ADDARGS );
LONG AddLofiBytesMonoB( ADDARGS );
LONG AddLofiBytesStereoB( ADDARGS );
LONG AddLofiWordMonoB( ADDARGS );
LONG AddLofiWordStereoB( ADDARGS );
LONG AddLofiWordsMonoB( ADDARGS );
LONG AddLofiWordsStereoB( ADDARGS );
LONG AddLofiLongMonoB( ADDARGS );
LONG AddLofiLongStereoB( ADDARGS );
LONG AddLofiLongsMonoB( ADDARGS );
LONG AddLofiLongsStereoB( ADDARGS );

#endif /* ahi_addroutines_h */
