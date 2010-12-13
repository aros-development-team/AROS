#ifndef ACDR_DEBUG_H
#define ACDR_DEBUG_H

#if defined(__AROS__) || defined(__MORPHOS__)
#ifdef __AROS__
#	include <aros/debug.h>
#else
#       include <proto/sysdebug.h>
#endif
#	if DEBUG>0
#		define BUG(x) x
#               define dbinit() kprintf("Debugger running:" HANDLER_VERSION "GNU C" __VERSION__ "," __TIME__ "\n")
#               define dbuninit()
#		define dbprintf kprintf
#	else
#		define BUG(x)
#	endif
#	define BUG2(x)
#else
#	ifdef NDEBUG
#		define BUG(x) /* nothing */
#	else
#		define BUG(x) x
#	endif
#	if !defined(NDEBUG) || defined(DEBUG_SECTORS)
#		define BUG2(x) x
#	else
#		define BUG2(x) /* nothing */
#	endif
#endif

#ifndef D
#define D BUG
#endif
#ifndef bug
#define bug dbprintf
#endif

#endif /* ACDR_DEBUG_H */
