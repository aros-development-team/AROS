#ifndef _ARGS_H
#define _ARGS_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Prototypes *************************************************************/
struct RDArgs * getArguments(void);
UBYTE processArguments(void);

/* Return values from processArguments() */
#define APP_RUN		0
#define APP_STOP	1
#define APP_FAIL	2


#endif /* _ARGS_H */
