/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROTO_EXEC_SYSBASE_H
#define PROTO_EXEC_SYSBASE_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/exec_sysbase_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/exec_sysbase.h>
#else
#include <defines/exec_sysbase.h>
#endif

#endif /* PROTO_EXEC_SYSBASE_H */
