/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
*/

#include "oss_intern.h"

#include <hidd/unixio.h>

#include <unistd.h>
#include <errno.h>
/* avoid conflicts between our __unused define and the ones that might come in
   via fcntl.h */
#undef __unused
#include <fcntl.h>

#include <sys/ioctl.h>

#include <sys/soundcard.h>
//#include <sys/mman.h>

/* Make it build on both newer and older OSS libraries */
#if !defined(SNDCTL_DSP_SPEED) && defined(SOUND_PCM_WRITE_RATE)
#  define SNDCTL_DSP_SPEED  SOUND_PCM_WRITE_RATE
#endif

#include <aros/debug.h>

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_Open(

    SYNOPSIS
        char *filename, BOOL read, BOOL write, BOOL blocking)

    FUNCTION
        Open a host OSS audio device for subsequent OSS operations.

    INPUTS
        filename - NUL-terminated host path, or NULL for /dev/dsp.
        read - Request read access when write is also TRUE.
        write - Request write access. If FALSE, the device is opened
                read-only regardless of read.
        blocking - FALSE adds O_NONBLOCK to the host open flags.

    RESULT
        TRUE if the host descriptor is nonnegative; FALSE on failure.

    NOTES
        The library uses one shared descriptor. Serialize its operations
        and keep the library open while using the device. Call OSS_Open()
        before any device operation; the initial descriptor value is not
        a valid closed-state marker. Close the device with OSS_Close()
        before closing the library.

    BUGS
        Opening again overwrites the stored descriptor without closing
        the previous one, even when the new open fails. Close the active
        device before reopening. Format and capability caches survive
        close and reopen and may describe an earlier device.

*****************************************************************************/
BOOL OSS_Open(char *filename, BOOL read, BOOL write, BOOL blocking)
{
    int openflags;
    
    if (!filename) filename = "/dev/dsp";
    
    if (write && read)
    {
        openflags = O_RDWR;
    }
    else if (write)
    {
        openflags = O_WRONLY;
    }
    else
    {
        openflags = O_RDONLY;
    }
    
    if (!blocking) openflags |= O_NONBLOCK;
    
    audio_fd = Hidd_UnixIO_OpenFile(unixio, filename, openflags, 0, NULL);
    
    return (audio_fd >= 0) ? TRUE : FALSE;
}

/******************************************************************************/

/*****************************************************************************
    NAME
        void OSS_Close(

    SYNOPSIS
        void)

    FUNCTION
        Close the stored host descriptor and mark the device closed.

    RESULT
        None. The host close result is not reported.

    NOTES
        Call only after an OSS_Open() attempt. If the stored descriptor
        is already negative, nothing is done. The descriptor is set to
        -1 even if the host close fails. Cached formats and capabilities
        are retained.

*****************************************************************************/
void OSS_Close(void)
{
    if (audio_fd >= 0)
    {
        Hidd_UnixIO_CloseFile(unixio, audio_fd, NULL);
        audio_fd = -1;
    }
}

/******************************************************************************/

/*****************************************************************************
    NAME
        void OSS_Reset(

    SYNOPSIS
        void)

    FUNCTION
        Issue SNDCTL_DSP_RESET to the open device.

    RESULT
        None. Host errors are not reported.

    NOTES
        Requires a successful OSS_Open() and an open device.
        If the stored descriptor is negative, nothing is done.

*****************************************************************************/
void OSS_Reset(void)
{
    if (audio_fd >= 0)
    {
        int value = 0;
        
        Hidd_UnixIO_IOControlFile(unixio, audio_fd, SNDCTL_DSP_RESET, &value, NULL);
    }
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_SetFragmentSize(

    SYNOPSIS
        int num_fragments, int fragment_size)

    FUNCTION
        Request an output-buffer fragment configuration.

    INPUTS
        num_fragments - Requested fragment count, packed into the upper
                        16 bits of the host argument. Use a nonnegative
                        value whose left shift by 16 fits in int.
        fragment_size - Base-two exponent of the fragment size in bytes,
                        occupying the lower 16 bits; 8 requests 256 bytes.

    RESULT
        TRUE if the host control call returns a nonnegative value; FALSE
        if no device is open or the call fails.

    NOTES
        Requires a successful OSS_Open() and an open device.
        Arguments are packed without validation. The host may adjust the
        configuration; this function does not return the accepted values.

*****************************************************************************/
BOOL OSS_SetFragmentSize(int num_fragments, int fragment_size)
{
    if (audio_fd >= 0)
    {
        int value;
        int retval;
        
        value = (num_fragments << 16) | fragment_size;
        
        retval = Hidd_UnixIO_IOControlFile(unixio, audio_fd, SNDCTL_DSP_SETFRAGMENT, &value, NULL);
        return (retval < 0) ? FALSE : TRUE;
    }
    else
    {
        return FALSE;
    }
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_GetOutputInfo(

    SYNOPSIS
        int *num_fragments_available, int *num_fragments_allocated, int *fragment_size, int *num_bytes_available)

    FUNCTION
        Query output-buffer availability with SNDCTL_DSP_GETOSPACE.

    INPUTS
        num_fragments_available - Optional int output for fully available
                                  fragments.
        num_fragments_allocated - Optional int output for total fragments.
        fragment_size - Optional int output for bytes per fragment.
        num_bytes_available - Optional int output for available bytes,
                              including space in partially used fragments.

    RESULT
        FALSE if the stored descriptor is negative, leaving outputs
        unchanged. Otherwise TRUE, even if the host query fails.

    NOTES
        Requires a successful OSS_Open() and an open device.
        Each output pointer may be NULL; other pointers must be writable.

    BUGS
        The host result is ignored and the local result structure is not
        initialized. On a failed query, copied values are indeterminate.
        TRUE does not certify that the outputs are valid.

*****************************************************************************/
BOOL OSS_GetOutputInfo(int *num_fragments_available, int *num_fragments_allocated,
                       int *fragment_size, int *num_bytes_available)

{
    if (audio_fd >= 0)
    {
        audio_buf_info info;
        
        Hidd_UnixIO_IOControlFile(unixio, audio_fd, SNDCTL_DSP_GETOSPACE, &info, NULL);
        
        if (num_fragments_available) *num_fragments_available = info.fragments;
        if (num_fragments_allocated) *num_fragments_allocated = info.fragstotal;
        if (fragment_size) *fragment_size = info.fragsize;
        if (num_bytes_available) *num_bytes_available = info.bytes;
        
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_GetOutputPointer(

    SYNOPSIS
        int *processed_bytes, int *fragment_transitions, int *dmapointer)

    FUNCTION
        Query output progress with SNDCTL_DSP_GETOPTR.

    INPUTS
        processed_bytes - Optional int output for bytes processed.
        fragment_transitions - Optional int output for fragment transitions
                               since the previous host pointer query.
        dmapointer - Optional int output for the current DMA-buffer offset.

    RESULT
        FALSE if the stored descriptor is negative, leaving outputs
        unchanged. Otherwise TRUE, even if the host query fails.

    NOTES
        Requires a successful OSS_Open() and an open device.
        Each output pointer may be NULL; other pointers must be writable.

    BUGS
        The host result is ignored and the local result structure is not
        initialized. On a failed query, copied values are indeterminate.
        TRUE does not certify that the outputs are valid.

*****************************************************************************/
BOOL OSS_GetOutputPointer(int *processed_bytes, int *fragment_transitions, int *dmapointer)
{
    if (audio_fd >= 0)
    {
        count_info info;
        
        Hidd_UnixIO_IOControlFile(unixio, audio_fd, SNDCTL_DSP_GETOPTR, &info, NULL);
        
        if (processed_bytes) *processed_bytes = info.bytes;
        if (fragment_transitions) *fragment_transitions = info.blocks;
        if (dmapointer) *dmapointer = info.ptr;
        
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    
}


/******************************************************************************/

int audio_supported_fmts;

static BOOL get_supported_fmts(void)
{
    if (audio_supported_fmts) return TRUE;
    if (audio_fd < 0) return FALSE;
    
    Hidd_UnixIO_IOControlFile(unixio, audio_fd, SNDCTL_DSP_GETFMTS, &audio_supported_fmts, NULL);
    
    return TRUE;
    
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_FormatSupported_S8(

    SYNOPSIS
        void)

    FUNCTION
        Test the cached OSS format mask for signed 8-bit PCM.

    RESULT
        TRUE if AFMT_S8 is set; otherwise FALSE.

    NOTES
        Call after a successful OSS_Open().
        A nonzero mask is reused without querying the device. Close and
        reopen do not invalidate it. A zero mask is queried again when a
        descriptor is available. Host query failures are not reported
        separately, so FALSE does not prove that a format is unsupported.

*****************************************************************************/
BOOL OSS_FormatSupported_S8(void)
{
    if (!get_supported_fmts()) return FALSE;
    
    return (audio_supported_fmts & AFMT_S8) ? TRUE : FALSE;
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_FormatSupported_U8(

    SYNOPSIS
        void)

    FUNCTION
        Test the cached OSS format mask for unsigned 8-bit PCM.

    RESULT
        TRUE if AFMT_U8 is set; otherwise FALSE.

    NOTES
        Call after a successful OSS_Open().
        A nonzero mask is reused without querying the device. Close and
        reopen do not invalidate it. A zero mask is queried again when a
        descriptor is available. Host query failures are not reported
        separately, so FALSE does not prove that a format is unsupported.

*****************************************************************************/
BOOL OSS_FormatSupported_U8(void)
{
    if (!get_supported_fmts()) return FALSE;
    
    return (audio_supported_fmts & AFMT_U8) ? TRUE : FALSE;
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_FormatSupported_S16LE(

    SYNOPSIS
        void)

    FUNCTION
        Test the cached OSS format mask for signed 16-bit little-endian PCM.

    RESULT
        TRUE if AFMT_S16_LE is set; otherwise FALSE.

    NOTES
        Call after a successful OSS_Open().
        A nonzero mask is reused without querying the device. Close and
        reopen do not invalidate it. A zero mask is queried again when a
        descriptor is available. Host query failures are not reported
        separately, so FALSE does not prove that a format is unsupported.

*****************************************************************************/
BOOL OSS_FormatSupported_S16LE(void)
{
    if (!get_supported_fmts()) return FALSE;
    
    return (audio_supported_fmts & AFMT_S16_LE) ? TRUE : FALSE;
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_FormatSupported_S16BE(

    SYNOPSIS
        void)

    FUNCTION
        Test the cached OSS format mask for signed 16-bit big-endian PCM.

    RESULT
        TRUE if AFMT_S16_BE is set; otherwise FALSE.

    NOTES
        Call after a successful OSS_Open().
        A nonzero mask is reused without querying the device. Close and
        reopen do not invalidate it. A zero mask is queried again when a
        descriptor is available. Host query failures are not reported
        separately, so FALSE does not prove that a format is unsupported.

*****************************************************************************/
BOOL OSS_FormatSupported_S16BE(void)
{
    if (!get_supported_fmts()) return FALSE;
    
    return (audio_supported_fmts & AFMT_S16_BE) ? TRUE : FALSE;
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_FormatSupported_U16LE(

    SYNOPSIS
        void)

    FUNCTION
        Test the cached OSS format mask for unsigned 16-bit little-endian PCM.

    RESULT
        TRUE if AFMT_U16_LE is set; otherwise FALSE.

    NOTES
        Call after a successful OSS_Open().
        A nonzero mask is reused without querying the device. Close and
        reopen do not invalidate it. A zero mask is queried again when a
        descriptor is available. Host query failures are not reported
        separately, so FALSE does not prove that a format is unsupported.

*****************************************************************************/
BOOL OSS_FormatSupported_U16LE(void)
{
    if (!get_supported_fmts()) return FALSE;
    
    return (audio_supported_fmts & AFMT_U16_LE) ? TRUE : FALSE;
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_FormatSupported_U16BE(

    SYNOPSIS
        void)

    FUNCTION
        Test the cached OSS format mask for unsigned 16-bit big-endian PCM.

    RESULT
        TRUE if AFMT_U16_BE is set; otherwise FALSE.

    NOTES
        Call after a successful OSS_Open().
        A nonzero mask is reused without querying the device. Close and
        reopen do not invalidate it. A zero mask is queried again when a
        descriptor is available. Host query failures are not reported
        separately, so FALSE does not prove that a format is unsupported.

*****************************************************************************/
BOOL OSS_FormatSupported_U16BE(void)
{
    if (!get_supported_fmts()) return FALSE;
    
    return (audio_supported_fmts & AFMT_U16_BE) ? TRUE : FALSE;
}

/******************************************************************************/

int audio_capabilities;

static BOOL get_capabilities(void)
{
    if (audio_capabilities) return TRUE;
    if (audio_fd < 0) return FALSE;
    
    Hidd_UnixIO_IOControlFile(unixio, audio_fd, SNDCTL_DSP_GETCAPS, &audio_capabilities, NULL);
    
    return TRUE;
    
}

/******************************************************************************/

/*****************************************************************************
    NAME
        int OSS_Revision(

    SYNOPSIS
        void)

    FUNCTION
        Read the revision bits of the cached OSS capability mask.

    RESULT
        The mask ANDed with DSP_CAP_REVISION, or zero if no mask is
        available. This is not the oss.library version.

    NOTES
        Call after a successful OSS_Open(). A nonzero capability mask
        is reused across close and reopen. A zero mask is queried again
        when a descriptor is available. Host query failures have no
        separate error result; cached bits may describe an earlier device.

*****************************************************************************/
int OSS_Revision(void)
{
    if (!get_capabilities()) return 0;
    
    return audio_capabilities & DSP_CAP_REVISION;
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_Capability_Duplex(

    SYNOPSIS
        void)

    FUNCTION
        Test the cached OSS capability mask for full-duplex support.

    RESULT
        TRUE if DSP_CAP_DUPLEX is set; otherwise FALSE.

    NOTES
        Call after a successful OSS_Open(). A nonzero capability mask
        is reused across close and reopen. A zero mask is queried again
        when a descriptor is available. Host query failures have no
        separate error result; cached bits may describe an earlier device.

*****************************************************************************/
BOOL OSS_Capability_Duplex(void)
{
    if (!get_capabilities()) return 0;
    
    return (audio_capabilities & DSP_CAP_DUPLEX) ? TRUE : FALSE;
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_Capability_Realtime(

    SYNOPSIS
        void)

    FUNCTION
        Test the cached OSS capability mask for the host real-time capability.

    RESULT
        TRUE if DSP_CAP_REALTIME is set; otherwise FALSE.

    NOTES
        Call after a successful OSS_Open(). A nonzero capability mask
        is reused across close and reopen. A zero mask is queried again
        when a descriptor is available. Host query failures have no
        separate error result; cached bits may describe an earlier device.
        The flag does not guarantee AROS scheduling or latency.

*****************************************************************************/
BOOL OSS_Capability_Realtime(void)
{
    if (!get_capabilities()) return 0;
    
    return (audio_capabilities & DSP_CAP_REALTIME) ? TRUE : FALSE;
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_Capability_Trigger(

    SYNOPSIS
        void)

    FUNCTION
        Test the cached OSS capability mask for trigger-control support.

    RESULT
        TRUE if DSP_CAP_TRIGGER is set; otherwise FALSE.

    NOTES
        Call after a successful OSS_Open(). A nonzero capability mask
        is reused across close and reopen. A zero mask is queried again
        when a descriptor is available. Host query failures have no
        separate error result; cached bits may describe an earlier device.

*****************************************************************************/
BOOL OSS_Capability_Trigger(void)
{
    if (!get_capabilities()) return 0;
    
    return (audio_capabilities & DSP_CAP_TRIGGER) ? TRUE : FALSE;
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_Capability_MMap(

    SYNOPSIS
        void)

    FUNCTION
        Test the cached OSS capability mask for host memory-mapping support.

    RESULT
        TRUE if DSP_CAP_MMAP is set; otherwise FALSE.

    NOTES
        Call after a successful OSS_Open(). A nonzero capability mask
        is reused across close and reopen. A zero mask is queried again
        when a descriptor is available. Host query failures have no
        separate error result; cached bits may describe an earlier device.
        OSS_MMap() remains unimplemented even when this flag is set.

*****************************************************************************/
BOOL OSS_Capability_MMap(void)
{
    if (!get_capabilities()) return 0;
    
    return (audio_capabilities & DSP_CAP_MMAP) ? TRUE : FALSE;
}

/******************************************************************************/

static BOOL set_format(int fmt)
{
    if (audio_fd >= 0)
    {
        int val = fmt;
        int retval = Hidd_UnixIO_IOControlFile(unixio, audio_fd, SNDCTL_DSP_SETFMT, &val, NULL);

        if ((val != fmt) || (retval < 0))
        {
            return FALSE;
        }
        
        return TRUE;
        
    }
    else
    {
        return FALSE;
    }
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_SetFormat_S8(

    SYNOPSIS
        void)

    FUNCTION
        Request signed 8-bit PCM with SNDCTL_DSP_SETFMT.

    RESULT
        TRUE only if the host call succeeds and returns AFMT_S8.
        FALSE if no device is open, the call fails, or another format
        is returned.

    NOTES
        Requires a successful OSS_Open() and an open device.
        FALSE does not imply that the device format is unchanged. A host
        substitution is not rolled back.

*****************************************************************************/
BOOL OSS_SetFormat_S8(void)
{
    return set_format(AFMT_S8);
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_SetFormat_U8(

    SYNOPSIS
        void)

    FUNCTION
        Request unsigned 8-bit PCM with SNDCTL_DSP_SETFMT.

    RESULT
        TRUE only if the host call succeeds and returns AFMT_U8.
        FALSE if no device is open, the call fails, or another format
        is returned.

    NOTES
        Requires a successful OSS_Open() and an open device.
        FALSE does not imply that the device format is unchanged. A host
        substitution is not rolled back.

*****************************************************************************/
BOOL OSS_SetFormat_U8(void)
{
    return set_format(AFMT_U8);
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_SetFormat_S16LE(

    SYNOPSIS
        void)

    FUNCTION
        Request signed 16-bit little-endian PCM with SNDCTL_DSP_SETFMT.

    RESULT
        TRUE only if the host call succeeds and returns AFMT_S16_LE.
        FALSE if no device is open, the call fails, or another format
        is returned.

    NOTES
        Requires a successful OSS_Open() and an open device.
        FALSE does not imply that the device format is unchanged. A host
        substitution is not rolled back.

*****************************************************************************/
BOOL OSS_SetFormat_S16LE(void)
{
    return set_format(AFMT_S16_LE);
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_SetFormat_S16BE(

    SYNOPSIS
        void)

    FUNCTION
        Request signed 16-bit big-endian PCM with SNDCTL_DSP_SETFMT.

    RESULT
        TRUE only if the host call succeeds and returns AFMT_S16_BE.
        FALSE if no device is open, the call fails, or another format
        is returned.

    NOTES
        Requires a successful OSS_Open() and an open device.
        FALSE does not imply that the device format is unchanged. A host
        substitution is not rolled back.

*****************************************************************************/
BOOL OSS_SetFormat_S16BE(void)
{
    return set_format(AFMT_S16_BE);
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_SetFormat_U16LE(

    SYNOPSIS
        void)

    FUNCTION
        Request signed 16-bit little-endian PCM with SNDCTL_DSP_SETFMT.

    RESULT
        TRUE only if the host call succeeds and returns AFMT_S16_LE.
        FALSE if no device is open, the call fails, or another format
        is returned.

    NOTES
        Requires a successful OSS_Open() and an open device.
        FALSE does not imply that the device format is unchanged. A host
        substitution is not rolled back.

    BUGS
        Despite its name, this function requests AFMT_S16_LE and
        does not select unsigned PCM.

*****************************************************************************/
BOOL OSS_SetFormat_U16LE(void)
{
    return set_format(AFMT_S16_LE);
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_SetFormat_U16BE(

    SYNOPSIS
        void)

    FUNCTION
        Request signed 16-bit big-endian PCM with SNDCTL_DSP_SETFMT.

    RESULT
        TRUE only if the host call succeeds and returns AFMT_S16_BE.
        FALSE if no device is open, the call fails, or another format
        is returned.

    NOTES
        Requires a successful OSS_Open() and an open device.
        FALSE does not imply that the device format is unchanged. A host
        substitution is not rolled back.

    BUGS
        Despite its name, this function requests AFMT_S16_BE and
        does not select unsigned PCM.

*****************************************************************************/
BOOL OSS_SetFormat_U16BE(void)
{
    return set_format(AFMT_S16_BE);
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_SetMono(

    SYNOPSIS
        void)

    FUNCTION
        Request mono operation with SNDCTL_DSP_STEREO.

    RESULT
        TRUE if the host call succeeds and returns 0. FALSE if no
        device is open, the call fails, or another value is returned.

    NOTES
        Requires a successful OSS_Open() and an open device.
        A rejected or substituted result is not rolled back.

*****************************************************************************/
BOOL OSS_SetMono(void)
{
    int val = 0;
    int retval;
    
    if (audio_fd < 0) return FALSE;
    
    retval = Hidd_UnixIO_IOControlFile(unixio, audio_fd, SNDCTL_DSP_STEREO, &val, NULL);
    
    if ((retval < 0) || (val != 0)) return FALSE;
    
    return TRUE;
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_SetStereo(

    SYNOPSIS
        void)

    FUNCTION
        Request stereo operation with SNDCTL_DSP_STEREO.

    RESULT
        TRUE if the host call succeeds and returns 1. FALSE if no
        device is open, the call fails, or another value is returned.

    NOTES
        Requires a successful OSS_Open() and an open device.
        A rejected or substituted result is not rolled back.

*****************************************************************************/
BOOL OSS_SetStereo(void)
{
    int val = 1;
    int retval;
    
    if (audio_fd < 0) return FALSE;
    
    retval = Hidd_UnixIO_IOControlFile(unixio, audio_fd, SNDCTL_DSP_STEREO, &val, NULL);
    
    if ((retval < 0) || (val != 1)) return FALSE;
    
    return TRUE;
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_SetNumChannels(

    SYNOPSIS
        int numchannels)

    FUNCTION
        Request a channel count with SNDCTL_DSP_CHANNELS.

    INPUTS
        numchannels - Requested channel count. The value is passed to the
                      host without validation.

    RESULT
        TRUE if the channel-control call succeeds, or if its stereo/mono
        fallback succeeds. FALSE if no device is open or both attempts
        fail.

    NOTES
        Requires a successful OSS_Open() and an open device.
        If the host channel-control call fails, counts greater than one
        fall back to stereo; all other counts fall back to mono.

    BUGS
        The accepted channel count is not checked or returned. TRUE does
        not establish that the requested count was selected.

*****************************************************************************/
BOOL OSS_SetNumChannels(int numchannels)
{
    int val = numchannels;
    int retval;
    
    if (audio_fd < 0) return FALSE;
    
    retval = Hidd_UnixIO_IOControlFile(unixio, audio_fd, SNDCTL_DSP_CHANNELS, &val, NULL);
    if (retval < 0)
    {
        return (numchannels > 1) ? OSS_SetStereo() : OSS_SetMono();
    }
    
    return TRUE;
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_SetWriteRate(

    SYNOPSIS
        int rate, int *used_rate)

    FUNCTION
        Request a sample rate with SNDCTL_DSP_SPEED.

    INPUTS
        rate - Requested samples per second, passed without validation.
        used_rate - Optional writable int receiving the rate returned by
                    the host. May be NULL.

    RESULT
        TRUE if the host call succeeds. FALSE if no device is open or the
        call fails; used_rate is then left unchanged.

    NOTES
        Requires a successful OSS_Open() and an open device.
        The accepted rate may differ from rate. Inspect used_rate when
        the exact playback rate matters.

*****************************************************************************/
BOOL OSS_SetWriteRate(int rate, int *used_rate)
{
    int val = rate;
    int retval;
    
    if (audio_fd < 0) return FALSE;
    
    retval = Hidd_UnixIO_IOControlFile(unixio, audio_fd, SNDCTL_DSP_SPEED, &val, NULL);
    if (retval < 0)
    {
        return FALSE;
    }
    
    if (used_rate) *used_rate = val;
    
    return TRUE;
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_MMap(

    SYNOPSIS
        APTR *mapped_address, int len, BOOL read, BOOL write)

    FUNCTION
        Report that OSS buffer mapping is not implemented.

    INPUTS
        mapped_address - Unused; not written. May be NULL.
        len - Unused.
        read - Unused.
        write - Unused.

    RESULT
        Always FALSE.

    NOTES
        Prints a diagnostic on each call. No memory is mapped, regardless
        of OSS_Capability_MMap().

*****************************************************************************/
BOOL OSS_MMap(APTR *mapped_address, int len, BOOL read, BOOL write)
{
#if 1
/* FIXME: Can't use mmap yet! */
    kprintf("\n=== Dont' call OSS_MMap! Not implemented yet! ===\n\n");
    return FALSE;
#else
    APTR buf;
    int protection;
    
    if (audio_fd < 0) return FALSE;
    
    if (read && write)
    {
        protection = PROT_READ | PROT_WRITE;
    }
    else if (read)
    {
        protection = PROT_READ;
    }
    else
    {
        protection = PROT_WRITE;
    }

    buf = (APTR)mmap(NULL, len, protection, MAP_SHARED, audio_fd, 0);
    if (buf == MAP_FAILED)
    {
        return FALSE;
    }
    
    *mapped_address = buf;
#endif
    
    return TRUE;
    
}

/******************************************************************************/

/*****************************************************************************
    NAME
        void OSS_MUnmap(

    SYNOPSIS
        APTR mapped_address, int len)

    FUNCTION
        Report that OSS buffer unmapping is not implemented.

    INPUTS
        mapped_address - Unused.
        len - Unused.

    RESULT
        None.

    NOTES
        Prints a diagnostic on each call. No mapping or memory is freed.

*****************************************************************************/
void OSS_MUnmap(APTR mapped_address, int len)
{
#if 1
/* FIXME: Can't use munmap yet! */
    kprintf("\n=== Dont' call OSS_MUnmap! Not implemented yet! ===\n\n");
#else
    if ((audio_fd >= 0) && (mapped_address != MAP_FAILED))
    {
        munmap(mapped_address, len);
    }
#endif
}

/******************************************************************************/

/*****************************************************************************
    NAME
        BOOL OSS_SetTrigger(

    SYNOPSIS
        BOOL input, BOOL output)

    FUNCTION
        Set the requested input and output trigger bits.

    INPUTS
        input - TRUE includes PCM_ENABLE_INPUT.
        output - TRUE includes PCM_ENABLE_OUTPUT.

    RESULT
        TRUE if SNDCTL_DSP_SETTRIGGER returns a nonnegative value. FALSE
        if no device is open or the host call fails.

    NOTES
        Requires a successful OSS_Open() and an open device.
        Both FALSE sends zero. The value returned through the host
        argument is not checked.

*****************************************************************************/
BOOL OSS_SetTrigger(BOOL input, BOOL output)
{
    int val = 0;
    int retval;
    
    if (audio_fd < 0) return FALSE;
    
    if (input) val |= PCM_ENABLE_INPUT;
    if (output) val |= PCM_ENABLE_OUTPUT;

    retval = Hidd_UnixIO_IOControlFile(unixio, audio_fd, SNDCTL_DSP_SETTRIGGER, &val, NULL);
    
    return (retval < 0) ? FALSE : TRUE;
}

/******************************************************************************/

/*****************************************************************************
    NAME
        int OSS_Write(

    SYNOPSIS
        APTR buf, int size)

    FUNCTION
        Write audio bytes to the open host device.

    INPUTS
        buf - Buffer containing at least size readable bytes. Keep it
              valid and unchanged until the call returns.
        size - Nonnegative byte count, not a sample or frame count.

    RESULT
        A nonnegative byte count, which may be smaller than size.
        -1 means no open device or an unclassified host error.
        -2 means the host returned EAGAIN.
        -3 means the host returned EINTR.
        -4 means the host write failed with an error value of zero.

    NOTES
        Requires a successful OSS_Open() and an open device.
        The call follows the device's blocking mode. It does not retry
        short writes or errors. Advance the buffer only by the number
        of bytes actually written. No buffer pointer is retained.

*****************************************************************************/
int OSS_Write(APTR buf, int size)
{
    int written;
    int Errno;
    
    if (audio_fd < 0)
    {
        return -1;
    }
    
    written = Hidd_UnixIO_WriteFile(unixio, audio_fd, buf, size, &Errno);
    if (written == -1)
    {
        switch(Errno)
        {
            case EAGAIN:
                written = -2; /* Retval -2. Caller should treat it like EAGAIN. */
                break;
                
            case EINTR:
                written = -3; /* Retval -3. Caller should treat it like EINTR. */
                break;
                
            case 0:
                written = -4; /* Retval -4. Caller should treat it like a 0-Errno.
                                 (but since retval of write() was -1, like EAGAIN
                                 maybe?) */
                break;
        }
                
    }
    
    return written;
}

/******************************************************************************/
