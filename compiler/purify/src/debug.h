#ifndef _DEBUG_H
#define _DEBUG_H

#ifndef DEBUG
#   define DEBUG 0
#endif

#ifndef _PNDEBUG
#   define passert(expr)   if(!(expr) ) Purify_AssertFailed (__FILE__,__LINE__,__FUNCTION__,#expr)
#else
#   define passert(expr)   /* eps */
#endif

void Purify_AssertFailed (const char * file, int line, const char * fname,
			const char * expr);

#endif /* _DEBUG_H */
