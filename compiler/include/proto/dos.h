/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_DOS_H
#define PROTO_DOS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/dos_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/dos.h>
#else
#include <defines/dos.h>
#endif

#endif /* PROTO_DOS_H */
