#include <proto/exec.h>
#include <proto/mesa.h>
#include <aros/symbolsets.h>

#include "egl.h"

struct Library * MesaBase = NULL;

PUBLIC struct st_api * st_api_create_OpenGL(void)
{
    if (!MesaBase)
        MesaBase = OpenLibrary("mesa.library", 0L);

    if (MesaBase)
        return (struct st_api *) AROSMesaGetOpenGLStateTrackerApi();
    else
        return NULL;
}

static VOID CloseMesa()
{
    if (MesaBase)
        CloseLibrary(MesaBase);
}

ADD2EXPUNGELIB(CloseMesa, 5)
