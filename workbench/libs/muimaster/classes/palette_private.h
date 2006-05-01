#ifndef _PALETTE_PRIVATE_H_
#define _PALETTE_PRIVATE_H_

/*** Instance data **********************************************************/
struct MUI_PaletteData
{
    const char      	      	**names;
    struct  MUI_Palette_Entry   *entries;
    struct IClass               *notifyclass;
    Object                      *list, *coloradjust;
    ULONG                       numentries;
    ULONG                       group;
    ULONG                       rgb[3];
    struct Hook                 display_hook;
    struct Hook                 setcolor_hook;
};
#endif /* _PALETTE_PRIVATE_H_ */
