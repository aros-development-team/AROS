/* Copyright (c) 1992-1993 SAS Institute, Inc., Cary, NC USA */
/* All Rights Reserved */


#ifndef _STDARG_H
#define _STDARG_H 1

typedef char * va_list;

#define va_start(a,b) a=(char *)(&b+1)
#define va_arg(a,b)  (*((b *) ((a +=  ((sizeof(b)+sizeof(int)-1) & ~(sizeof(int)-1))   ) -  ((sizeof(b)+sizeof(int)-1) & ~(sizeof(int)-1)) )))
#define va_end(a)

#endif
