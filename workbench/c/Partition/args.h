#ifndef _ARGS_H_
#define _ARGS_H_

/*
    Copyright � 2004-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

/*** Structures *************************************************************/
enum Argument
{
    DEVICE,
    UNIT,
    SYSSIZE,  /* Size of System partition in MBs */
    SYSTYPE,  /* Type of filesystem on System partition [FFSIntl, SFS] */
    SYSNAME,  /* Name of System partition */
    WORKSIZE, /* Size of Work partition in MBs */
    MAXWORK,  /* Create maximum-sized Work partition */
    WORKTYPE, /* Type of filesystem on Work partition [FFSIntl, SFS] */
    WORKNAME, /* Name of Work partition */
    WIPE,     /* Destroy all existing partitions */
    FORCE,    /* Don't ask the user for permission */
    QUIET,    /* Don't print anything */
    RDB,      /* Create only RDB partitions */
    COUNT     /* Number of arguments */
};

/*** Prototypes *************************************************************/
BOOL ReadArguments(VOID);
VOID FreeArguments(VOID);
IPTR GetArgument(enum Argument arg);

/*** Macros *****************************************************************/
#define ARG(a) GetArgument((a))

#endif /* _ARGS_H_ */
