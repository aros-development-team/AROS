#ifdef __AROS__

#define SQRT sqrt

#ifdef AROS_KERNEL
#ifdef AROS_FAST_BSTR
#define USE_FAST_BSTR
#endif
#endif

#endif

#ifdef __GNUC__
#define __saveds
#endif
