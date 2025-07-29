#ifndef __EXEC_DEBUG_INTERNAL_H__
#define __EXEC_DEBUG_INTERNAL_H__

void    InternalDebug(void *);
char    GetK(struct ExecBase *SysBase);
UQUAD   GetQ(char *);
ULONG   GetL(char *);
UWORD   GetW(char *);
UBYTE   GetB(char *);
int     get_irq_list(char *buf);
char   *NextWord(char *);

#if __WORDSIZE == 64
#define GetA (APTR)GetQ
#else
#define GetA (APTR)GetL
#endif

#endif
