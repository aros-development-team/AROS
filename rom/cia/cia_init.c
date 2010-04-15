/*
 * This is a skeleton of cia.resource implementation.
 *
 * Note that it uses custom initcode because cia.resource does not
 * act in standard way. This module in fact contains two resources:
 * ciaa.resource and ciab.resource.
 * genmodule does not support such thing, so everything is written
 * by hands here.
 * 
 * Since we don't use genmodule, we have to use some other way for
 * generating include files. A standard SFD file is used to keep all
 * the definitions, it's processed using sfdc (note that it had to
 * be fixed in order to support explicit base specification correctly)
 *
 * Note that fd2inline (which also supports generating AROS macros) will
 * generate a BROKEN inline file for this resource. It handles missing
 * base specification incorrectly.
 * 
 * sonic <pavel_fedin@mail.ru>
 */

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/alib.h>

#include "cia_intern.h"

#define _STR(A) #A
#define STR(A) _STR(A)

AROS_LD2(struct Interrupt *, AddICRVector,
         AROS_LDA(WORD, iCRBit, D0),
         AROS_LDA(struct Interrupt *, interrupt, A1),
         struct Library *, resource, 1, Cia
);
AROS_LD2(void, RemICRVector,
         AROS_LDA(WORD, iCRBit, A0),
         AROS_LDA(struct Interrupt *, interrupt, A1),
         struct Library *, resource, 2, Cia
);
AROS_LD1(WORD, AbleICR,
         AROS_LDA(WORD, mask, D0),
         struct Library *, resource, 3, Cia
);
AROS_LD1(WORD, SetICR,
         AROS_LDA(WORD, mask, D0),
         struct Library *, resource, 4, Cia
);

#define RESIDENT_FLAGS    RTF_COLDSTART
#define RESIDENT_PRIORITY 20		 /* I set this value just for sample, i don't know the correct one */
#define RESIDENT_NAME     "cia.resource"
#define RESIDENT_VERSION  0
#define RESIDENT_REVISION 1
#define RESIDENT_DATE     "14.12.2008"

static const char resident_name[] = RESIDENT_NAME;
static const char resident_id[]   = "\0$VER:" RESIDENT_NAME " " STR(RESIDENT_VERSION) "." STR(RESIDENT_REVISION) " (" __DATE__ ")";

static AROS_UFP3 (APTR, Cia_Init,
		  AROS_UFPA(struct Library *, lh, D0),
		  AROS_UFPA(BPTR, segList, A0),
		  AROS_UFPA(struct ExecBase *, sysBase, A6));

struct Resident const Cia_ROMTag =
{
    RTC_MATCHWORD,
    &Cia_ROMTag,
    (APTR)(&Cia_ROMTag + 1),
    RESIDENT_FLAGS,
    RESIDENT_VERSION,
    NT_RESOURCE,
    RESIDENT_PRIORITY,
    (CONST_STRPTR)resident_name,
    (CONST_STRPTR)&resident_id[6],
    (APTR)Cia_Init
};

static const APTR Cia_FuncTable[]=
{
    &AROS_SLIB_ENTRY(AddICRVector,Cia),
    &AROS_SLIB_ENTRY(RemICRVector,Cia),
    &AROS_SLIB_ENTRY(AbleICR,Cia),
    &AROS_SLIB_ENTRY(SetICR,Cia),
    (void *)-1
};

static struct CIABase *InitResource(char *Name, struct ExecBase *SysBase)
{
    int vecsize;
    APTR mem;
    struct CIABase *base;

    vecsize = 4*LIB_VECTSIZE;
    if (vecsize > 0)
        vecsize = ((vecsize-1)/sizeof(IPTR) + 1)*sizeof(IPTR);
    mem = AllocMem(vecsize+sizeof(struct CIABase), MEMF_PUBLIC|MEMF_CLEAR);
    if (!mem)
         return NULL;

    base = (struct CIABase *)(mem + vecsize);
    base->lib.lib_Node.ln_Type = NT_RESOURCE;
    base->lib.lib_Node.ln_Pri = RESIDENT_PRIORITY;
    base->lib.lib_Node.ln_Name = Name;
    MakeFunctions(base, (APTR)Cia_FuncTable, NULL);

    AddResource(base);
    return base;
}

static AROS_UFH3 (APTR, Cia_Init,
		  AROS_UFHA(struct Library *, lh, D0),
		  AROS_UFHA(BPTR, segList, A0),
		  AROS_UFHA(struct ExecBase *, sysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct CIABase *base;

    /* Initialize ciaa.resource */
    base = InitResource("ciaa.resource", SysBase);
    if (!base)
        return NULL;

    /* Initialize ciab.resource */
    base = InitResource("ciab.resource", SysBase);

    return base;

    AROS_USERFUNC_EXIT
}
