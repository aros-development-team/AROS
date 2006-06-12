
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
