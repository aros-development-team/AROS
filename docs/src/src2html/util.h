#include <time.h>

extern void * xmalloc (int);
extern void xfree (void *);
extern char * xstrdup (const char *);

extern char * expandpath (const char *);
extern char * substvar (const char *);
extern char * xbasename (const char *, const char *);
extern char * buildname (const char *, ...);
extern time_t getfiledate (const char *);
