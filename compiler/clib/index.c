#include <string.h>

char *index(const char *s, int c)
{
     while (*s != (char)c && *s != '\0') s++;

     if (*s == (char)c) return s;

     return NULL;
}
