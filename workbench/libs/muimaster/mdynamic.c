#include <exec/types.h>

#ifdef _AROS
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <string.h>
#else
#include <config.h>
#include <gmodule.h>
#endif

#include <zunepriv.h>
#include <file.h>
#include <extclass.h>
#include <mcc.h>

#define QUERY_MCC 0
#define QUERY_MCP 1

/* Classes are searched first in user dir, then in system dir.
 * After all, if they want to force you to use system classes first,
 * they can create a forbidden (chmod 000) user libs dir ?
 * Anyway ...
 * Other search paths are welcome.
 */
Class *
_zune_class_load(CONST_STRPTR className)
{
#ifndef _AROS
    GModule *module;
    STRPTR modpath;
    QueryFunc query;
#else
    struct Library *module;
#endif
    struct MUI_CustomClass *mcc;

#ifndef _AROS
    if (!g_module_supported())
	return NULL;

    
    modpath = g_module_build_path(__zune_file_get_user_libs_dir(), className);
g_print("loading %s in %s\n", className, modpath);
    module = g_module_open (modpath, 0);
    g_free(modpath);
    if (!module)
    {
	modpath = g_module_build_path(__zune_file_get_lib_dir(), className);
g_print("loading %s in %s\n", className, modpath);
	module = g_module_open (modpath, 0);
	g_free(modpath);
	if (!module)
	{
	    g_warning("_zune_class_load : can't load %s\n", className);
	    return NULL;
	}
    }
/*  g_print("module opened !"); */
    if (!g_module_symbol(module, "MCC_Query", (gpointer *)&query))
    {
	g_warning("_zune_class_load : %s\n", g_module_error());
	g_module_close(module);
	return NULL;
    }

    mcc = (struct MUI_CustomClass *)(*query)(QUERY_MCC);
    if (!mcc)
    {
	g_warning("_zune_class_load : dynamic creation of class %s failed\n", className);
	g_module_close(module);
	return NULL;
    }
#else
kprintf("*** _zune_class_load(\"%s\"): ", className);
    module = OpenLibrary(className, 0);
    if (module == NULL)
    {
      char path[256];
      strcpy(path, "mui/");
      strcpy(path+4, className);
kprintf("OpenLibrary(\"%s\") failed, trying OpenLibrary(\"%s\")... ", className, path);
      module = OpenLibrary(path, 0);
      if (module == NULL)
        return NULL;
    }

    /* call MCC_Query() */
    mcc = AROS_LVO_CALL1(ULONG,
        AROS_LCA(LONG,QUERY_MCC,D0),
        struct Library *,module,5,);
    if (!mcc)
    {
kprintf("query failed.\n");
        CloseLibrary(module);
        return NULL;
    }
#endif

kprintf("OK.\n");
    mcc->mcc_Module = module;
    return mcc->mcc_Class;
}
