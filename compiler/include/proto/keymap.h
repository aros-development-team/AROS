/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_KEYMAP_H
#define PROTO_KEYMAP_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#ifndef KeymapBase
extern struct Library * KeymapBase;
#endif

#include <clib/keymap_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/keymap.h>
#else
#include <defines/keymap.h>
#endif

#endif /* PROTO_KEYMAP_H */
