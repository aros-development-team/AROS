/*
    Copyright © 2007-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "uuid_private.h"
#include LC_LIBDEFS_FILE

/* Name string is a fully-qualified domain name */
static uuid_t NameSpace_DNS = { /* 6ba7b810-9dad-11d1-80b4-00c04fd430c8 */
       0x6ba7b810,
       0x9dad,
       0x11d1,
       0x80, 0xb4, {0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}
};

/* Name string is a URL */
static uuid_t NameSpace_URL = { /* 6ba7b811-9dad-11d1-80b4-00c04fd430c8 */
       0x6ba7b811,
       0x9dad,
       0x11d1,
       0x80, 0xb4, {0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}
};

/* Name string is an ISO OID */
static uuid_t NameSpace_OID = { /* 6ba7b812-9dad-11d1-80b4-00c04fd430c8 */
       0x6ba7b812,
       0x9dad,
       0x11d1,
       0x80, 0xb4, {0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}
};

/* Name string is an X.500 DN (in DER or a text output format) */
static uuid_t NameSpace_X500 = { /* 6ba7b814-9dad-11d1-80b4-00c04fd430c8 */
       0x6ba7b814,
       0x9dad,
       0x11d1,
       0x80, 0xb4, {0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}
};

static uuid_t _null = {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0}};

AROS_LH2(void, UUID_GetNameSpace,
         AROS_LHA(uint32_t, NameSpace, D0),
         AROS_LHA(uuid_t *, uuid, A0),
         struct uuid_base *, UUIDBase, 6, UUID)
{
    AROS_LIBFUNC_INIT

    ASSERT(uuid);
    
    switch(NameSpace)
    {
        case UUID_NAMESPACE_DNS:
            *uuid = NameSpace_DNS;
            break;

        case UUID_NAMESPACE_URL:
            *uuid = NameSpace_URL;
            break;

        case UUID_NAMESPACE_OID:
            *uuid = NameSpace_OID;
            break;
            
        case UUID_NAMESPACE_X500:
            *uuid = NameSpace_X500;
            break;
            
        default:
            *uuid = _null;
            break;
    }

    AROS_LIBFUNC_EXIT
}
