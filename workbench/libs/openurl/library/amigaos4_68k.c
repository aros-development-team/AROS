/*
**  openurl.library - universal URL display and browser
**  launcher library
**
**  AmigaOS4 68k Jump table - Provides jump table for old
**  68k programs.
**
**  Written by Alexandre Balaban <alexandre@balaban.name>
*/

#undef __USE_INLINE__

#if !defined(__amigaos4__)
#  error This file is AmigaOS4 specific and should not be used on other platforms
#endif

// ABA, DON'T KNOW WHY, BUT SOMETHING SEEMS TO BE MESSED INTO OPENURL.H
#define URL_GetPrefs_Default URL_GetPrefs_Mode

#include <exec/emulation.h>
#include <libraries/openurl.h>
#include <interfaces/openurl.h>

/* amigaos4 *****************************************************************/

STATIC ULONG stub_OpenPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct LibraryManagerInterface *Self = (struct LibraryManagerInterface *) ExtLib->ILibrary;

	return (ULONG)Self->Open(0);
}
struct EmuTrap stub_Open = { TRAPINST, TRAPTYPE, stub_OpenPPC };

/* amigaos4 *****************************************************************/

STATIC ULONG stub_ClosePPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct LibraryManagerInterface *Self = (struct LibraryManagerInterface *) ExtLib->ILibrary;

	return (ULONG)Self->Close();
}
struct EmuTrap stub_Close = { TRAPINST, TRAPTYPE, stub_ClosePPC };

/* amigaos4 *****************************************************************/

STATIC ULONG stub_ExpungePPC(ULONG *regarray)
{
	return 0UL;
}
struct EmuTrap stub_Expunge = { TRAPINST, TRAPTYPE, stub_ExpungePPC };

/* amigaos4 *****************************************************************/

STATIC ULONG stub_ReservedPPC(ULONG *regarray)
{
	return 0UL;
}
struct EmuTrap stub_Reserved = { TRAPINST, TRAPTYPE, stub_ReservedPPC };

/* amigaos4 *****************************************************************/

static ULONG stub_URL_OpenAPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct OpenURLIFace *Self = (struct OpenURLIFace *) ExtLib->MainIFace;

	return Self->URL_OpenA(
		(STRPTR)regarray[REG68K_A0/4],
		(struct TagItem *)regarray[REG68K_A1/4]
	);
}
struct EmuTrap stub_URL_OpenA = { TRAPINST, TRAPTYPE, stub_URL_OpenAPPC };

/* amigaos4 *****************************************************************/

static struct URL_Prefs * stub_URL_OldGetPrefsPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct OpenURLIFace *Self = (struct OpenURLIFace *) ExtLib->MainIFace;

	return Self->URL_GetPrefsA(
		  NULL
	);
}
struct EmuTrap stub_URL_OldGetPrefs = { TRAPINST, TRAPTYPE, (ULONG (*)(ULONG *))stub_URL_OldGetPrefsPPC };

/* amigaos4 *****************************************************************/

static void stub_URL_OldFreePrefsPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct OpenURLIFace *Self = (struct OpenURLIFace *) ExtLib->MainIFace;

	Self->URL_FreePrefs(
		(struct URL_Prefs *)regarray[REG68K_A0/4],
		  NULL
	);
}
struct EmuTrap stub_URL_OldFreePrefs = { TRAPINST, TRAPTYPE, (ULONG (*)(ULONG *))stub_URL_OldFreePrefsPPC };

/* amigaos4 *****************************************************************/

static ULONG stub_URL_OldSetPrefsPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct OpenURLIFace *Self = (struct OpenURLIFace *) ExtLib->MainIFace;

	 struct TagItem stags[] = {URL_SetPrefs_Save,0,TAG_DONE};

	 stags[0].ti_Data = regarray[REG68K_D0/4];

	return Self->URL_SetPrefsA(
		(struct URL_Prefs *)regarray[REG68K_A0/4],
		stags
	);
}
struct EmuTrap stub_URL_OldSetPrefs = { TRAPINST, TRAPTYPE, stub_URL_OldSetPrefsPPC };

/* amigaos4 *****************************************************************/

static struct URL_Prefs * stub_URL_OldGetDefaultPrefsPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct OpenURLIFace *Self = (struct OpenURLIFace *) ExtLib->MainIFace;

	return Self->URL_GetPrefs(
		  URL_GetPrefs_Default,
		  TRUE,
		  TAG_DONE
	);
}
struct EmuTrap stub_URL_OldGetDefaultPrefs = { TRAPINST, TRAPTYPE, (ULONG (*)(ULONG *))stub_URL_OldGetDefaultPrefsPPC };

/* amigaos4 *****************************************************************/

static ULONG stub_URL_OldLaunchPrefsAppPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct OpenURLIFace *Self = (struct OpenURLIFace *) ExtLib->MainIFace;

	return Self->URL_LaunchPrefsAppA(
		 NULL
	 );
}
struct EmuTrap stub_URL_OldLaunchPrefsApp = { TRAPINST, TRAPTYPE, (ULONG (*)(ULONG *))stub_URL_OldLaunchPrefsAppPPC };

/* amigaos4 *****************************************************************/
LONG VARARGS68K OS4_dispatch ( struct OpenURLIFace * Self, struct RexxMsg *msg, UBYTE ** resPtr );

static LONG stub_dispatchPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct OpenURLIFace *Self = (struct OpenURLIFace *) ExtLib->MainIFace;
    UBYTE * pRes = NULL;

	LONG res= OS4_dispatch(
		  Self,
		(struct RexxMsg*)regarray[REG68K_A0/4],
				&pRes
	);

    regarray[REG68K_A0/4] = (ULONG)pRes;

    return res;
}
struct EmuTrap stub_dispatch = { TRAPINST, TRAPTYPE, (ULONG (*)(ULONG *))stub_dispatchPPC };

/* amigaos4 *****************************************************************/

static struct URL_Prefs * stub_URL_GetPrefsAPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct OpenURLIFace *Self = (struct OpenURLIFace *) ExtLib->MainIFace;

	return Self->URL_GetPrefsA(
		(struct TagItem *)regarray[REG68K_A0/4]
	);
}
struct EmuTrap stub_URL_GetPrefsA = { TRAPINST, TRAPTYPE, (ULONG (*)(ULONG *))stub_URL_GetPrefsAPPC };

/* amigaos4 *****************************************************************/

static void stub_URL_FreePrefsAPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct OpenURLIFace *Self = (struct OpenURLIFace *) ExtLib->MainIFace;

	Self->URL_FreePrefsA(
		(struct URL_Prefs *)regarray[REG68K_A0/4],
		(struct TagItem *)regarray[REG68K_A1/4]
	);
}
struct EmuTrap stub_URL_FreePrefsA = { TRAPINST, TRAPTYPE, (ULONG (*)(ULONG *))stub_URL_FreePrefsAPPC };

/* amigaos4 *****************************************************************/

static ULONG stub_URL_SetPrefsAPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct OpenURLIFace *Self = (struct OpenURLIFace *) ExtLib->MainIFace;

	return Self->URL_SetPrefsA(
		(struct URL_Prefs *)regarray[REG68K_A0/4],
		(struct TagItem *)regarray[REG68K_A1/4]
	);
}
struct EmuTrap stub_URL_SetPrefsA = { TRAPINST, TRAPTYPE, stub_URL_SetPrefsAPPC };

/* amigaos4 *****************************************************************/

static ULONG stub_URL_LaunchPrefsAppAPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct OpenURLIFace *Self = (struct OpenURLIFace *) ExtLib->MainIFace;

	return Self->URL_LaunchPrefsAppA(
		(struct TagItem *)regarray[REG68K_A1/4]
	);
}
struct EmuTrap stub_URL_LaunchPrefsAppA = { TRAPINST, TRAPTYPE, stub_URL_LaunchPrefsAppAPPC };

/* amigaos4 *****************************************************************/

static ULONG stub_URL_GetAttrPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct OpenURLIFace *Self = (struct OpenURLIFace *) ExtLib->MainIFace;

	return Self->URL_GetAttr(
		(ULONG)regarray[REG68K_D0/4],
		(ULONG*)regarray[REG68K_A0/4]
	);
}
struct EmuTrap stub_URL_GetAttr = { TRAPINST, TRAPTYPE, stub_URL_GetAttrPPC };

/* amigaos4 *****************************************************************/

ULONG VecTable68K[] = {
	(ULONG)&stub_Open,
	(ULONG)&stub_Close,
	(ULONG)&stub_Expunge,
	(ULONG)&stub_Reserved,

	 (ULONG)&stub_URL_OpenA,
	 (ULONG)&stub_URL_OldGetPrefs,
	 (ULONG)&stub_URL_OldFreePrefs,
	 (ULONG)&stub_URL_OldSetPrefs,
	 (ULONG)&stub_URL_OldGetDefaultPrefs,
	 (ULONG)&stub_URL_OldLaunchPrefsApp,
	 (ULONG)&stub_dispatch,
	 (ULONG)&stub_URL_GetPrefsA,
	 (ULONG)&stub_URL_FreePrefsA,
	 (ULONG)&stub_URL_SetPrefsA,
	 (ULONG)&stub_URL_LaunchPrefsAppA,
	 (ULONG)&stub_URL_GetAttr,

	(ULONG)-1
};
