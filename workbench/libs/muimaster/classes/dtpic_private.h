#ifndef _DTPIC_PRIVATE_H_
#define _DTPIC_PRIVATE_H_

#include <exec/types.h>
#include <libraries/mui.h>

/*** Instance data **********************************************************/
struct Dtpic_DATA
{
    struct Library *datatypesbase;

    STRPTR name;
    APTR dto;
    struct BitMapHeader *bmhd;
    struct BitMap *bm;
    struct MUI_EventHandlerNode ehn;

    BOOL highlighted;   // mouse pointer is within object
    BOOL selected;      // gadget is selected
    LONG deltaalpha;    // increment/decrement for each tick
    LONG currentalpha;  // the actual alpha for rendering
    BOOL eh_active;     // TRUE after MUIM_Window_AddEventHandler

    LONG alpha;
    BOOL darkenselstate;
    LONG fade;
    BOOL lightenonmouse;

};

#endif /* _DTPIC_PRIVATE_H_ */
