/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_EXEC_H
#define PROTO_EXEC_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/exec_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/exec.h>
#else
#include <defines/exec.h>
#endif

/* Common support functions */
#ifndef PROTO_ALIB_H
#include <proto/alib.h>
#endif

#endif /* PROTO_EXEC_H */
