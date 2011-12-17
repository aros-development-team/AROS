/*
   Copyright © 2002-2011, The AROS Development Team. All rights reserved.
   $Id$
*/

#include <prefs/prefhdr.h>
#include <prefs/font.h>

struct WandererFontPrefsData
{
  /* Wanderer Font Data */
  struct TextFont   *wfpd_IconFont;
  struct TextAttr   wfpd_IconFontTA;
  struct TextFont   *wfpd_OldIconFont;
};

void WandererPrefs_ReloadFontPrefs(CONST_STRPTR prefsfile, struct WandererFontPrefsData *wfpd);
void WandererPrefs_CloseOldIconFont(struct WandererFontPrefsData *wfpd);
