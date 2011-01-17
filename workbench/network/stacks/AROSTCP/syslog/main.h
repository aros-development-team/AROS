/* MediaLogger */

#include <intuition/classusr.h>
#include <syslog.h>

#define findobj(parent, id) (Object*)DoMethod(parent, MUIM_FindUData, id)

#define MENUBAR                -1
#define MENU_QUIT               7
#define MENU_SAVEAS             8
#define MENU_SETTINGS           9
#define MENU_CLEAR              10

#define PREFS_BUTTON_CANCEL     24
#define PREFS_BUTTON_USE        25
#define PREFS_BUTTON_SAVE       26
#define PREFS_POPPEN_ERRORS     27
#define PREFS_POPPEN_IMPORTANT  28
#define PREFS_POPPEN_OTHERS     29

extern struct Library	*MUIMasterBase;
#if !defined(__AROS__)
extern struct Library  *UtilityBase,
									*GfxBase,
									*SysBase,
									*DOSBase,
									*LocaleBase,
									*IntuitionBase;
#endif

extern struct Locale *Locale;
extern Object *List, *SWin, *DbgLevel;

struct SysLogEntry
{
  ULONG  Level;
  STRPTR Time;
  STRPTR ProcessName;
  STRPTR EventDescription;
};
