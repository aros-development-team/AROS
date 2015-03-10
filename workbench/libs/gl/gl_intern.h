/*
    Copyright 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef GL_INTERN_H
#define GL_INTERN_H

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef PROTO_EXEC_H
#   include <proto/exec.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif

#include <dos/notify.h>

#include "gl_libdefs.h"

struct LIBBASE
{
    struct Library              glb_Lib;
    struct Library              *glb_DOS;
    struct SignalSemaphore      glb_Sem;
    struct NotifyRequest        glb_Notify;
    char                        *glb_GLImpl;
};

#define GLB(lb)  ((struct LIBBASE *)lb)

#endif /* GL_INTERN_H */
