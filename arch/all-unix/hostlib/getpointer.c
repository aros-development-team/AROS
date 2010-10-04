#include <aros/debug.h>
#include <aros/libcall.h>

#include <dlfcn.h>

AROS_LH3(void *, HostLib_GetPointer,
         AROS_LHA(void *,       handle, A0),
         AROS_LHA(const char *, symbol, A1),
         AROS_LHA(char **,      error,  A2),
         APTR, HostLibBase, 3, HostLib)
{
    AROS_LIBFUNC_INIT

    void *ptr;

    D(bug("[hostlib] GetPointer: handle=0x%08x, symbol=%s\n", handle, symbol));

    dlerror();

    ptr = dlsym(handle, (char *) symbol);

    if (error != NULL)
        *error = dlerror();

    return ptr;

    AROS_LIBFUNC_EXIT
}
