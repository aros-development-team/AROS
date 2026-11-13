#include <stdarg.h>

STRPTR strnew(APTR pool, STRPTR original);
STRPTR vfmtnew(APTR pool, STRPTR fmt, va_list args);
STRPTR fmtnew(APTR pool, STRPTR fmt, ...);
