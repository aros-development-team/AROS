
#include <proto/iffparse.h>
#include <prefs/prefhdr.h>

/**************************************************************************/

#define ID_Prefs    MAKE_ID('P','R','E','F')
#define SIZE_Prefs  (sizeof(struct MPS_Prefs)-sizeof(struct MinList))

#define ID_IFNode   MAKE_ID('I','F','N','D')
#define SIZE_IFNode (sizeof(struct ifnode)-sizeof(struct MinNode))

/**************************************************************************/

ULONG saveIFFPrefs(UBYTE *file,struct MPS_Prefs *prefs, struct MiamiPanelBase_intern *MiamiPanelBaseIntern);
ULONG loadIFFPrefs(ULONG where,struct MPS_Prefs *prefs, struct MiamiPanelBase_intern *MiamiPanelBaseIntern);
