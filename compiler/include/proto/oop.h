/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_OOP_H
#define PROTO_OOP_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#define OOPNAME  "oop.library"

#ifndef OOPBase
extern struct Library * OOPBase;
#endif

#include <clib/oop_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/oop.h>
#else
#include <defines/oop.h>
#endif

#endif /* PROTO_OOP_H */
