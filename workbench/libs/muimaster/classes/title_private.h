#ifndef _TITLE_PRIVATE_H_
#define _TITLE_PRIVATE_H_

/*** Instance data **********************************************************/
struct Title_DATA
{
    struct Hook *layout_hook;
    ULONG location;
    struct MUI_EventHandlerNode ehn;
    LONG background;
    LONG activetab;
    LONG oldactivetab;
};

#endif /* _TITLE_PRIVATE_H_ */
