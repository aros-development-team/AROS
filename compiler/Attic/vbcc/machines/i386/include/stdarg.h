#ifndef __STDARG_H
#define __STDARG_H 1

typedef unsigned char *va_list;

#define va_start(ap, lastarg) ((ap) = (va_list)(&lastarg + 1))
#define va_arg(ap, type) ((ap) += (sizeof(type)<sizeof(int)?sizeof(int):sizeof(type)), ((type *)(ap))[-1])
#define va_end(ap) ((ap) = 0L)

#endif

