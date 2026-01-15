#ifndef _PANEL_STATUSBAR_H_
#define _PANEL_STATUSBAR_H_

struct panel_StatusBar_DATA
{
    struct Node                         iwp_Node;
    IPTR                                iwp_Flags;
    Object                              *iwp_StatusBar_StatusBarObj;
    Object                              *iwp_StatusBar_StatusTextObj;
    struct Hook                         iwp_StatusBar_updateHook;
};
#endif
