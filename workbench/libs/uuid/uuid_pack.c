/*
    Copyright (C) 2007-2008, The AROS Development Team. All rights reserved.
*/

#define DEBUG 1

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "uuid_private.h"
#include LC_LIBDEFS_FILE

/* Convert UUID into 16-byte stream in network (BigEndian) format */
/*****************************************************************************

    NAME */
        AROS_LH2I(void, UUID_Pack,

/*  SYNOPSIS */
         AROS_LHA(const uuid_t *, uuid, A0),
         AROS_LHA(uint8_t *, out, A1),

/*  LOCATION */
         struct uuid_base *, UUIDBase, 9, UUID)
/*  FUNCTION
        Serialize a UUID into a 16-byte sequence. The time_low, time_mid
        and time_hi_and_version fields are written most significant byte
        first, followed by clock_seq_hi_and_reserved, clock_seq_low and
        the six node bytes in their stored order. The result is independent
        of the native byte order of uuid_t.

    INPUTS
        uuid - Non-NULL pointer to a readable uuid_t.
        out - Non-NULL pointer to at least 16 writable bytes. The output
              buffer must not overlap the input object.

    RESULT
        None. Exactly 16 bytes are written to out; no terminator is added.

    NOTES
        Neither buffer size nor UUID validity is checked.

    SEE ALSO
        UUID_Unpack(), UUID_Unparse()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    int i;
    
    *out++ = (uuid->time_low >> 24) & 0xff;
    *out++ = (uuid->time_low >> 16) & 0xff;
    *out++ = (uuid->time_low >> 8) & 0xff;
    *out++ = (uuid->time_low) & 0xff;
    *out++ = (uuid->time_mid >> 8) & 0xff;
    *out++ = (uuid->time_mid) & 0xff;
    *out++ = (uuid->time_hi_and_version >> 8) & 0xff;
    *out++ = (uuid->time_hi_and_version ) & 0xff;
    *out++ = uuid->clock_seq_hi_and_reserved;
    *out++ = uuid->clock_seq_low;
    
    for (i=0; i < 6; i++)
        *out++ = uuid->node[i];
    
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2I(void, UUID_Unpack,

/*  SYNOPSIS */
         AROS_LHA(const uint8_t *, in, A0),
         AROS_LHA(uuid_t *, uuid, A1),

/*  LOCATION */
         struct uuid_base *, UUIDBase, 10, UUID)
/*  FUNCTION
        Deserialize the 16-byte representation produced by UUID_Pack().
        The first four bytes form time_low, the next two time_mid and the
        next two time_hi_and_version, all most significant byte first.
        The remaining bytes supply clock_seq_hi_and_reserved,
        clock_seq_low and the six node bytes.

    INPUTS
        in - Non-NULL pointer to at least 16 readable bytes.
        uuid - Non-NULL pointer to a writable uuid_t. The output object
               must not overlap the input buffer.

    RESULT
        None. All UUID fields are written to *uuid.

    NOTES
        The input is unchanged. Neither buffer size nor UUID validity is
        checked; the nil UUID is accepted.

    SEE ALSO
        UUID_Pack(), UUID_Parse()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    int i;
    uint32_t tmp;
    
    tmp = *in++;
    tmp = (tmp << 8) | *in++;
    tmp = (tmp << 8) | *in++;
    tmp = (tmp << 8) | *in++;
    uuid->time_low = tmp;
    
    tmp = *in++;
    tmp = (tmp << 8) | *in++;
    uuid->time_mid = tmp;
    
    tmp = *in++;
    tmp = (tmp << 8) | *in++;
    uuid->time_hi_and_version = tmp;
    
    uuid->clock_seq_hi_and_reserved = *in++;
    uuid->clock_seq_low = *in++;
    
    for (i=0; i < 6; i++)
        uuid->node[i] = *in++;
    
    AROS_LIBFUNC_EXIT
}
