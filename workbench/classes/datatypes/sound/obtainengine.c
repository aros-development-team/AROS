#include <datatypes/soundclass.h>
#include <datatypes/datatypesclass.h>
#include <intuition/classusr.h>
#include <intuition/classes.h>
#include <proto/exec.h>

#include "classbase.h"
#include LC_LIBDEFS_FILE

/***************************************************************************************************/

AROS_LH0(struct IClass *, ObtainEngine,
         LIBBASETYPEPTR, LIBBASE, 5, BASENAME)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(LIBBASETYPEPTR, LIBBASE)

    return (((struct ClassBase *)LIBBASE)->cb_Class);
        
    AROS_LIBFUNC_EXIT
}

/***************************************************************************************************/
