#ifndef _ARGS_H_
#define _ARGS_H_

/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

/*** Structures *************************************************************/
enum Argument
{
    FROM,
    USE,
    SAVE,
    PUBSCREEN,
    COUNT  /* Number of arguments */
};

/*** Prototypes *************************************************************/
BOOL ReadArguments(int argc, char **argv);
VOID FreeArguments(VOID);
IPTR GetArgument(enum Argument arg);

/*** Macros *****************************************************************/
#define ARG(a) GetArgument((a))

#endif /* _ARGS_H_ */
