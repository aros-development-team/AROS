/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_MATHFFP_H
#define PROTO_MATHFFP_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/mathffp_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/mathffp.h>
#else
#include <defines/mathffp.h>
#endif

#endif /* PROTO_MATHFFP_H */
