/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_LOCALE_H
#define PROTO_LOCALE_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#ifndef LocaleBase
extern struct Library * LocaleBase;
#endif

#include <clib/locale_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/locale.h>
#else
#include <defines/locale.h>
#endif

#endif /* PROTO_LOCALE_H */
