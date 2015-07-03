#ifndef STORAGE_INTERN_H
#define STORAGE_INTERN_H

#include <aros/libcall.h>
#include <exec/libraries.h>
#include <libcore/base.h>

#include LC_LIBDEFS_FILE

struct StorageBase_intern {
    struct LibHeader   lh;
};

/* Namespace structures */

struct Storage_IDNode
{
    struct Node                                 SIDN_Node;                      /* ln_Name = ID (e.g. "CD0") */
};

struct Storage_IDFamily
{
    struct Node                                 SIDF_Node;                      /* ln_Name = IDBase (e.g "CD") */
    struct List                                 SIDF_IDs;
};

#endif
