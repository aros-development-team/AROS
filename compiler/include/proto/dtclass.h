/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROTO_DTCLASS_H
#define PROTO_DTCLASS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/dtclass_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/dtclass.h>
#else
#include <defines/dtclass.h>
#endif

#endif /* PROTO_DTCLASS_H */
