/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROTO_CONSOLE_H
#define PROTO_CONSOLE_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/console_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/console.h>
#else
#include <defines/console.h>
#endif

#endif /* PROTO_CONSOLE_H */
