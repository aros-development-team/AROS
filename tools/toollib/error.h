#ifndef TOOLLIB_ERROR_H
#define TOOLLIB_ERROR_H

#ifndef TOOLLIB_TOOLLIB_H
#   include <toollib/toollib.h>
#endif

extern void Error	    PARAMS ((const char * fmt, ...));
extern void Warn	    PARAMS ((const char * fmt, ...));
extern void StdError	    PARAMS ((const char * fmt, ...));
extern void StdWarn	    PARAMS ((const char * fmt, ...));
extern void PrintErrorStack PARAMS ((void));
extern void PushMsg	    PARAMS ((const char * pre, const char * fmt,
				va_list args, const char * post));
extern void PushError	    PARAMS ((const char * fmt, ...));
extern void PushWarn	    PARAMS ((const char * fmt, ...));
extern void PushStdError    PARAMS ((const char * fmt, ...));
extern void PushStdWarn     PARAMS ((const char * fmt, ...));
extern void ErrorExit	    PARAMS ((int ec));

#endif /* TOOLLIB_ERROR_H */

