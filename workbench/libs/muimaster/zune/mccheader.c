/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef MCC_HEADER_C
#define MCC_HEADER_C

/*
 * To be included in only one file from a MCC source files.
 * Before including this, you must define the following things:
 * SUPERCLASS - name of your mcc superclass
 * UserLibName - name used to find your class
 * struct Data - instance data of your class
 * _Dispatcher - dispatcher of your class
 * BOOL ClassInit (void) - called at module loading. TRUE = success.
 * void ClassExit (void) - called at module unloading
 */

/*
 * Exported Functions declarations
 */
G_MODULE_EXPORT const gchar *g_module_check_init (GModule *module);
G_MODULE_EXPORT void g_module_unload (GModule *module);
G_MODULE_EXPORT ULONG MCC_Query (LONG which);

static struct MUI_CustomClass *ThisClass = NULL;

/******************************************************************************/
/* Functions definitions                                                      */
/******************************************************************************/

G_MODULE_EXPORT const gchar *
g_module_check_init (GModule *module)
{
    struct Library base;

    base.lib_Node.ln_Name = (char *)UserLibName;

    ThisClass = MUI_CreateCustomClass(&base,
				      SUPERCLASS, NULL,
				      sizeof(struct Data), _Dispatcher);
    if (ThisClass)
    {
	if (ClassInit())
	    return NULL;
	MUI_DeleteCustomClass(ThisClass);
	return "Cannot initialize custom class";
    }
    return "Cannot create custom class";
}


G_MODULE_EXPORT void 
g_module_unload (GModule *module)
{
    g_warning("At last ! g_module_unload was called for module %s\n",
	      g_module_name(module));
    
    ClassExit();
    if (ThisClass)
    {
	MUI_DeleteCustomClass(ThisClass);
	ThisClass = NULL;
    }
}


G_MODULE_EXPORT ULONG 
MCC_Query (LONG which)
{
    switch (which)
    {
	case 0:
	    return (ULONG)ThisClass;
    }
    return 0;
}



#endif
