#ifndef __STDDEF_H
#define __STDDEF_H 1

#ifndef __SIZE_T
#define __SIZE_T 1
typedef unsigned long size_t;
#endif

#ifndef __FPOS_T
#define __FPOS_T 1
typedef long fpos_t;
#endif

#ifndef __PTRDIFF_T
#define __PTRDIFF_T 1
typedef long ptrdiff_t;
#endif

#ifndef __WCHAR_T
#define __WCHAR_T
typedef char wchar_t;
#endif
#ifndef __TIME_T
#define __TIME_T 1
typedef long time_t;
#endif

#ifndef __CLOCK_T
#define __CLOCK_T 1
typedef long clock_t;
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef offsetof
#define offsetof(P,M) ((size_t)&((P *)NULL)->M)
#endif

#endif


