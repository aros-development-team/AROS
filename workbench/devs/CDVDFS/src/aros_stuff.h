#ifdef __AROS__

#include <aros/macros.h>
#include <dos/bptr.h>

#ifdef AROS_KERNEL
#ifdef AROS_FAST_BSTR
#define USE_FAST_BSTR
#endif
#endif

#else

#define AROS_BE2WORD(x) x
#define AROS_BE2LONG(x) x

#define AROS_BSTR_ADDR(s)        (((STRPTR)BADDR(s))+1)
#define AROS_BSTR_strlen(s)      (AROS_BSTR_ADDR(s)[-1])
#define AROS_BSTR_getchar(s,l)   (AROS_BSTR_ADDR(s)[l])

#endif

#ifdef LATTICE
#define SAVEDS __saveds
#else
#define SAVEDS
#endif

