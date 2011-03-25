#include <aros/kernel.h>
#include <dos/bptr.h>
#include <proto/alib.h>
#include <proto/exec.h>

#include <inttypes.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include "debug_intern.h"

#define D(x)
#define DSEGS(x)

static void FindSymbol(module_t *mod, char **function, void **funstart, void **funend, void *addr)
{
    dbg_sym_t *sym = mod->m_symbols;
    unsigned int i;

    if (!addr)
	return;

    for (i = 0; i < mod->m_symcnt; i++)
    {
        APTR highest = sym[i].s_highest;

	/* Symbols with zero length have zero in s_highest */
	if (!highest)
	    highest = sym[i].s_lowest;

	if (sym[i].s_lowest <= addr && highest >= addr) {
	    *function = sym[i].s_name;
	    *funstart = sym[i].s_lowest;
	    *funend   = sym[i].s_highest;

	    return;
	}
    }

    /* Indicate that symbol not found */
    *function = NULL;
    *funstart = NULL;
    *funend   = NULL;
}

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH2(int, KrnDecodeLocationA,

/*  SYNOPSIS */
	AROS_LHA(void *, addr, A0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct KernelBase *, KernelBase, 24, Kernel)

/*  FUNCTION
	Locate the given address in the list of registered modules and return
	information about it.

    INPUTS
	addr - An address to resolve
	tags - An optional taglist. ti_Tag can be one of the following tags and
	       ti_Data is always a pointer to a storage of specified type.
	       Resulting values will be placed into specified locations if the
	       function succeeds.

	    KDL_ModuleName     (char *) - Module name
	    KDL_SegmentName    (char *) - Segment name. Can be NULL if there were
					  no segment names provided for the module.
	    KDL_SegmentPointer (BPTR)   - DOS pointer to the corresponding segment.
					  Note that it will be different from
					  KDL_SegmentStart value
	    
	    KDL_SegmentNumber  (unsigned int) - Order number of the segment in the
						module
	    KDL_SegmentStart   (void *) - Start address of actual segment contents
					  in memory.
	    KDL_SegmentEnd     (void *) - End address of actual segment contents
					  in memory.
	
	    The following tags may return NULL values if there was no corresponding
	    information provided for the module:

	    KDL_SymbolName     (char *) - Symbol name (function or variable name)
	    KDL_SymbolStart    (void *) - Start address of contents described by this
					  symbol.
	    KDL_SymbolEnd      (void *) - End address of contents described by this
					  symbol.

    RESULT
	Zero if lookup failed and no corresponding module found, nonzero
	otherwise.

    NOTES
	If the function fails values pointed to by taglist will not be changed.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct segment *seg;
    void *dummy;
    char **module   = (char **)&dummy;
    char **segment  = (char **)&dummy;
    char **function = (char **)&dummy;
    void **secstart = &dummy;
    void **secend   = &dummy;
    void **funstart = &dummy;
    void **funend   = &dummy;
    BPTR  *secptr   = (BPTR *)&dummy;
    unsigned int *secnum = (unsigned int *)&dummy;
    const struct TagItem *tstate = tags;
    struct TagItem *tag;
    void *symaddr = NULL;
    int ret = 0;
    int super;

    D(bug("[KRN] KrnDecodeLocationA(0x%p)\n", addr));

    /* Parse TagList */
    while ((tag = LibNextTagItem(&tstate)))
    {
	switch (tag->ti_Tag) {
	case KDL_ModuleName:
	    module = (char **)tag->ti_Data;
	    break;

	case KDL_SegmentName:
	    segment = (char **)tag->ti_Data;
	    break;

	case KDL_SegmentPointer:
	    secptr = (BPTR *)tag->ti_Data;
	    break;

	case KDL_SegmentNumber:
	    secnum = (unsigned int *)tag->ti_Data;
	    break;

	case KDL_SegmentStart:
	    secstart = (void **)tag->ti_Data;
	    break;

	case KDL_SegmentEnd:
	    secend = (void **)tag->ti_Data;
	    break;

	case KDL_SymbolName:
	    function = (char **)tag->ti_Data;
	    symaddr = addr;
	    break;

	case KDL_SymbolStart:
	    funstart = (void **)tag->ti_Data;
	    symaddr = addr;
	    break;

	case KDL_SymbolEnd:
	    funend = (void **)tag->ti_Data;
	    symaddr = addr;
	    break;
	}
    }

    /* We can be called in supervisor mode. No semaphores in the case! */
    super = KrnIsSuper();
    if (!super)
	ObtainSemaphoreShared(&KernelBase->kb_ModSem);

    ForeachNode(&KernelBase->kb_Modules, seg)
    {
        DSEGS(bug("[KRN] Checking segment 0x%p - 0x%p, num %u, module %s\n", seg->s_lowest, seg->s_highest, seg->s_num, seg->s_mod->m_name));

	/* if address suits the segment bounds, you got it */
	if ((seg->s_lowest <= addr) && (seg->s_highest >= addr))
	{
	    D(bug("[KRN] Found module %s, Segment %u (%s, 0x%p - 0x%p)\n", seg->s_mod->m_name, seg->s_num,
		   seg->s_name, seg->s_lowest, seg->s_highest));

	    *module   = seg->s_mod->m_name;
	    *segment  = seg->s_name;
	    *secptr   = seg->s_seg;
	    *secnum   = seg->s_num;
	    *secstart = seg->s_lowest;
	    *secend   = seg->s_highest;

	    /* Now look up the function if requested */
	    FindSymbol(seg->s_mod, function, funstart, funend, symaddr);

	    ret = 1;
	    break;
	}
    }

    if (!super)
	ReleaseSemaphore(&KernelBase->kb_ModSem);

    /* Try to search kernel debug information if found nothing */
    if (!ret)
    {
    	struct ELF_ModuleInfo *kmod;

	D(bug("[KRN] Checking kernel modules...\n"));

    	for (kmod = KernelBase->kb_KernelModules; kmod; kmod = kmod->Next)
    	{
	    /* We understand only ELF here */
    	    if (kmod->Type == DEBUG_ELF)
    	    {
	    	struct elfheader *eh     = kmod->eh;
		struct sheader *sections = kmod->sh;
		ULONG int_shnum    = eh->shnum;
		ULONG int_shstrndx = eh->shstrndx;
		ULONG shstr;
		unsigned int i;

            	/* Get wider versions of shnum and shstrndx from first section header if needed */
	        if (int_shnum == 0)
        	    int_shnum = sections[0].size;
	        if (int_shstrndx == SHN_XINDEX)
        	    int_shstrndx = sections[0].link;

		shstr = SHINDEX(int_shstrndx);

		D(bug("[KRN] Module %s, %d sections at 0x%p\n", kmod->Name, int_shnum, sections));

		for (i=0; i < int_shnum; i++)
	    	{
	    	    void *s_lowest = sections[i].addr;

		    /* Ignore all empty segments */
		    if (s_lowest && sections[i].size)
		    {
			void *s_highest = s_lowest + sections[i].size - 1;

			if ((s_lowest <= addr) && (s_highest >= addr))
			{
			    char *s_name = NULL;

			    if (sections[shstr].type == SHT_STRTAB)
				s_name = sections[shstr].addr + sections[i].name;

			    D(bug("[KRN] Found module %s, Segment %u (%s, 0x%p - 0x%p)\n", kmod->Name, i, kseg->s_num,
				  s_name, s_lowest, s_highest));

			    *module   = (char *)kmod->Name;
			    *segment  = s_name;
			    *secptr   = NULL;
			    *secnum   = i;
			    *secstart = s_lowest;
			    *secend   = s_highest;

			    ret = 1;
			    break;
			}
		    }
		}

		/* If we are in correct module, let's try to look up the symbol */
		if (ret)
		{
		    char *m_str = NULL;

		    /* Find symbols name table */
		    for (i = 0; i < int_shnum; i++)
		    {
			if ((sections[i].type == SHT_STRTAB) && (i != shstr))
			{
			    m_str = sections[i].addr;
			    D(bug("[KRN] Symbol name table of length %d in section %d at 0x%p\n", sections[i].size, i, m_str));
			}
		    }

		    for (i = 0; i < int_shnum; i++)
		    {
		    	if (sections[i].addr && sections[i].type == SHT_SYMTAB)
			{
			    struct symbol *st = (struct symbol *)sections[i].addr;
			    unsigned int symcnt = sections[i].size / sizeof(struct symbol);
			    unsigned int j;

			    for (j=0; j < symcnt; j++)
			    {
			    	int idx = st[j].shindex;
			    	void *s_lowest, *s_highest, *cmp_highest;
			    	char *s_name;

				/* Ignore these - they should not be here at all */
			    	if ((idx == SHN_UNDEF) || (idx == SHN_COMMON))
				    continue;
				/* TODO: perhaps XINDEX support is needed */
				if (idx == SHN_XINDEX)
				    continue;

				s_name = (m_str) ? m_str + st[j].name : NULL;

				s_lowest = (void *)st[j].value;
				if (idx != SHN_ABS)
				    s_lowest += (IPTR)sections[idx].addr;

				if (st[j].size)
				{
				    s_highest = s_lowest + st[j].size - 1;
				    cmp_highest = s_highest;
				}
				else
				{
				    s_highest = NULL;
				    cmp_highest = s_lowest;
				}
				
				if ((s_lowest <= addr) && (cmp_highest >= addr))
				{
				    *function = s_name;
				    *funstart = s_lowest;
				    *funend   = s_highest;

				    return 1;
				}
			    }
			}
		    }
		    break;
		}
	    }
	}
    }

    return ret;

    AROS_LIBFUNC_EXIT
}
