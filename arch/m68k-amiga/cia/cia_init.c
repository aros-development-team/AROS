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

#include <hardware/cia.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>

#define DEBUG 0
#include <aros/debug.h>

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
#define RESIDENT_PRIORITY 80
#define RESIDENT_NAME     "cia.resource"
#define RESIDENT_VERSION  0
#define RESIDENT_REVISION 1

static const char resident_name[] = RESIDENT_NAME;
static const char resident_id[]   = "\0$VER:" RESIDENT_NAME " " STR(RESIDENT_VERSION) "." STR(RESIDENT_REVISION) " (" ADATE ")";

AROS_INTP(Cia_Handler);

static AROS_UFP3 (APTR, Cia_Init,
		  AROS_UFPA(struct Library *, lh, D0),
		  AROS_UFPA(BPTR, segList, A0),
		  AROS_UFPA(struct ExecBase *, sysBase, A6));

extern void Cia_End(void);

struct Resident const Cia_ROMTag =
{
    RTC_MATCHWORD,
    &Cia_ROMTag,
    (APTR)&Cia_End,
    RESIDENT_FLAGS,
    RESIDENT_VERSION,
    NT_RESOURCE,
    RESIDENT_PRIORITY,
    resident_name,
    &resident_id[6],
    (APTR)Cia_Init
};

static const APTR Cia_FuncTable[]=
{
    &AROS_SLIB_ENTRY(AddICRVector,Cia,1),
    &AROS_SLIB_ENTRY(RemICRVector,Cia,2),
    &AROS_SLIB_ENTRY(AbleICR,Cia,3),
    &AROS_SLIB_ENTRY(SetICR,Cia,4),
    (void *)-1
};

static struct CIABase *InitResource(char *Name, struct ExecBase *SysBase)
{
    struct CIABase *base;

    base = (struct CIABase *)MakeLibrary((APTR)Cia_FuncTable, NULL, NULL, sizeof(struct CIABase), 0);

    if (base) {
	base->lib.lib_Node.ln_Type = NT_RESOURCE;
	base->lib.lib_Node.ln_Name = Name;
	base->lib.lib_Version      = RESIDENT_VERSION;
	base->lib.lib_IdString     = (STRPTR)&resident_id[6];
	base->lib.lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;
	base->lib.lib_Revision     = RESIDENT_REVISION;

	AddResource(base);
    }
    return base;
}

THIS_PROGRAM_HANDLES_SYMBOLSET(INIT)
DECLARESET(INIT)

static AROS_UFH3 (APTR, Cia_Init,
		  AROS_UFHA(struct Library *, lh, D0),
		  AROS_UFHA(BPTR, segList, A0),
		  AROS_UFHA(struct ExecBase *, sysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct CIABase *base;

    /* Process any init sets */
    if (!set_call_libfuncs(SETNAME(INIT), 1, 1, SysBase))
    	    return NULL;

    /* Initialize ciaa.resource */
    base = InitResource("ciaa.resource", SysBase);
    if (!base)
        return NULL;

    base->hw = (struct CIA*)0xbfe001;
    base->hw->ciaicr = 0x7f;
    base->hw->ciacra = 0x00;
    base->hw->ciacrb = 0x80;
    base->hw->ciatodhi = 0x00;
    base->hw->ciatodmid = 0x0f;
    base->hw->ciatodlow = 0x00;
    base->hw->ciacrb = 0x00;
    base->hw->ciapra = 0x00;
    base->hw->ciaddra = 0x03;
    base->hw->ciaddrb = 0xff;
    base->inten_mask = INTF_PORTS;

    base->ciaint.is_Node.ln_Pri = 120;
    base->ciaint.is_Node.ln_Type = NT_INTERRUPT;
    base->ciaint.is_Node.ln_Name = "CIA-A";
    base->ciaint.is_Code = (APTR)Cia_Handler;
    base->ciaint.is_Data = base;
    AddIntServer(INTB_PORTS, &base->ciaint);

    D(bug("CIA-A %p\n", base));

    /* Initialize ciab.resource */
    base = InitResource("ciab.resource", SysBase);
    if (!base)
        return NULL;

    base->hw = (struct CIA*)0xbfd000;
    base->hw->ciaicr = 0x7f;
    base->hw->ciacra = 0x00;
    base->hw->ciacrb = 0x80;
    base->hw->ciatodhi = 0x00;
    base->hw->ciatodmid = 0x0f;
    base->hw->ciatodlow = 0x00;
    base->hw->ciacrb = 0x00;
    base->hw->ciapra = 0xff;
    base->hw->ciaprb = 0xff;
    base->hw->ciaddra = 0xff;
    base->hw->ciaddrb = 0xff;
    base->inten_mask = INTF_EXTER;

    base->ciaint.is_Node.ln_Pri = 120;
    base->ciaint.is_Node.ln_Type = NT_INTERRUPT;
    base->ciaint.is_Node.ln_Name = "CIA-B";
    base->ciaint.is_Code = (APTR)Cia_Handler;
    base->ciaint.is_Data = base;
    AddIntServer(INTB_EXTER, &base->ciaint);
 
    D(bug("CIA-B %p\n", base));
 
    return base;

    AROS_USERFUNC_EXIT
}
