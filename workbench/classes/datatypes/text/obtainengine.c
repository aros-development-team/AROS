#include <exec/libraries.h>
#include <intuition/classes.h>
#include <proto/alib.h>
#include "compilerspecific.h"

#include LC_LIBDEFS_FILE

extern SAVEDS STDARGS struct IClass *ObtainEngine(void);

/***************************************************************************************************/

AROS_LH0(struct IClass *, ObtainEngine,
         LIBBASETYPEPTR, LIBBASE, 5, BASENAME)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(LIBBASETYPEPTR, LIBBASE)

    return ObtainEngine();
    
    AROS_LIBFUNC_EXIT
}

/***************************************************************************************************/
