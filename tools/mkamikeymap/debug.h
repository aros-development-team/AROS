
#define D(x)
#define DLINE(x)

#if defined(__AROS__)
#include <proto/dos.h>
#define CONSOUT(fmt, ...) Printf(fmt, ##__VA_ARGS__)
#define CONSERR(fmt, ...) Printf(fmt, ##__VA_ARGS__)
#else
#define CONSOUT(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#define CONSERR(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#endif
