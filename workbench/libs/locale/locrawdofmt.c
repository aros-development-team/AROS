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

#ifdef __MORPHOS__

/* move.b d0,(a3)+
   rts
*/
#define ARRAY_FUNC      0x16c04e75

/* KPutChar
	MOVE.L  A6,-(SP)        ;2F0E
	MOVEA.L (4).L,A6        ;2C7900000004
	JSR     (-$0204,A6)     ;4EAEFDFC
	MOVEA.L (SP)+,A6        ;2C5F
	RTS                     ;4E75
*/

#define SERIAL_FUNC0    0x2f0e2c79
#define SERIAL_FUNC1    0x00000004
#define SERIAL_FUNC2    0x4eaefdfc
#define SERIAL_FUNC3    0x2c5f4e75

struct HookData
{
	char	*PutChData;
	ULONG	OldA4;
};

/* Quick reg layout function
 */
char *_PPCCallM68k_RawDoFmt(char MyChar,
			    char *(*PutChProc)(char*,char),
			    char *PutChData,
			    ULONG OldA4,
			    struct ExecBase *sysBase);

char *PPCCallM68k_RawDoFmt(char MyChar,
			   char *(*PutChProc)(char*,char),
			   char *PutChData,
			   ULONG OldA4,
			   struct ExecBase *sysBase)
{
  /* As we call a *QUICK REG LAYOUT* function
   * below we must make sure that this function backups/restores
   * all registers
   */
  asm(""
      :
      :
      : "r13");

  return _PPCCallM68k_RawDoFmt(MyChar,
                               PutChProc,
			       PutChData,
			       OldA4,
			       sysBase);
}
#endif


AROS_UFH3(VOID, LocRawDoFmtFormatStringFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct Locale *, locale, A2),
    AROS_UFHA(char, fill, A1))
{
    AROS_USERFUNC_INIT


#ifdef __MORPHOS__
    struct HookData *data = hook->h_Data;
    char str[2];

    switch ((ULONG) hook->h_SubEntry)
    {
      case 0:
	/* Standard Array Function */
	*data->PutChData++ = fill;
	break;

      case 1:
	/* Standard Serial Function */
	str[0] = fill;
	str[1] = '\0';
	break;

      default:
	data->PutChData = PPCCallM68k_RawDoFmt(fill,
				     hook->h_SubEntry,
					       data->PutChData,
					       data->OldA4,
					       IntLB(LocaleBase)->lb_SysBase);
	break;
    }
#else

#ifdef __mc68000__
    register APTR pdata __asm(A3) = hook->h_Data;
#endif

    AROS_UFC3(void, hook->h_SubEntry,
    	AROS_UFCA(char, fill, D0),
	AROS_UFCA(APTR, pdata, A3),
	AROS_UFCA(struct ExecBase *, IntLB(LocaleBase)->lb_SysBase, A6));

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
    struct HookData   data;
    APTR    	      retval;

#ifdef __MORPHOS__

    if ((ULONG) PutChProc > 1)
    {
	if (*((ULONG*) PutChProc) == ARRAY_FUNC)
	{
	    PutChProc = 0;
	}
	else if ((((ULONG*) PutChProc)[0] == SERIAL_FUNC0) &&
		 (((ULONG*) PutChProc)[1] == SERIAL_FUNC1) &&
		 (((ULONG*) PutChProc)[2] == SERIAL_FUNC2) &&
		 (((ULONG*) PutChProc)[3] == SERIAL_FUNC3))
	{
	    PutChProc = (APTR) 1;
	}
    }

    hook.h_Entry    = (HOOKFUNC)AROS_ASMSYMNAME(LocRawDoFmtFormatStringFunc);
    hook.h_SubEntry = (HOOKFUNC)PutChProc;
    hook.h_Data     = &data;
    data.PutChData  = PutChData;
    data.OldA4      = REG_A4;

#else

    hook.h_Entry    = (HOOKFUNC)AROS_ASMSYMNAME(LocRawDoFmtFormatStringFunc);
    hook.h_SubEntry = (HOOKFUNC)PutChProc;
    hook.h_Data     = PutChData;

#endif

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
