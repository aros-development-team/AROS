/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_GADTOOLS_H
#define PROTO_GADTOOLS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

extern struct Library * GadToolsBase;

#include <clib/gadtools_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/gadtools.h>
#else
#include <defines/gadtools.h>
#endif

#endif /* PROTO_GADTOOLS_H */
