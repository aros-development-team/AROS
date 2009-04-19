#ifndef _ARGS_H_
#define _ARGS_H_

/*
    Copyright � 2003, The AROS Development Team. All rights reserved.
    $Id: args.h 21130 2004-02-28 22:50:12Z chodorowski $
 */

#include <exec/types.h>

/*** Structures *************************************************************/
enum Argument {
	FROM,
	USE,
	SAVE,
	COUNT /* Number of arguments */
};

/*** Prototypes *************************************************************/
BOOL ReadArguments(VOID);
VOID FreeArguments(VOID);
IPTR GetArgument(enum Argument arg);

/*** Macros *****************************************************************/
#define ARG(a) GetArgument((a))

#endif /* _ARGS_H_ */
