/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_REQTOOLS_H
#define PROTO_REQTOOLS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#ifndef RTBase
extern struct Library * RTBase;
#endif

#include <clib/reqtools_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/reqtools.h>
#else
#include <defines/reqtools.h>
#endif

#endif /* PROTO_REQTOOLS_H */
