/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROTO_CARDRES_H
#define PROTO_CARDRES_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/cardres_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/cardres.h>
#else
#include <defines/cardres.h>
#endif

#endif /* PROTO_CARDRES_H */
