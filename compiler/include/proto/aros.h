/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_AROS_H
#define PROTO_AROS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#ifndef ArosBase
extern struct Library * ArosBase;
#endif

#include <clib/aros_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/aros.h>
#else
#include <defines/aros.h>
#endif

#endif /* PROTO_AROS_H */
