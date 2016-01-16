
/*
**
**  $VER: mpegassert.h 1.11 (30.10.97)
**  mpegvideo.datatype 1.11
**
**  ANSI assert replacement (without abort())
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**
*/

#ifdef assert
#undef assert
#endif /* assert */

#if !defined(__AROS__)
#ifndef NDEBUG
#define assert(x)  ((void)((!(x))?((int)error_printf( mvid, "assert \"%s\" %s %ld\n", #x, __FILE__, __LINE__ )):(0)))
#else
#define assert(ignore)  ((void) 0)
#endif /* NDEBUG */
#else
#define assert(ignore)
#endif
