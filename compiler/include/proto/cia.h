/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROTO_CIA_H
#define PROTO_CIA_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/cia_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/cia.h>
#else
#include <defines/cia.h>
#endif

#endif /* PROTO_CIA_H */
