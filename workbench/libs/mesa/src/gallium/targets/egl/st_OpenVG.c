#include <proto/exec.h>
#include <proto/vega.h>
#include <aros/symbolsets.h>

#include "egl.h"

struct Library * VegaBase = NULL;

PUBLIC struct st_api * st_api_create_OpenVG(void)
{
    if (!VegaBase)
        VegaBase = OpenLibrary("vega.library", 0L);

    if (VegaBase)
        return (struct st_api *) GetOpenVGStateTrackerApi();
    else
        return NULL;
}

static VOID CloseVega()
{
    if (VegaBase)
        CloseLibrary(VegaBase);
}

ADD2EXPUNGELIB(CloseVega, 5)
