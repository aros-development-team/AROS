#ifndef _HASH
#define _HASH

#define PURIFY_MemType_Heap	    0	/* malloc()'d data */
#define PURIFY_MemType_Stack	    1	/* data on stack */
#define PURIFY_MemType_Code	    2	/* code memory */
#define PURIFY_MemType_Data	    3	/* data segment */

#define PURIFY_MemFlag_Readable     1	/* Flag: May read */
#define PURIFY_MemFlag_Writable     2	/* Flag: May write */
#define PURIFY_MemFlag_Free	    4	/* Flag: Free memory */
#define PURIFY_MemFlag_Empty	    8	/* Flag: Not yet written to */

#define PURIFY_MemAccess_Read	    0
#define PURIFY_MemAccess_Write	    1

typedef struct _MemHash MemHash;

struct _MemHash
{
    MemHash    * next;
    void       * mem;	    /* This memory is purified */
    char       * flags;     /* These are the flags for the memory */
    int 	 size;	    /* The size of the memory */
    int 	 type;	    /* Type of this memory */
    const void * data;	    /* Userdata. If type == normal, then
				points to PMemoryNode.
				If type == stack, points to name of
				variable.
				If type == code, points to filename.
				If type == data, points to file- or
				variablename.
			    */
};

typedef MemHash * MemHashTable[256];

extern MemHash * Purify_LastNode;

/* Prototypes */
MemHash * Purify_AddMemory (void * mem, int size, int flag, int type);
void Purify_RemMemory (const void * mem);
void Purify_SetMemoryFlags (MemHash * mem, int offset, int size, int flag);
void Purify_ModifyMemoryFlags (MemHash * mem, int offset, int size, int flag,
    int mask);
MemHash * Purify_FindMemory (const void * mem);
MemHash * Purify_FindNextMemory (const void * mem, int * offset);
int Purify_CheckMemoryAccess (const void * mem, int size, int access);
void Purify_PrintMemory (void);

#endif /* _HASH */
