#ifndef UTILITY_NAME_H
#define UTILITY_NAME_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Namespace structures
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

struct NamedObject
{
    APTR no_Object;
};

/* AllocNamedObject() Tags */
#define ANO_NameSpace 4000
#define ANO_UserSpace 4001
#define ANO_Priority  4002
#define ANO_Flags     4003 /* see below */

/* ANO_Flags */
#define NSB_NODUPS      0
#define NSF_NODUPS (1L<<0)
#define NSB_CASE        1
#define NSF_CASE   (1L<<1)

#endif /* UTILITY_NAME_H */
