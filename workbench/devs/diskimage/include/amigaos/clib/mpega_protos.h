#ifndef CLIB_MPEGA_PROTOS_H
#define CLIB_MPEGA_PROTOS_H


/*
**	$VER: mpega_protos.h 1.0 (01.04.2010)
**
**	C prototypes. For use with 32 bit integers only.
**
**	Copyright © 2010 
**	All Rights Reserved
*/

#ifndef  LIBRARIES_MPEGA_H
#include <libraries/mpega.h>
#endif

MPEGA_STREAM * MPEGA_open(char * filename, MPEGA_CTRL * ctrl);
void MPEGA_close(MPEGA_STREAM * mpds);
LONG MPEGA_decode_frame(MPEGA_STREAM * mpds, WORD ** pcm);
LONG MPEGA_seek(MPEGA_STREAM * mpds, ULONG ms_time_position);
LONG MPEGA_time(MPEGA_STREAM * mpds, ULONG * ms_time_position);
LONG MPEGA_find_sync(BYTE * buffer, LONG buffer_size);
LONG MPEGA_scale(MPEGA_STREAM * mpds, LONG scale_percent);

#endif	/*  CLIB_MPEGA_PROTOS_H  */
