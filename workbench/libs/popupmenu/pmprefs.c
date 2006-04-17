#include <dos/dos.h>
#include <proto/dos.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include <string.h>
#include "pmprefs.h"

#ifdef __AROS__
#include "pmpriv.h"
void PM_LoadPrefsFile(STRPTR filename, ULONG flags, struct PopupMenuPrefs *prefs, struct PopupMenuPrefs *defprefs);
#endif

struct PopupMenuPrefs DefaultPrefs = {
	1,			/* pmp_Flags		*/
	0,			/* pmp_SubMenuDelay	*/
	PMP_ANIM_NONE,		/* pmp_Animation	*/
	PMP_PD_SCREENBAR,	/* pmp_PulldownPos	*/
	FALSE,			/* pmp_Sticky		*/
	FALSE,			/* pmp_SameHeight	*/
	0,			/* pmp_MenuBorder	*/
	0,			/* pmp_SelItemBorder	*/
	0,			/* pmp_SeparatorBar	*/
	0,			/* pmp_MenuTitles	*/
	0,			/* pmp_MenuItems	*/
	2,			/* pmp_XOffset		*/
	2,			/* pmp_YOffset		*/
	2,			/* pmp_XSpace		*/
	2,			/* pmp_YSpace		*/
	2,			/* pmp_Intermediate	*/
	0,			/* pmp_TextDisplace	*/
	-30,			/* pmp_ShadowR		*/
	-30,			/* pmp_ShadowG		*/
	-30,			/* pmp_ShadowB		*/
	0,			/* pmp_TransparencyR	*/
	0,			/* pmp_TransparencyG	*/
	0,			/* pmp_TransparencyB	*/
	0,			/* pmp_TransparencyBlur	*/
	0,			/* pmp_AnimationSpeed	*/

	{0},			/* pmp_Reserved		*/
};

struct PopupMenuPrefs LoadedPrefs;

struct PopupMenuPrefs *PM_Prefs = &DefaultPrefs;

void PM_Prefs_Free()
{
}

void PM_Prefs_Load(STRPTR file)
{
	PM_LoadPrefsFile(file, 0, &LoadedPrefs, &DefaultPrefs);
	PM_Prefs = &LoadedPrefs;
}

void __saveds __asm PM_ReloadPrefs(void)
{
	PM_Prefs_Load(PMP_PATH);
}
