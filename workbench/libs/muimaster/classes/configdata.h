/*
    Copyright � 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_CONFIGDATA_H
#define _MUI_CLASSES_CONFIGDATA_H

#define MUIC_Configdata "Configdata.mui"

/* The config items for MUIM_GetConfigItem */
#define MUICFG_Invalid                  (-1L)
#define MUICFG_Window_Spacing_Left      0x01  /* ULONG, horiz pixels (def.=4) */
#define MUICFG_Window_Spacing_Right     0x02  /* ULONG, horiz pixels (def.=4) */
#define MUICFG_Window_Spacing_Top       0x03  /* ULONG, vert pixels (def.=3) */
#define MUICFG_Window_Spacing_Bottom    0x04  /* ULONG, vert pixels (def.=3) */
#define MUICFG_Radio_HSpacing           0x05  /* ULONG, horiz pixels (def.=4) */
#define MUICFG_Radio_VSpacing           0x06  /* ULONG, vertical pixels (def.=1) */
#define MUICFG_Group_HSpacing           0x07  /* ULONG, horiz pixels (def.=6) */
#define MUICFG_Group_VSpacing           0x08  /* ULONG, vertical pixels (def.=3) */
#define MUICFG_List_FontLeading         0x0b  /* ULONG, vertical pixels (def.=1) */
#define MUICFG_GroupTitle_Position      0x0f  /* ULONG, 1=centered */
#define MUICFG_GroupTitle_Color         0x10  /* ULONG, 0=normal */
#define MUICFG_Cycle_MenuCtrl_Level     0x11  /* ULONG, num of entries (def.=2) */
#define MUICFG_Cycle_MenuCtrl_Position  0x12  /* ULONG, 1=on active (def.=below=0) */
#define MUICFG_Frame_Drag               0x18
#define MUICFG_Cycle_Menu_Recessed      0x19  /* ULONG, 1=true (def.=false=0) */
#define MUICFG_Cycle_MenuCtrl_Speed     0x1a  /* ULONG, num of ticks (0..50) (def.=0) */
#define MUICFG_Font_Normal              0x1e
#define MUICFG_Font_List                0x1f
#define MUICFG_Font_Tiny                0x20
#define MUICFG_Font_Fixed               0x21
#define MUICFG_Font_Title               0x22
#define MUICFG_Font_Big	                0x23
#define MUICFG_PublicScreen             0x24
#define MUICFG_Frame_Button             0x2b
#define MUICFG_Frame_ImageButton        0x2c
#define MUICFG_Frame_Text               0x2d
#define MUICFG_Frame_String             0x2e
#define MUICFG_Frame_ReadList           0x2f
#define MUICFG_Frame_InputList          0x30
#define MUICFG_Frame_Prop               0x31
#define MUICFG_Frame_Gauge              0x32
#define MUICFG_Frame_Group              0x33
#define MUICFG_Frame_PopUp              0x34
#define MUICFG_Frame_Virtual            0x35
#define MUICFG_Frame_Slider             0x36
#define MUICFG_Background_Window        0x37
#define MUICFG_Background_Requester     0x38
#define MUICFG_Background_Button        0x39
#define MUICFG_Background_List          0x3a
#define MUICFG_Background_Text          0x3b
#define MUICFG_Background_Prop          0x3c
#define MUICFG_Background_PopUp         0x3d
#define MUICFG_Background_Selected      0x3e
#define MUICFG_Background_ListCursor    0x3f
#define MUICFG_Background_ListSelect    0x40
#define MUICFG_Background_ListSelCur    0x41
#define MUICFG_Image_ArrowUp            0x42
#define MUICFG_Image_ArrowDown          0x43
#define MUICFG_Image_ArrowLeft          0x44
#define MUICFG_Image_ArrowRight         0x45
#define MUICFG_Image_CheckMark          0x46
#define MUICFG_Image_RadioButton        0x47
#define MUICFG_Image_Cycle              0x48
#define MUICFG_Image_PopUp              0x49
#define MUICFG_Image_PopFile            0x4a
#define MUICFG_Image_PopDrawer          0x4b
#define MUICFG_Image_PropKnob           0x4c
#define MUICFG_Image_Drawer             0x4d
#define MUICFG_Image_HardDisk           0x4e
#define MUICFG_Image_Disk               0x4f
#define MUICFG_Image_Chip               0x50
#define MUICFG_Image_Volume             0x51
#define MUICFG_Image_Network            0x52
#define MUICFG_Image_Assign             0x53
#define MUICFG_Background_Register      0x54
#define MUICFG_Image_TapePlay           0x55
#define MUICFG_Image_TapePlayBack       0x56
#define MUICFG_Image_TapePause          0x57
#define MUICFG_Image_TapeStop           0x58
#define MUICFG_Image_TapeRecord         0x59
#define MUICFG_Background_Framed        0x5a
#define MUICFG_Background_Slider        0x5b
#define MUICFG_Background_SliderKnob    0x5c
#define MUICFG_Image_TapeUp             0x5d
#define MUICFG_Image_TapeDown           0x5e
#define MUICFG_Font_Button              0x80
#define MUICFG_String_Background        0x84
#define MUICFG_String_Text              0x85
#define MUICFG_String_ActiveBackground  0x86
#define MUICFG_String_ActiveText        0x87
#define MUICFG_Font_Knob                0x88
#define MUICFG_Frame_Knob               0x90
#define MUICFG_Background_Page          0x95
#define MUICFG_Background_ReadList      0x96


#define MUIM_Configdata_GetString      (METHOD_USER|0x00426621) /* Zune 20030319 */
#define MUIM_Configdata_GetULong       (METHOD_USER|0x00427253) /* Zune 20030319 */
#define MUIM_Configdata_SetULong       (METHOD_USER|0x00427224) /* Zune 20030320 */
#define MUIM_Configdata_SetImspec      (METHOD_USER|0x0042b581) /* Zune 20030323 */
#define MUIM_Configdata_SetFramespec   (METHOD_USER|0x00424b5c) /* Zune 20030331 */
#define MUIM_Configdata_SetFont        (METHOD_USER|0x004265c4) /* Zune 20030323 */
#define MUIM_Configdata_Save           (METHOD_USER|0x0042571a) /* Zune 20030320 */
#define MUIM_Configdata_Load           (METHOD_USER|0x004278ba) /* Zune 20030320 */
struct MUIP_Configdata_GetString       {ULONG MethodID; ULONG id; };
struct MUIP_Configdata_GetULong        {ULONG MethodID; ULONG id; };
struct MUIP_Configdata_SetULong        {ULONG MethodID; ULONG id; ULONG val; };
struct MUIP_Configdata_SetImspec       {ULONG MethodID; ULONG id; CONST_STRPTR imspec; };
struct MUIP_Configdata_SetFramespec    {ULONG MethodID; ULONG id; CONST_STRPTR framespec; };
struct MUIP_Configdata_SetFont         {ULONG MethodID; ULONG id; CONST_STRPTR font; };
struct MUIP_Configdata_Save            {ULONG MethodID; CONST_STRPTR filename; };
struct MUIP_Configdata_Load            {ULONG MethodID; CONST_STRPTR filename; };


#define MUIA_Configdata_Application (TAG_USER|0x10203453) /* ZV1: i..  Object * */
#define MUIA_Configdata_ZunePrefs   (TAG_USER|0x10203454) /* ZV1: PRIV .g.  struct ZunePrefsNew * */
#define MUIA_Configdata_ApplicationBase (TAG_USER|0x10203455) /* ZV1: i..  Object * */

extern const struct __MUIBuiltinClass _MUI_Configdata_desc; /* PRIV */

#endif
