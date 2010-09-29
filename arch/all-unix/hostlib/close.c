#include <aros/debug.h>
#include <aros/libcall.h>

#include <dlfcn.h>

AROS_LH2(int, HostLib_Close,
         AROS_LHA(void *,  handle, A0),
         AROS_LHA(char **, error,  A1),
         struct HostLibBase *, HostLibBase, 2, HostLib)
{
    AROS_LIBFUNC_INIT

    int ret;

    D(bug("[hostlib] Close: handle=0x%08x\n", handle));

    ret = dlclose(handle);

    if (error != NULL)
        *error = ret != 0 ? dlerror() : NULL;

    return ret;

    AROS_LIBFUNC_EXIT
}
