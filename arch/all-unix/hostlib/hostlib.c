#include <exec/types.h>
#include <aros/libcall.h>

#define timeval sys_timeval
#include <dlfcn.h>
#undef timeval

#include "hostlib_intern.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

AROS_LH2(void *, HostLib_Open,
         AROS_LHA(const char *, filename, A0),
         AROS_LHA(char **,      error,    A1),
         struct HostLibBase *, HostLibBase, 1, HostLib) {

    AROS_LIBFUNC_INIT

    void *handle;

    D(bug("[hostlib] Open: filename=%s\n", filename));
    
    handle = dlopen((char *) filename, RTLD_NOW);

    if (error != NULL)
        *error = handle != NULL ? dlerror() : NULL;

    return handle;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(int, HostLib_Close,
         AROS_LHA(void *,  handle, A0),
         AROS_LHA(char **, error,  A1),
         struct HostLibBase *, HostLibBase, 2, HostLib) {

    AROS_LIBFUNC_INIT

    int ret;

    D(bug("[hostlib] Close: handle=0x%08x\n", handle));

    ret = dlclose(handle);

    if (error != NULL)
        *error = ret != 0 ? dlerror() : NULL;

    return ret;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void *, HostLib_GetPointer,
         AROS_LHA(void *,       handle, A0),
         AROS_LHA(const char *, symbol, A1),
         AROS_LHA(char **,      error,  A2),
         struct HostLibBase *, HostLibBase, 3, HostLib) {

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
