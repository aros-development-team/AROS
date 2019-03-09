#ifndef STORAGE_INTERN_H
#define STORAGE_INTERN_H

/*
    Copyright © 1995-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif
#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif
#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif

#include LC_LIBDEFS_FILE

struct StorageBase_intern {
    struct Library    lh;
    struct List         sb_IDs;
    struct List         sb_Devices;
};

/* ID Namespace structures */

struct Storage_IDFamily
{
    struct Node                                 SIDF_Node;                      /* ln_Name = IDBase (e.g "CD") */
    struct List                                 SIDF_IDs;
};

struct Storage_IDNode
{
    struct Node                                 SIDN_Node;                      /* ln_Name = ID (e.g. "CD0") */
};


#endif
