#if defined(__i386__)
#include "./i386/setjmp.S"
#elif defined(__x86_64__)
#include "./x86_64/setjmp.S"
#elif defined(__sparc__)
#include "./sparc64/setjmp.S"
#elif defined(__mips__)
#include "./mips/setjmp.S"
#elif defined(__powerpc__)
#include "./powerpc/setjmp.S"
#else
#error "Unknwon target cpu type"
#endif
