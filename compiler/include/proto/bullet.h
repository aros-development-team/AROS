/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROTO_BULLET_H
#define PROTO_BULLET_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/bullet_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/bullet.h>
#else
#include <defines/bullet.h>
#endif

#endif /* PROTO_BULLET_H */
