#ifndef PARTITION_INTERN_H
#define PARTITION_INTERN_H

/*
   (C) 2001 AROS - The Amiga Research OS
   $Id$

   Desc: Internal definitions for partition.library
   Lang: english
*/
#ifdef __AMIGAOS__
#define SysBase (((struct LibHeader *)PartitionBase)->lh_SysBase)
#else
#ifndef EXEC_TYPES_H
#  include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#  include <exec/libraries.h>
#endif
#ifndef PARTITION_H
#   include <libraries/partition.h>
#endif

#include <aros/libcall.h>
#include LC_LIBDEFS_FILE

/* Predeclaration */
LIBBASETYPE;

#define SysBase GM_SYSBASE_FIELD(PartitionBase)

LIBBASETYPE
{
    struct PartitionBase partbase;
    struct ExecBase *sysbase;
    APTR seglist;
};

#endif
#endif /* PARTITION_INTERN_H */

