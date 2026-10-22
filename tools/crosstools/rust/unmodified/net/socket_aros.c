/*
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL) Version 1.1.
*/
#include "socket_native.h"
#include <exec/libraries.h>
#include <proto/exec.h>
#include <aros/libcall.h>
#include <errno.h>

int aros_socket_native_call(struct aros_socket_request *request)
{
    struct Library *base = OpenLibrary((CONST_STRPTR)"bsdsocket.library", 4);
    if (!base)
        return ENETDOWN;
    if (base->lib_NegSize < (AROS_SOCKET_VECTOR + 1) * LIB_VECTSIZE) {
        CloseLibrary(base);
        return ENOSYS;
    }
    int error = (int)AROS_LC2(LONG, NativeSocket,
        AROS_LCA(struct aros_socket_request *, request, A0),
        AROS_LCA(ULONG, sizeof(*request), D0),
        struct Library *, base, AROS_SOCKET_VECTOR, BSDSocket);
    /* NativeSocket reports errno by value. Closing this task's temporary
       base cannot invalidate any task-independent descriptor it returned. */
    CloseLibrary(base);
    return error;
}
