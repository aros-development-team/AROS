#ifndef PARTITION_INTERN_H
#define PARTITION_INTERN_H

/*
    Copyright © 2001-2011, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Internal definitions for partition.library
   Lang: english
*/

#include <exec/libraries.h>
#include <libraries/partition.h>

#include <aros/libcall.h>
#include LC_LIBDEFS_FILE

struct PartitionBase_intern
{
    struct PartitionBase partbase;
    BPTR segList;
    struct List bootList;
    struct Library *dosBase;
};

#define PTYPE(x) ((struct PartitionType *)x)

#endif /* PARTITION_INTERN_H */
