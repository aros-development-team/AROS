#ifdef __AROS__

#define SQRT sqrt

/* There's no more difference in FIB handling between IOFS and packet versions
#ifdef AROS_KERNEL */
#ifdef AROS_FAST_BSTR
#define USE_FAST_BSTR
#endif
/*#endif */

#endif

#ifdef __GNUC__
#define __saveds
#endif
