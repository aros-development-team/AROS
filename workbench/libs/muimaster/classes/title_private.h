#ifndef _TITLE_PRIVATE_H_
#define _TITLE_PRIVATE_H_

/*** Instance data **********************************************************/
struct Title_DATA
{
    struct Hook *layout_hook;
    ULONG location;
    struct MUI_EventHandlerNode ehn;
    LONG activetab;
    LONG oldactivetab;
    IPTR background;
};

#endif /* _TITLE_PRIVATE_H_ */
