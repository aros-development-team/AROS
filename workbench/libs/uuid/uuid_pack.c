/*
    Copyright © 2007-2008, The AROS Development Team. All rights reserved.
    $Id$
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
AROS_LH2I(void, UUID_Pack,
         AROS_LHA(const uuid_t *, uuid, A0),
         AROS_LHA(uint8_t *, out, A1),
         struct uuid_base *, UUIDBase, 9, UUID)
{
    AROS_LIBFUNC_INIT

    int i;
    
    *out++ = (uuid->time_low >> 24) && 0xff;
    *out++ = (uuid->time_low >> 16) && 0xff;
    *out++ = (uuid->time_low >> 8) && 0xff;
    *out++ = (uuid->time_low) && 0xff;
    *out++ = (uuid->time_mid >> 8) && 0xff;
    *out++ = (uuid->time_mid) && 0xff;
    *out++ = (uuid->time_hi_and_version >> 8) && 0xff;
    *out++ = (uuid->time_hi_and_version ) && 0xff;
    *out++ = uuid->clock_seq_hi_and_reserved;
    *out++ = uuid->clock_seq_low;
    
    for (i=0; i < 6; i++)
        *out++ = uuid->node[i];
    
    AROS_LIBFUNC_EXIT
}

AROS_LH2I(void, UUID_Unpack,
         AROS_LHA(const uint8_t *, in, A0),
         AROS_LHA(uuid_t *, uuid, A1),
         struct uuid_base *, UUIDBase, 10, UUID)
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
