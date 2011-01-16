#include <proto/exec.h>
#if !defined(__mc68000)
#include <proto/mesa.h>
#endif
#include <aros/symbolsets.h>

#include "egl.h"

struct Library * MesaBase = NULL;

PUBLIC struct st_api * st_api_create_OpenGL(void)
{
#if !defined(__mc68000)
    if (!MesaBase)
        MesaBase = OpenLibrary("mesa.library", 0L);

    if (MesaBase)
        return (struct st_api *) AROSMesaGetOpenGLStateTrackerApi();
    else
#endif
        return NULL;
}

static VOID CloseMesa()
{
    if (MesaBase)
        CloseLibrary(MesaBase);
}

ADD2EXPUNGELIB(CloseMesa, 5)
