#ifndef DEFINES_MPEGA_PROTOS_H
#define DEFINES_MPEGA_PROTOS_H

/*
    Desc: Defines for mpega
*/

#include <aros/libcall.h>
#include <exec/types.h>

#define __MPEGA_open_WB(__MPEGABase, __arg1, __arg2) \
    AROS_LC2(MPEGA_STREAM *, MPEGA_open, \
    	AROS_LCA(char *, (__arg1), A0), \
    	AROS_LCA(MPEGA_CTRL *, (__arg2), A1), \
    	struct Library *, (__MPEGABase), 5, mpega)

#define MPEGA_open(arg1, arg2) \
    __MPEGA_open_WB(MPEGABase, (arg1), (arg2))

#define __MPEGA_close_WB(__MPEGABase, __arg1) \
    AROS_LC1(void, MPEGA_close, \
    	AROS_LCA(MPEGA_STREAM *, (__arg1), A0), \
    	struct Library *, (__MPEGABase), 6, mpega)

#define MPEGA_close(arg1) \
    __MPEGA_close_WB(MPEGABase, (arg1))

#define __MPEGA_decode_frame_WB(__MPEGABase, __arg1, __arg2) \
    AROS_LC2(LONG, MPEGA_decode_frame, \
    AROS_LCA(MPEGA_STREAM *, (__arg1), A0), \
    AROS_LCA(WORD *, (__arg2), A1), \
    struct Library *, (__MPEGABase), 7, mpega)
    
#define MPEGA_decode_frame(arg1, arg2) \
    __MPEGA_decode_frame_WB(MPEGABase, (arg1), (arg2))
    
#define __MPEGA_seek_WB(__MPEGABase, __arg1, __arg2) \
    AROS_LC2(LONG, MPEGA_seek, \
    	AROS_LCA(MPEGA_STREAM *, (__arg1), A0), \
    	AROS_LCA(ULONG, (__arg2), D0), \
    	struct Library *, (__MPEGABase), 8, mpega)
    
#define MPEGA_seek(arg1, arg2) \
    __MPEGA_seek_WB(MPEGABase, (arg1), (arg2))

#define __MPEGA_time_WB(__MPEGABase, __arg1, __arg2) \
    AROS_LC2(LONG, MPEGA_time, \
        AROS_LCA(MPEGA_STREAM *, (__arg1), A0), \
        AROS_LCA(ULONG *, (__arg2), A1), \
        struct Library *, (__MPEGABase), 9, mpega)

#define MPEGA_time(arg1, arg2) \
    __MPEGA_time_WB(MPEGABase, (arg1), (arg2))

#define __MPEGA_find_sync_WB(__MPEGABase, __arg1, __arg2) \
    AROS_LC2(LONG, MPEGA_find_sync, \
    	AROS_LCA(UBYTE *, (__arg1), A0), \
    	AROS_LCA(LONG, (__arg2), D0), \
    	struct Library *, (__MPEGABase), 10, mpega)

#define MPEGA_find_sync(arg1, arg2) \
    __MPEGA_find_sync_WB(MPEGABase, (arg1), (arg2))
    
#define __MPEGA_scale_WB(__MPEGABase, __arg1, __arg2) \
    AROS_LC2(LONG, MPEGA_scale, \
    	AROS_LCA(MPEGA_STREAM *, (__arg1), A0), \
    	AROS_LCA(LONG, (__arg2), D0), \
    	struct Library *, (__MPEGABase), 11, mpega)

#define MPEGA_scale(arg1, arg2) \
    __MPEGA_scale_WB(MPEGABase, (arg1), (arg2))
    
#endif /* DEFINES_MPEGA_PROTOS_H*/
