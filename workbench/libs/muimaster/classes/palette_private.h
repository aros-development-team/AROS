#ifndef _PALETTE_PRIVATE_H_
#define _PALETTE_PRIVATE_H_

#include <utility/hooks.h>
#include <libraries/mui.h>

/*** Instance data **********************************************************/
struct MUI_PaletteData
{
    const char                  **names;
    struct  MUI_Palette_Entry   *entries;
    Object                      *list, *coloradjust;
    ULONG                       numentries;
    ULONG                       group;
    ULONG                       rgb[3];
    struct Hook                 display_hook;
    struct Hook                 setcolor_hook;
    char                        buf[20];
};
#endif /* _PALETTE_PRIVATE_H_ */
