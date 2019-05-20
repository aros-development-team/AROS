#ifndef _SECURITY_MEMORY_H
#define _SECURITY_MEMORY_H
/************************************************************
* MultiUser - MultiUser Task/File Support System				*
* ---------------------------------------------------------	*
* Memory management														*
* ---------------------------------------------------------	*
* © Copyright 1993-1994 Geert Uytterhoeven						*
* All Rights Reserved.													*
************************************************************/

/* This structure might change/melt/whatever whenever */

struct MemNode {
	struct MinNode TaskMemNode;
	struct MinNode Node;
	APTR Address;
	ULONG Size;
	struct secOldTaskNode *Owner;
};

/*
 *      Private Function Prototypes
 */

extern BOOL InitMemory(void);
extern void CleanUpMemory(void);
extern APTR MAlloc(ULONG size);
extern void Free(APTR block, ULONG size);
extern APTR MAllocV(ULONG size);
extern void FreeV(APTR block);

extern void InitMemList(void);

#endif /* _SECURITY_MEMORY_H */
