#ifndef PROTO_EXEC_H
#define PROTO_EXEC_H
/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef AROS_SYSTEM_H
#   include <aros/system.h>
#endif

#ifndef SysBase
extern struct ExecBase * SysBase;
#endif

#include <clib/exec_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#   include <inline/exec.h>
#else
#   include <defines/exec.h>
#endif

#if defined(ENABLE_RT) && ENABLE_RT && !defined(ENABLE_RT_EXEC)
#   define ENABLE_RT_EXEC   1
#   include <aros/rt.h>
#endif

#endif /* PROTO_EXEC_H */
