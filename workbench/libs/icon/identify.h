#ifndef _IDENTIFY_H_
#define _IDENTIFY_H_

/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <workbench/workbench.h>
#include <workbench/icon.h>

/*** Prototypes *************************************************************/
struct DiskObject *__FindDefaultIcon_WB(struct IconIdentifyMsg *iim, struct Library *IconBase);

/*** Macros *****************************************************************/
#define FindDefaultIcon(iim) (__FindDefaultIcon_WB((iim), IconBase))

#endif /* _IDENTIFY_H_ */
