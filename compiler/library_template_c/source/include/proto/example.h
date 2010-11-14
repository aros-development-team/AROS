/*
**	$VER: example.h 37.1 (4.12.96)
**
**	prototype/pragma include for example.library
**
**	(C) Copyright 1996 Andreas R. Kleinert
**	All Rights Reserved.
*/

#ifndef PROTO_EXAMPLE_H
#define PROTO_EXAMPLE_H

#ifndef __AROS__
#   include <clib/example_protos.h>
#   include <pragmas/example_pragmas.h>
#else
#   ifndef AROS_SYSTEM_H
#	include <aros/system.h>
#   endif

#   include <clib/example_protos.h>

#   if defined(_AMIGA) && defined(__GNUC__)
#	include <inline/example.h>
#   else
#	include <defines/example.h>
#   endif
#endif

#endif /* PROTO_EXAMPLE_H */
