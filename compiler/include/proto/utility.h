/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_UTILITY_H
#define PROTO_UTILITY_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#ifndef UtilityBase
  extern struct UtilityBase * UtilityBase;
#endif

#include <clib/utility_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/utility.h>
#else
#include <defines/utility.h>
#endif

#endif /* PROTO_UTILITY_H */
