/*
    (C) 1995-97 AROS - The Amiga Research OS
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
