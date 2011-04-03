#ifndef DEBUG_DEBUG_H
#define DEBUG_DEBUG_H 1

/*************************************************************************/

#include <stdarg.h>
#include <exec/types.h>
#include <exec/initializers.h>

#include <proto/exec.h>

#include <SDI/SDI_compiler.h>

/*************************************************************************/

#if defined(DEBUG) && !defined(NODEBUG)

extern ULONG debug_disable;

/* This is our debug function */
void STDARGS debug( char *text, ...);

#else
    #ifdef __VBCC__
        #define debug(...) do{}while(0)
    #endif
    #ifdef __GNUC__
		#define debug(...)  {}
    #endif
#endif

/*************************************************************************/

#endif /* DEBUG_DEBUG_H */

