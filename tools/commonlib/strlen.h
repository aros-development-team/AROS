#ifndef COMMONLIB_STRLEN_H
#define COMMONLIB_STRLEN_H

#include <commonlib/commonlib.h>

#if defined(__GNUC__) && !defined(CommonLib_NoInline)
#   define strlen(str)                              \
	({                                          \
	    const char * _str = (str);              \
	    const char * ptr = _str;		    \
						    \
	    while (*ptr) ptr ++;                    \
						    \
	    (size_t)((IPTR)ptr) - ((IPTR)(_str));   \
	})

#else
    size_t strlen (CONST char * str)
    {
	const char * ptr = str;

	while (*ptr) ptr ++;

	return (size_t)((IPTR)ptr) - ((IPTR)str);
    }
#endif

#endif /* COMMONLIB_STRLEN_H */

