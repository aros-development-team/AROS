/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:40:54  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include <clib/dos_protos.h>

#define SHT_PROGBITS	1
#define SHT_SYMTAB	2
#define SHT_NOBITS	8
#define SHT_REL 	9

#define RELO_32 	1
#define RELO_EXEC	2

struct elfheader
{
    UBYTE ident[16];
    UWORD type;
    UWORD machine;
    ULONG version;
    APTR entry;
    ULONG phoff;
    ULONG shoff;
    ULONG flags;
    UWORD ehsize;
    UWORD phentsize;
    UWORD phnum;
    UWORD shentsize;
    UWORD shnum;
    UWORD shstrndx;
};

struct sheader
{
    ULONG name;
    ULONG type;
    ULONG flags;
    APTR addr;
    ULONG offset;
    ULONG size;
    ULONG link;
    ULONG info;
    ULONG addralign;
    ULONG entsize;
};

struct symbol
{
    ULONG name;
    ULONG value;
    ULONG size;
    UBYTE info;
    UBYTE other;
    WORD shindex;
};

struct relo
{
    ULONG addr;
    ULONG info;
};

struct hunk
{
    ULONG size;
    UBYTE *memory;
};

int read_block(BPTR file, ULONG offset, APTR buffer, ULONG size)
{
    LONG subsize;
    UBYTE *buf=(UBYTE *)buffer;
    if(Seek(file,offset,OFFSET_BEGINNING)<0)
	return 1;
    while(size)
    {
	subsize=Read(file,buf,size);
	if(subsize==0)
	{
	    ((struct Process *)FindTask(NULL))->pr_Result2=ERROR_BAD_HUNK;
	    return 1;
	}
	if(subsize<0)
	    return 1;
	buf+=subsize;
	size-=subsize;
    }
    return 0;
}

BPTR LoadSeg_ELF(BPTR file)
{
    UBYTE *shtab=NULL;
    struct symbol *symtab=NULL;
    struct hunk *hunks=NULL;
    struct relo *reltab=NULL;

    ULONG numsym, numrel, i;
    WORD t, mint=0, maxt=0;
    struct elfheader eh;
    struct sheader *sh;
    UBYTE *loaded;
    struct symbol *symbol;
    BPTR last=0;

#define ERROR(a)    { *error=a; goto end; }
    LONG *error=&((struct Process *)FindTask(NULL))->pr_Result2;

    if(read_block(file,0,&eh,sizeof(eh)))
	goto end;
    if(eh.ident[0]!=0x7f||eh.ident[1]!='E'||eh.ident[2]!='L'||eh.ident[3]!='F')
	ERROR(ERROR_NOT_EXECUTABLE);
    if(eh.type!=1||eh.machine!=3)
	ERROR(ERROR_OBJECT_WRONG_TYPE);
    shtab=(UBYTE *)AllocVec(eh.shentsize*eh.shnum,MEMF_ANY);
    if(shtab==NULL)
	ERROR(ERROR_NO_FREE_STORE);
    if(read_block(file,eh.shoff,shtab,eh.shentsize*eh.shnum))
	goto end;
    for(t=0;;t++)
    {
	if(t==eh.shnum)
	    ERROR(ERROR_OBJECT_WRONG_TYPE);
	sh=(struct sheader *)(shtab+t*eh.shentsize);
	if(sh->type==SHT_SYMTAB)
	    break;
    }
    symtab=(struct symbol *)AllocVec(sh->size,MEMF_ANY);
    if(symtab==NULL)
	ERROR(ERROR_NO_FREE_STORE);
    if(read_block(file,sh->offset,symtab,sh->size))
	goto end;
    numsym=sh->size/sizeof(struct symbol);
    mint=maxt=symtab[0].shindex;
    for(i=1;i<numsym;i++)
    {
	if(symtab[i].shindex<mint)
	    mint=symtab[i].shindex;
	if(symtab[i].shindex>maxt)
	    maxt=symtab[i].shindex;
    }
    hunks=(struct hunk *)AllocVec(sizeof(struct hunk)*((LONG)maxt-mint+1),MEMF_CLEAR);
    if(hunks==NULL)
	ERROR(ERROR_NO_FREE_STORE);
    hunks-=mint;
    for(t=0;t<eh.shnum;t++)
    {
	sh=(struct sheader *)(shtab+t*eh.shentsize);
	if(sh->type==SHT_PROGBITS||sh->type==SHT_NOBITS)
	    hunks[t].size=sh->size;
    }
    for(i=0;i<numsym;i++)
	if(symtab[i].shindex<0)
	{
	    symtab[i].value=hunks[symtab[i].shindex].size;
	    hunks[symtab[i].shindex].size+=symtab[i].size;
	}
    for(t=mint;t<=maxt;t++)
	if(hunks[t].size)
	{
	    hunks[t].memory=(UBYTE *)AllocVec(hunks[t].size+sizeof(BPTR),MEMF_CLEAR);
	    if(hunks[t].memory==NULL)
		ERROR(ERROR_NO_FREE_STORE);
	    hunks[t].memory+=sizeof(BPTR);
	}
    loaded=NULL;
    for(t=0;t<eh.shnum;t++)
    {
	sh=(struct sheader *)(shtab+t*eh.shentsize);
	switch(sh->type)
	{
	    case SHT_PROGBITS:
		if(read_block(file,sh->offset,hunks[t].memory,sh->size))
		    goto end;
		loaded=hunks[t].memory;
		break;
	    case SHT_REL:
		if(loaded==NULL)
		    ERROR(ERROR_OBJECT_WRONG_TYPE);
		reltab=AllocVec(sh->size,MEMF_ANY);
		if(reltab==NULL)
		    ERROR(ERROR_NO_FREE_STORE);
		if(read_block(file,sh->offset,reltab,sh->size))
		    goto end;
		numrel=sh->size/sizeof(struct relo);
		for(i=0;i<numrel;i++)
		{
		    symbol=&symtab[reltab[i].info>>8];
		    switch(reltab[i].info&0xff)
		    {
			case RELO_32:
			    *(ULONG *)&loaded[reltab[i].addr]+=
				(ULONG)hunks[symbol->shindex].memory+symbol->value;
			    break;
			case RELO_EXEC: break;
			default:
			    ERROR(ERROR_BAD_HUNK);
		    }
		}
		FreeVec(reltab);
		reltab=NULL;
		loaded=NULL;
		break;
	}
    }
    for(t=mint;t<0;t++)
	if(hunks[t].size)
	{
	    ((BPTR *)hunks[t].memory)[-1]=last;
	    last=MKBADDR((BPTR *)hunks[t].memory-1);
	}
    for(t=maxt;t>=0;t--)
	if(hunks[t].size)
	{
	    ((BPTR *)hunks[t].memory)[-1]=last;
	    last=MKBADDR((BPTR *)hunks[t].memory-1);
	}
    FreeVec(hunks+mint);
    hunks=NULL;

end:
    FreeVec(reltab);
    if(hunks!=NULL)
    {
	for(t=mint;t<=maxt;t++)
	    if(hunks[t].memory!=NULL)
		FreeVec(hunks[t].memory-sizeof(BPTR));
	FreeVec(hunks+mint);
    }
    FreeVec(symtab);
    FreeVec(shtab);
    return last;
}
