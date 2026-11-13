#ifndef __DEBUG_H__
#define __DEBUG_H__

/* debug.h - by Harry "Piru" Sintonen
*/

#define DB_LEVEL 20

//#define DEBUG

#ifdef DEBUG
   #define BreakPoint __emit(0x4afc)
   void dprintf(const char *, ...);
#  ifdef __GNUC__
#    define KPRINTF(l, x) do { if ((l) >= DB_LEVEL) \
     { dprintf("%s:%s/%lu: ", __FILE__, __FUNCTION__, __LINE__); dprintf x;} } while (0)
#  else
#    define KPRINTF(l, x) do { if ((l) >= DB_LEVEL) \
     { dprintf("%s/%lu: ", __FILE__, __LINE__); dprintf x;} } while (0)
#  endif /* __GNUC__ */
#  define DB(x) x
   void dumpmem(void *mem, unsigned long int len);
#else /* !DEBUG */

#define KPRINTF(l, x) do { } while(0)
#define DB(x)
#define BreakPoint

#endif /* DEBUG */

#endif /* __DEBUG_H__ */
