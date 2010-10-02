#define DEBUG 0
#include <aros/debug.h>
#include <aros/libcall.h>

#include <dlfcn.h>

AROS_LH2(void *, HostLib_Open,
         AROS_LHA(const char *, filename, A0),
         AROS_LHA(char **,      error,    A1),
         struct HostLibBase *, HostLibBase, 1, HostLib)
{

    AROS_LIBFUNC_INIT

    void *handle;

    D(bug("[hostlib] Open: filename=%s\n", filename));
    
    handle = dlopen((char *) filename, RTLD_NOW);

    if (error != NULL)
        *error = handle == NULL ? dlerror() : NULL;

    return handle;

    AROS_LIBFUNC_EXIT
}
