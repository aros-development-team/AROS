#ifndef _VBCCINLINE_MPEGA_H
#define _VBCCINLINE_MPEGA_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

MPEGA_STREAM * __MPEGA_open(__reg("a6") struct Library *, __reg("a0") char * filename, __reg("a1") MPEGA_CTRL * ctrl)="\tjsr\t-30(a6)";
#define MPEGA_open(filename, ctrl) __MPEGA_open(MPEGABase, (filename), (ctrl))

void __MPEGA_close(__reg("a6") struct Library *, __reg("a0") MPEGA_STREAM * mpds)="\tjsr\t-36(a6)";
#define MPEGA_close(mpds) __MPEGA_close(MPEGABase, (mpds))

LONG __MPEGA_decode_frame(__reg("a6") struct Library *, __reg("a0") MPEGA_STREAM * mpds, __reg("a1") WORD ** pcm)="\tjsr\t-42(a6)";
#define MPEGA_decode_frame(mpds, pcm) __MPEGA_decode_frame(MPEGABase, (mpds), (pcm))

LONG __MPEGA_seek(__reg("a6") struct Library *, __reg("a0") MPEGA_STREAM * mpds, __reg("d0") ULONG ms_time_position)="\tjsr\t-48(a6)";
#define MPEGA_seek(mpds, ms_time_position) __MPEGA_seek(MPEGABase, (mpds), (ms_time_position))

LONG __MPEGA_time(__reg("a6") struct Library *, __reg("a0") MPEGA_STREAM * mpds, __reg("a1") ULONG * ms_time_position)="\tjsr\t-54(a6)";
#define MPEGA_time(mpds, ms_time_position) __MPEGA_time(MPEGABase, (mpds), (ms_time_position))

LONG __MPEGA_find_sync(__reg("a6") struct Library *, __reg("a0") BYTE * buffer, __reg("d0") LONG buffer_size)="\tjsr\t-60(a6)";
#define MPEGA_find_sync(buffer, buffer_size) __MPEGA_find_sync(MPEGABase, (buffer), (buffer_size))

LONG __MPEGA_scale(__reg("a6") struct Library *, __reg("a0") MPEGA_STREAM * mpds, __reg("d0") LONG scale_percent)="\tjsr\t-66(a6)";
#define MPEGA_scale(mpds, scale_percent) __MPEGA_scale(MPEGABase, (mpds), (scale_percent))

#endif /*  _VBCCINLINE_MPEGA_H  */
