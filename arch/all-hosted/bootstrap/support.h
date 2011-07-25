#include <aros/config.h>

extern char *DefaultConfig;

char *getosversion(void);
char *join_string(int argc, char **argv);
char *namepart(char *name);

#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)

void kprintf(const char *, ...);

#else

#define kprintf(...) fprintf(stderr, __VA_ARGS__)

#endif
