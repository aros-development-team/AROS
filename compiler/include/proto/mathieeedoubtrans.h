/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_MATHIEEEDOUBTRANS_H
#define PROTO_MATHIEEEDOUBTRANS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/mathieeedoubtrans_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/mathieeedoubtrans.h>
#else
#include <defines/mathieeedoubtrans.h>
#endif

#endif /* PROTO_MATHIEEEDOUBTRANS_H */
