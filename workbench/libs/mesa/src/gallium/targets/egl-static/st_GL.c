#include <proto/exec.h>
#if !defined(__mc68000)
#include <proto/gl.h>
#endif
#include <aros/symbolsets.h>

#include "egl_st.h"

struct Library * GLBase = NULL;

PUBLIC struct st_api * st_gl_api_create(void)
{
#if !defined(__mc68000)
    if (!GLBase)
        GLBase = OpenLibrary("gl.library", 20L);

    if (GLBase)
        return (struct st_api *) GetOpenGLStateTrackerApi();
    else
#endif
        return NULL;
}

static VOID CloseMesa()
{
    if (GLBase)
    {
        CloseLibrary(GLBase);
        GLBase = NULL;
    }
}

ADD2EXPUNGELIB(CloseMesa, 5)
