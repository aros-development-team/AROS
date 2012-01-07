#ifndef _APPOBJECTS_H_
#define _APPOBJECTS_H_

/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <workbench/workbench.h>

/*** Prototypes *************************************************************/
BOOL SendAppIconMessage(struct AppIcon * appicon, LONG numargs, STRPTR args);

#endif /* _APPOBJECTS_H_ */
