/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef __DEBUG_H
#define __DEBUG_H

/* Debug Macros */

#ifdef __AROS__

#undef DEBUG

#ifdef MYDEBUG
#define DEBUG 1
#else
#define DEBUG 0
#endif
#include <aros/debug.h>

#else /* ! __AROS__ */

#ifdef __amigaos4__
#   define bug DebugPrintF
#else
#   define bug kprintf
#endif

#define ASSERT(x)
#define ASSERT_VALID_PTR(x)

#ifdef MYDEBUG

#ifdef __AMIGAOS4__
#    undef SysBase
#    include <proto/exec.h>
#    define D(x) do {Forbid();DebugPrintF("%s/%ld Task \"%s\" [%s()] => ", __FILE__, __LINE__, FindTask(NULL)->tc_Node.ln_Name,__PRETTY_FUNCTION__);(x);Permit();} while(0);
#else
void kprintf(char *string, ...);
#    define D(x) {kprintf("%s/%ld (%s): ", __FILE__, __LINE__, FindTask(NULL)->tc_Node.ln_Name);(x);};
#endif

#else
#define D(x) ;

#endif /* MYDEBUG */

#endif /* ! __AROS__ */

#endif /* __DEBUG_H */
