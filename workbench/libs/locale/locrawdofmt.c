/*
    Copyright (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: Locale_RawDoFmt - locale.library's private replacement
    	  of exec.library/RawDoFmt function. IPrefs will install
	  the patch.
	  
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include "locale_intern.h"
#include <aros/asmcall.h>

extern struct LocaleBase *globallocalebase;

#define LocaleBase globallocalebase

AROS_UFH3(VOID, LocRawDoFmtFormatStringFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct Locale *, locale, A2),
    AROS_UFHA(char, fill, A1))
{
    AROS_USERFUNC_INIT

#ifdef __mc68000__
    register APTR pdata __asm(A3) = hook->h_Data;
#else
    APTR pdata = hook->h_Data;
#endif

    AROS_UFC3(void, hook->h_SubEntry,
    	AROS_UFCA(char, fill, D0),
	AROS_UFCA(APTR, pdata, A3),
	AROS_UFCA(struct ExecBase *, IntLB(LocaleBase)->lb_SysBase, A6));

#ifdef __mc68000__
    hook->h_Data = pdata;
#endif

    AROS_USERFUNC_EXIT
}

#undef LocaleBase

 /*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH4(APTR, LocRawDoFmt,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, FormatString, A0),
	AROS_LHA(APTR        , DataStream, A1),
	AROS_LHA(VOID_FUNC   , PutChProc, A2),
	AROS_LHA(APTR        , PutChData, A3),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 31, Locale)

/*  FUNCTION
    	See exec.library/RawDoFmt
	
    INPUTS
    	See exec.library/RawDoFmt

    RESULT

    NOTES
    	This function is not called by apps directly. Instead exec.library/RawDoFmt
	is patched to use this function. This means, that the LocaleBase parameter
	above actually points to SysBase!!! But I may not rename it, because then
	no entry for this function is generated in the Locale functable by the
	corresponding script!
	
    EXAMPLE

    BUGS

    SEE ALSO
	RawDoFmt(), FormatString().

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    locale_lib.fd and clib/locale_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

#define LocaleBase globallocalebase

    struct Hook       hook;
    APTR    	      retval;
    
    hook.h_Entry    = (HOOKFUNC)LocRawDoFmtFormatStringFunc;
    hook.h_SubEntry = (HOOKFUNC)PutChProc;
    hook.h_Data     = PutChData;

    //kprintf("LocRawDoFmt: FormatString = \"%s\"\n", FormatString);
 
    REPLACEMENT_LOCK;
    
    retval = FormatString(&(IntLB(LocaleBase)->lb_CurrentLocale->il_Locale),
    	    	    	  (STRPTR)FormatString,
			  DataStream,
			  &hook);

    REPLACEMENT_UNLOCK;
    
    //kprintf("LocRawDoFmt: FormatString: returning %x\n", retval);
    
    return retval;
    
    AROS_LIBFUNC_EXIT
    
} /* LocRawDoFmt */

#undef LocaleBase
