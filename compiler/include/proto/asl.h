/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_ASL_H
#define PROTO_ASL_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/asl_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/asl.h>
#else
#include <defines/asl.h>
#endif

#endif /* PROTO_ASL_H */
