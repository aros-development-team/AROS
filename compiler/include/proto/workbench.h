/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_WORKBENCH_H
#define PROTO_WORKBENCH_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/workbench_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/workbench.h>
#else
#include <defines/workbench.h>
#endif

#endif /* PROTO_WORKBENCH_H */
