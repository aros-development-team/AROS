/*
    Copyright © 2007-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "uuid_private.h"
#include LC_LIBDEFS_FILE

static char value(char chr)
{
    if (isdigit(chr))
        chr -= '0';
    else
        chr -= isupper(chr) ? 'A' - 10 : 'a' - 10;
    return chr;
}

AROS_LH2(int, UUID_Parse,
         AROS_LHA(const char *, in, A0),
         AROS_LHA(uuid_t *, uuid, A1),
         struct uuid_base *, UUIDBase, 7, UUID)
{
    AROS_LIBFUNC_INIT
    
    int i;
    uuid_t tmp = {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0}};
    
    D(bug("[UUID] UUID_Parse(%s, %p)\n", in, uuid));
    
    if (strlen(in) == UUID_STRLEN)
    {
        for (i=0; i < 8; i++, in++)
        {
            if (isxdigit(*in))
            {
                tmp.time_low = (tmp.time_low  << 4) | value(*in); 
            }
            else return 0;
        }
        
        if (*in++ != '-')
            return 0;
        
        for (i=0; i < 4; i++, in++)
        {
            if (isxdigit(*in))
            {
                tmp.time_mid = (tmp.time_mid << 4) | value(*in); 
            }
            else return 0;
        }

        if (*in++ != '-')
            return 0;
        
        for (i=0; i < 4; i++, in++)
        {
            if (isxdigit(*in))
            {
                tmp.time_hi_and_version = 
                    (tmp.time_hi_and_version << 4) | value(*in); 
            }
            else return 0;
        }

        if (*in++ != '-')
            return 0;
                
        if (!(isxdigit(in[0]) && isxdigit(in[1])))
            return 0;
        
        tmp.clock_seq_hi_and_reserved = (value(in[0]) << 4) | value(in[1]);
        
        if (!(isxdigit(in[2]) && isxdigit(in[3])))
            return 0;
        
        tmp.clock_seq_low = (value(in[2]) << 4) | value(in[3]);
        in +=4;
        
        if (*in++ != '-')
            return 0;
        
        for (i=0; i < 6; i++, in+=2)
        {
            if (!(isxdigit(in[0]) && isxdigit(in[1])))
                return 0;

            tmp.node[i] = (value(in[0]) << 4) | value(in[1]);
        }
        
        *uuid = tmp;
        return 1;
    }
    
    return 0;
    
    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, UUID_Unparse,
         AROS_LHA(const uuid_t *, uuid, A0),
         AROS_LHA(char *, out, A1),
         struct uuid_base *, UUIDBase, 8, UUID)
{
    AROS_LIBFUNC_INIT

    snprintf(out, UUID_STRLEN + 1,
            "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            (unsigned int)uuid->time_low,
            uuid->time_mid,
            uuid->time_hi_and_version,
            uuid->clock_seq_hi_and_reserved,
            uuid->clock_seq_low,
            uuid->node[0],uuid->node[1],uuid->node[2],
            uuid->node[3],uuid->node[4],uuid->node[5]);
    
    AROS_LIBFUNC_EXIT
}
