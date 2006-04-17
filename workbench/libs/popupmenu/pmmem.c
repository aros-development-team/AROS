//
// pmmem.h
//

#include "pmpriv.h"
#include "pmmem.h"

APTR PM_AllocVecPooled(LONG size)
{
#if defined(__AROS__) || defined(__MORPHOS__)
	ULONG *p;

	// To Do: Add Semaphore protection to the pool!!!
    	// stegerg: MOS and AROS have MEMF_SEM_PROTECTED. See pminit.c
	
	size += sizeof(ULONG);
	
	p = AllocPooled(MemPool, size);
	if(p) {
		*p++ = size;
		return (APTR)p;
	}
	return NULL;
#else	
	return AllocVec(size, MEMF_CLEAR);
#endif	
}

void PM_FreeVecPooled(APTR mem)
{
#if defined(__AROS__) || defined(__MORPHOS__)
	ULONG *p = (ULONG *)mem;
	p--;
	FreePooled(MemPool, p, *p);
#else	
	FreeVec(mem);
#endif
}

ULONG PM_String_Length(STRPTR s)
{
	ULONG r=(ULONG)s;
	
	while(*s++);
	
	return ((ULONG)s)-r;
}

STRPTR PM_String_Copy(STRPTR Source, STRPTR Dest, LONG Len)
{
	if(Len==-1) {
		while(*Source) *Dest++=*Source++;
		*Dest++=0;
		return Dest;
	} else {
		LONG ctr=0;
		
		while(ctr<Len) {
			Dest[ctr]=Source[ctr];
			ctr++;
		}
		return &Dest[ctr];
	}
}

ULONG PM_String_Compare(STRPTR str1, STRPTR str2)
{
	ULONG i, j=0;
	if(!str1 || !str2) return 0;
	for(i=0;;i++) {
		j+=str1[i];
		j-=str2[i];
		if(!str1[i] || !str2[i] || j) return j;
	}
}

void PM_StrCat(STRPTR str1, STRPTR str2)
{
	while(*str1) str1++;
	while(*str2) *str1++=*str2++;
	*str1=0;
}
