#ifdef SASC
#define STDARGS __stdargs
#define CONSTRUCTOR
#define DESTRUCTOR
#define INTERRUPT __interrupt
#endif
#ifdef __GNUC__
#define STDARGS
#define CONSTRUCTOR __attribute__ ((constructor))
#define DESTRUCTOR __attribute__ ((destructor))
#define INTERRUPT
#endif

#ifndef __AROS__
#define IPTR ULONG
#endif
