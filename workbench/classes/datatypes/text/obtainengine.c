#include <exec/libraries.h>
#include <intuition/classes.h>
#include <proto/exec.h>
#include <aros/libcall.h>
#include "compilerspecific.h"

#include "text_intern.h"

#if defined(__AROS__) && !defined(__MORPHOS__)
#include LC_LIBDEFS_FILE
#else
#include "libdefs.h"
#endif

extern SAVEDS STDARGS struct IClass *ObtainEngine(struct TextBase_intern *libbase);

/***************************************************************************************************/

AROS_LH0(struct IClass *, ObtainEngine,
         LIBBASETYPEPTR, LIBBASE, 5, BASENAME)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(LIBBASETYPEPTR, LIBBASE)

    return ObtainEngine(LIBBASE);
    
    AROS_LIBFUNC_EXIT
}

/***************************************************************************************************/
