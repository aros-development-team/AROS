#ifndef AROS_AROSBASE_H
#define AROS_AROSBASE_H
/*
    (C) 1995-96 AROS - The Amiga Replacement OS
*/

struct AROSBase
{
    void * StdOut;
    void (*kprintf)(char *, ...);
};

#endif /* AROS_AROSBASE_H */
