/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef __DEBUG_H
#define __DEBUG_H

/* Debug Macros */

#ifdef __AROS__

#include <aros/debug.h>

#else /* not __AROS__ */

#define bug kprintf

#ifdef MYDEBUG
void kprintf(char *string, ...);
#define D(x) {kprintf("%s/%ld (%s): ", __FILE__, __LINE__, FindTask(NULL)->tc_Node.ln_Name);(x);};
#else
#define D(x) ;

#endif /* MYDEBUG */

#endif /* not __AROS__ */

#endif /* __DEBUG_H */
