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
#include <libraries/security.h>

#include LC_LIBDEFS_FILE
#define  DEBUG_NAME_STR                     "[" NAME_STRING "]"

#endif /* _SECURITY_INTERN_H */
