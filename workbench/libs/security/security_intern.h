/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#ifndef _SECURITY_INTERN_H
#define _SECURITY_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif
#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif
#ifndef HIDD_HIDD_H
#include <hidd/hidd.h>
#endif
#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif

#define DEBUG   1

#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <hardware/intbits.h>
#include <asm/segments.h>
#include <asm/linkage.h>
#include <asm/ptrace.h>
#include <dos/dosextens.h>

#include <aros/arossupportbase.h>
#include <aros/asmcall.h>
#include <aros/config.h>
#include <aros/debug.h>
#include <aros/multiboot.h>

#include <hardware/custom.h>

#include LC_LIBDEFS_FILE
#define  DEBUG_NAME_STR                     "[" NAME_STRING "]"

   /*
    *    Extended Owner Information Structure
    *
    *    A pointer to this structure is returned by secGetTaskExtOwner().
    *    You MUST use secFreeExtOwner() to deallocate it!!
    */

struct muExtOwner {
   UWORD                    uid;
   UWORD                    gid;
   UWORD                    NumSecGroups;               /* Number of Secondary Groups this */
                                                        /* user belongs too. */
};


/* Function called to initialize the plugin.
 * The plugin should perform any initialization it needs, and then formally
 * register itself with the security.library.
 * module should be passed to the security.library when a handler is registered.
 * */


/* Plugin Module Definition - must be located in the executeable */
struct secPluginHeader	{
#define secPLUGIN_RECOGNITION	MAKE_ID('m', 'S', 'p', 'I')
    
    ULONG                   plugin_magic;               /* This should be secPLUGIN_RECOGNITION */
    /* Module Descriptor */
    ULONG	            Version;	                /* = MUFS_PLUGIN_INTERFACE */
    APTR                    Initialize;
    APTR                    Terminate;
//    secInitPluginFunc       Initialize;
//    secTerminatePluginFunc  Terminate;
};

/* internal plugin records */
typedef struct
{
    struct MinNode          Node;
    ULONG                   reference_count;
    BPTR                    SegList;
    struct secPluginHeader * header;	                /* For locating the init/fini functions */
    UBYTE                   modulename[64];	        /* For displaying */
} secPluginModule;

#endif /* _SECURITY_INTERN_H */
