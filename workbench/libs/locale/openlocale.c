/*
    Copyright (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: OpenLocale() - Give access to a new locale.
    Lang: english
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/locale.h>
#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>
#include <proto/dos.h>
#include <prefs/locale.h>
#include <proto/exec.h>
#include <proto/iffparse.h>
#include "locale_intern.h"

extern void InitLocale(
    STRPTR filename,
    struct IntLocale *,
    struct LocalePrefs *,
    struct LocaleBase *);

/*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH1(struct Locale *, OpenLocale,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, A0),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 26, Locale)

/*  FUNCTION
	This function will open for use a named locale. A locale is a
	data structure that contains many different parameters that
	an application needs in order to localise itself. Using this
	information an application can dynamically adapt to the user's
	environment.

	Locales are created using the Locale Preferences Editor. If
	you pass a NULL instead of a name, then you will receive the
	current default Locale. This is the normal procedure.

    INPUTS
	name    -   The name of the locale you wish opened, or NULL
		    to open the current default locale. This will
		    be an IFF PREF file which contains both LCLE
		    and CTRY chunks.

    RESULT
	A pointer to an initialised Locale structure, or NULL if none
	could be opened. If NULL is returned you can use IoErr()
	to find out what caused this error.

	If you pass NULL, you will always succeed.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CloseLocale()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    locale_lib.fd and clib/locale_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct LocaleBase *,LocaleBase)

    struct IntLocale *locale = NULL;

    /* Have we been asked for a disk-based locale? */
    if(name != NULL)
    {
	struct IFFHandle *iff;
	ULONG error;
	struct LocalePrefs *lp;
	struct ContextNode *cn;

	/* Clear error condition before we start. */
	SetIoErr(0);

	lp = AllocMem(sizeof(struct LocalePrefs), MEMF_CLEAR);
	if( lp == NULL )
	{
	    SetIoErr(ERROR_NO_FREE_STORE);
	    return NULL;
	}

	iff = AllocIFF();
	if(iff == NULL)
	{
	    FreeMem(lp, sizeof(struct LocalePrefs));
	    SetIoErr(ERROR_NO_FREE_STORE);
	    return NULL;
	}

	iff->iff_Stream = (ULONG)Open(name, MODE_OLDFILE);
	if(iff->iff_Stream == NULL)
	{
	    FreeMem(lp, sizeof(struct LocalePrefs));
	    FreeIFF(iff);
	    return NULL;
	}

	InitIFFasDOS(iff);

	if(!OpenIFF(iff, IFFF_READ))
	{
	    if(!StopChunk(iff, ID_PREF, ID_LCLE))
	    {
		while(1)
		{
		    error = ParseIFF(iff, IFFPARSE_SCAN);
		    if(error == 0)
		    {
			cn = CurrentChunk(iff);

			if((cn->cn_ID == ID_LCLE) && (cn->cn_Type == ID_PREF))
			{
			    if(ReadChunkBytes(iff, lp, sizeof(struct LocalePrefs)) == sizeof(struct LocalePrefs))
			    {
				locale = AllocMem(sizeof(struct IntLocale), MEMF_CLEAR|MEMF_PUBLIC);
				if(locale)
				{
				    InitLocale(name, locale, lp, LocaleBase);
				    break;
				}
				else
				    SetIoErr(ERROR_NO_FREE_STORE);
			    }
			}
		    } /* from a stop chunk */
		    else if(error != IFFERR_EOC)
			break;

		} /* while(1) */
	    } /* StopChunk() */

	    CloseIFF(iff);
	}
	Close((BPTR)iff->iff_Stream);
	FreeIFF(iff);
	FreeMem(lp, sizeof(struct LocalePrefs));
    }
    else
    {
	/* Return the current default */
	ObtainSemaphore(&IntLB(LocaleBase)->lb_LocaleLock);
	locale = IntLB(LocaleBase)->lb_CurrentLocale;
	locale->il_Count++;
	ReleaseSemaphore(&IntLB(LocaleBase)->lb_LocaleLock);
    }

    /* We let the optimiser do some CSE above */
    return (struct Locale *)locale;

    AROS_LIBFUNC_EXIT
} /* OpenLocale */
