/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_COMMODITIES_H
#define PROTO_COMMODITIES_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#ifndef CxBase
extern struct Library * CxBase;
#endif

#include <clib/commodities_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/commodities.h>
#else
#include <defines/commodities.h>
#endif

#endif /* PROTO_COMMODITIES_H */
