#ifndef AROS_AROSSUPPORTBASE_H
#define AROS_AROSSUPPORTBASE_H
/*
    (C) 1995-96 AROS - The Amiga Research OS
*/

struct AROSSupportBase
{
    void * StdOut;
    int (*kprintf)(const char *, ...);
    int (*rkprintf)(const char *, const char *, int, const char *, ...);
    void * DebugConfig;
};

#endif /* AROS_AROSSUPPORTBASE_H */
