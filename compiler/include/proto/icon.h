/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_ICON_H
#define PROTO_ICON_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#ifndef IconBase
extern struct Library * IconBase;
#endif

#include <clib/icon_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/icon.h>
#else
#include <defines/icon.h>
#endif

#endif /* PROTO_ICON_H */
