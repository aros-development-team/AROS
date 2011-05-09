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
    struct Library *pb_DOSBase;

    /* REMOVE ONCE ABIv1 HAS STABALIZED */
    struct Library *pb_UtilityBase;
};

#define PTYPE(x) ((struct PartitionType *)x)

/* We do NOT define DOSBase, because we want to be
 * explicit about all uses of DOSBase, since it may
 * be NULL.
 */

/* REMOVE ONCE ABIv1 HAS STABALIZED */
#define UtilityBase (((struct PartitionBase_intern *)PartitionBase)->pb_UtilityBase)

#endif /* PARTITION_INTERN_H */
