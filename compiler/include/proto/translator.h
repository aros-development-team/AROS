/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROTO_TRANSLATOR_H
#define PROTO_TRANSLATOR_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/translator_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/translator.h>
#else
#include <defines/translator.h>
#endif

#endif /* PROTO_TRANSLATOR_H */
