/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_MATHTRANS_H
#define PROTO_MATHTRANS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/mathtrans_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/mathtrans.h>
#else
#include <defines/mathtrans.h>
#endif

#endif /* PROTO_MATHTRANS_H */
