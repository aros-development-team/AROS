/*
    (C) 1995-97 AROS - The Amiga Research OS
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
