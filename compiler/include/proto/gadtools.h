/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_GADTOOLS_H
#define PROTO_GADTOOLS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/gadtools_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/gadtools.h>
#else
#include <defines/gadtools.h>
#endif

#endif /* PROTO_GADTOOLS_H */
