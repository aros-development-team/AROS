/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/execbase.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include "memory.h"
#include <stdlib.h>

#define NEWLIST(l)                          \
((l)->lh_Head=(struct Node *)&(l)->lh_Tail, \
 (l)->lh_Tail=NULL,                         \
 (l)->lh_TailPred=(struct Node *)(l))

extern void *ExecFunctions[];

#define MEMSIZE 1024*1024
static struct MemHeader mh;
static UBYTE memory[MEMSIZE+MEMCHUNK_TOTAL-1];

#define NUMVECT 131

struct ExecBase *SysBase;

#define STACKSIZE 4096

static void idle(void)
{
    /* If the idle task ever gets CPU time the emulation has locked up */
    exit(20);
}

int submain(int argc,char *argv[]);

static int gargc;
static char **gargv;

static void boot(void)
{
    /* return the returncode of the boot task */
    exit(submain(gargc,gargv));
}

static APTR allocmem(ULONG size)
{
    UBYTE *ret;

    size=(size+MEMCHUNK_TOTAL-1)&~(MEMCHUNK_TOTAL-1);
    ret=(UBYTE *)mh.mh_First;
    mh.mh_First=(struct MemChunk *)(ret+size);
    mh.mh_First->mc_Next=NULL;
    mh.mh_Free=mh.mh_First->mc_Bytes=((struct MemChunk *)ret)->mc_Bytes-size;
    return ret;
}

int main(int argc,char *argv[])
{
    /* Put arguments into globals */
    gargc=argc;
    gargv=argv;

    /*
	Prepare first MemHeader. I cannot use exec functions
	here because exec is not yet up.
    */
    mh.mh_Node.ln_Name="unknown memory type";
    mh.mh_Node.ln_Pri =0;
    mh.mh_Attributes  =MEMF_PUBLIC; /* Public to my emulation */
    mh.mh_First=(struct MemChunk *)
		(((ULONG)memory+MEMCHUNK_TOTAL-1)&~(MEMCHUNK_TOTAL-1));
    mh.mh_First->mc_Next=NULL;
    mh.mh_First->mc_Bytes=MEMSIZE;
    mh.mh_Lower=mh.mh_First;
    mh.mh_Upper=(UBYTE *)mh.mh_Lower+MEMSIZE;
    mh.mh_Free =MEMSIZE;

    /* The following allocations cannot and must not fail. */
    {
	/* Prepare exec.library */
	ULONG neg,i;
	neg=sizeof(struct JumpVec)*NUMVECT;
	neg=(neg+LIBALIGN-1)&~(LIBALIGN-1);
	SysBase=(struct ExecBase *)
		((UBYTE *)allocmem(neg+sizeof(struct ExecBase))+neg);
	for(i=0;i<NUMVECT;i++)
	{
	    SET_JMP(&((struct JumpVec *)SysBase)[-i-1]);
	    SET_VEC(&((struct JumpVec *)SysBase)[-i-1],ExecFunctions[i]);
	}
	/* Build GetCC vector (68000 version) */
	((UWORD *)((UBYTE *)SysBase-88*LIB_VECTSIZE))[0]=0x40c0; /* movew sr,d0 */
	((UWORD *)((UBYTE *)SysBase-88*LIB_VECTSIZE))[1]=0x4e75; /* rts         */

	SysBase->AttnFlags=sb->AttnFlags; /* No CPU check yet */

#ifdef mc68000
	/* CPU OK ? */
	if(SysBase->AttnFlags&AFB_68010)
	    fprintf(stderr,"Warning: wrong CPU version\n");
#endif

	SysBase->LibNode.lib_Node.ln_Name="exec.library";

	NEWLIST(&SysBase->MemList);
	AddHead(&SysBase->MemList,&mh.mh_Node);
	NEWLIST(&SysBase->ResourceList);
	NEWLIST(&SysBase->DeviceList);
	NEWLIST(&SysBase->IntrList);
	NEWLIST(&SysBase->LibList);
	NEWLIST(&SysBase->PortList);
	NEWLIST(&SysBase->TaskReady);
	NEWLIST(&SysBase->TaskWait);

	for(i=0;i<5;i++)
	{
	    NEWLIST(&SysBase->SoftInts[i].sh_List);
	}

	NEWLIST(&SysBase->SemaphoreList);

	/* There are no memhandlers yet.
	 * (not even the library flushing one which is part of ram/dos.library) */
	NEWLIST((struct List *)&SysBase->ex_MemHandlers);
	SysBase->IDNestCnt=0;
	SysBase->TDNestCnt=0;
	SysBase->AttnResched=0;
    }
    {
	/* Add boot task */
	struct Task *t;
	struct MemList *ml;

	ml=(struct MemList *)AllocMem(sizeof(struct MemList),MEMF_PUBLIC|MEMF_CLEAR);
	t =(struct Task *)   AllocMem(sizeof(struct Task),   MEMF_PUBLIC|MEMF_CLEAR);
	ml->ml_NumEntries     =1;
	ml->ml_ME[0].me_Addr  =t;
	ml->ml_ME[0].me_Length=sizeof(struct Task);

	NEWLIST(&t->tc_MemEntry);
	AddHead(&t->tc_MemEntry,&ml->ml_Node);
	t->tc_Node.ln_Name="Boot task";
	t->tc_Node.ln_Pri=0;
	t->tc_State=TS_RUN;
	t->tc_SigAlloc=0xffff;
	SysBase->ThisTask=t;
    }
    {
	/* Add idle task */
	struct Task *t;
	struct MemList *ml;
	UBYTE *s;

	ml=(struct MemList *)AllocMem(sizeof(struct MemList)+sizeof(struct MemEntry),
				      MEMF_PUBLIC|MEMF_CLEAR);
	t =(struct Task *)   AllocMem(sizeof(struct Task),    MEMF_PUBLIC|MEMF_CLEAR);
	s =(UBYTE *)         AllocMem(STACKSIZE,              MEMF_PUBLIC|MEMF_CLEAR);
	ml->ml_NumEntries     =2;
	ml->ml_ME[0].me_Addr  =t;
	ml->ml_ME[0].me_Length=sizeof(struct Task);
	ml->ml_ME[1].me_Addr  =s;
	ml->ml_ME[1].me_Length=STACKSIZE;

	NEWLIST(&t->tc_MemEntry);
	AddHead(&t->tc_MemEntry,&ml->ml_Node);
	t->tc_SPLower=s;
	t->tc_SPUpper=s+STACKSIZE;
	t->tc_Node.ln_Name="Idle task";
	t->tc_Node.ln_Pri=-128;
#if STACK_GROWS_DOWNWARDS
	t->tc_SPReg=(UBYTE *)t->tc_SPUpper-SP_OFFSET;
#else
	t->tc_SPReg=(UBYTE *)t->tc_SPLower-SP_OFFSET;
#endif
	AddTask(t,&idle,NULL);
    }
    Enable();
    Permit();
    boot();

    /* Get compiler happy */
    return 0;
}
