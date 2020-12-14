#ifndef SETMEM_PC_H
#define SETMEM_PC_H

/* support function for small copies ..*/
static VOID __smallsetmem(APTR destination, UBYTE c, ULONG length)
{
    int i;

    register char *p = (char *) destination;

    for (i = 0; i < length; i++) {
        p[i] = c;
    }
}

#endif
