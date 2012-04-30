#ifndef CLIB_MPEGA_PROTOS_H
#define CLIB_MPEGA_PROTOS_H

#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif

#ifndef LIBRARIES_MPEGA_H
#include <libraries/mpega.h>
#endif

AROS_LP2(MPEGA_STREAM *, MPEGA_open,
    AROS_LPA(char *, stream_name, A0),
    AROS_LPA(MPEGA_CTRL *, ctrl, A1),
    struct Library *, MPEGABase, 5, mpega);
    
AROS_LP1(void, MPEGA_close,
    AROS_LPA(MPEGA_STREAM *, mpega_stream, A0),
    struct Library *, MPEGABase, 6, mpega);
    
AROS_LP2(LONG, MPEGA_decode_frame,
    AROS_LPA(MPEGA_STREAM *, mpega_stream, A0),
    AROS_LPA(WORD *, pcm[MPEGA_MAX_CHANNELS], A1),
    struct Library *, MPEGABase, 7, mpega);
    
AROS_LP2(LONG, MPEGA_seek,
    AROS_LPA(MPEGA_STREAM *, mpega_stream, A0),
    AROS_LPA(ULONG, ms_time_position, D0),
    struct Library *, MPEGABase, 8, mpega);
    
AROS_LP2(LONG, MPEGA_time,
    AROS_LPA(MPEGA_STREAM *, mpega_stream, A0),
    AROS_LPA(ULONG *, ms_time_position, A1),
    struct Library *, MPEGABase, 9, mpega);

AROS_LP2(LONG, MPEGA_find_sync,
    AROS_LPA(UBYTE *, buffer, A0),
    AROS_LPA(LONG, buffer_size, D0),
    struct Library *, MPEGABase, 10, mpega);

AROS_LP2(LONG, MPEGA_scale,
    AROS_LPA(MPEGA_STREAM *, mpega_stream, A0),
    AROS_LPA(LONG, scale_percent, D0),
    struct Library *, MPEGABase, 11, mpega);
    
#endif /* CLIB_MPEGA_PROTOS_H */
