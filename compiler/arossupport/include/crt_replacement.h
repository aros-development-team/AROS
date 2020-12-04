#ifndef CRT_REPLACEMENT_H
#define CRT_REPLACEMENT_H

/*
 * Replacements for some CRT functions in order to
 * support the task of removing CRT from ROM modules
 */

static inline char *StrCpy(char *dest, const char *src)
{
    char *ptr = dest;

    while ((*ptr = *src))
    {
        ptr ++;
        src ++;
    }

    return dest;
}

#endif
