/*
   Copyright © 2002-2006, The AROS Development Team. All rights reserved.
   $Id: wandererprefsintern.h 24537 2006-06-12 20:29:02Z NicJA $
*/

#include <prefs/prefhdr.h>
#include <prefs/font.h>

struct WandererInternalPrefsData
{
  /* Wanderer Images Data */
  IPTR             WIPD_BackdropImage;
  ULONG            WIPD_BackdropImage_Width;
  ULONG            WIPD_BackdropImage_Height;
  IPTR             WIPD_WindowImage;
  ULONG            WIPD_WindowImage_Width;
  ULONG            WIPD_WindowImage_Height;

  /* Wanderer Font Data */
  struct TextFont  *WIPD_IconFont;
  struct TextAttr   WIPD_IconFontTA;
};
