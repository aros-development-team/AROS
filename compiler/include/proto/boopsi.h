/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_BOOPSI_H
#define PROTO_BOOPSI_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#define BOOPSINAME  "boopsi.library"

#ifndef BOOPSIBase
extern struct Library * BOOPSIBase;
#endif

#include <clib/boopsi_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/boopsi.h>
#else
#include <defines/boopsi.h>
#endif

#endif /* PROTO_BOOPSI_H */
