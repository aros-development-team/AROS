#ifndef _SUPPORT_H_
#define _SUPPORT_H_

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#ifdef __AROS__
#include <dos/bptr.h>
#endif

/*** Prototypes *************************************************************/
BOOL ReadLine(BPTR fh, STRPTR buffer, ULONG size);

#endif /* _SUPPORT_H_ */
