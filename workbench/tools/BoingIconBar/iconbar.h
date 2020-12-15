#ifndef ICONBAR_H
#define ICONBAR_H

struct Icon_Struct {
    LONG                Icon_Height;            // height icon
    LONG                Icon_Width;             // width icon
    LONG                Icon_PositionX;         // X position on main window
    LONG                Icon_PositionY;         // Y position on main window
    LONG                Icon_Status;            // status of icon: normal or selected
    BOOL                Icon_OK;                // everything OK with icon
    TEXT                Icon_Path[255];         // name to path of icon
    LONG                IK_Label_Length;        // length of label under icon in chars 
    STRPTR              IK_Label;               // icon label
};

struct Level_Struct {
    TEXT                Level_Name[20];         // name submenu - level name 
    LONG                Beginning;              // first icon on menu
    LONG                WindowPos_X;            // X position main window
    LONG                WindowPos_Y;            // Y position main window
};

struct Struct_BackgroundData {
    LONG                Width;                  // width
    LONG                Height;                 // height
};

#endif
