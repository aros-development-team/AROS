#ifndef AROS_AROSSUPPORTBASE_H
#define AROS_AROSSUPPORTBASE_H
/*
    (C) 1995-96 AROS - The Amiga Replacement OS
*/

struct AROSSupportBase
{
    void * StdOut;
    void (*kprintf)(char *, ...);
};

#endif /* AROS_AROSSUPPORTBASE_H */
