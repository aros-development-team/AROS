/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_CYBERGRAPHICS_H
#define PROTO_CYBERGRAPHICS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#define CGFXNAME  "cybergraphics.library"

#include <clib/cybergraphics_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/cybergraphics.h>
#else
#include <defines/cybergraphics.h>
#endif

#endif /* PROTO_CYBERGRAPHICS_H */
