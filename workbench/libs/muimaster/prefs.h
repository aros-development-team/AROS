#ifndef __ZUNE_PREFS_H__
#define __ZUNE_PREFS_H__

typedef enum CycleMenuPosition {
    CYCLE_MENU_POSITION_CENTERED,
    CYCLE_MENU_POSITION_TOP,
    CYCLE_MENU_POSITION_BOTTOM,
} CycleMenuPosition;

typedef enum GroupTitlePosition {
    GROUP_TITLE_POSITION_CENTERED,
    GROUP_TITLE_POSITION_LEFT,
    GROUP_TITLE_POSITION_RIGHT,
} GroupTitlePosition;

typedef enum GroupTitleColor {
    GROUP_TITLE_COLOR_WHITE,
    GROUP_TITLE_COLOR_BLACK,
    GROUP_TITLE_COLOR_3D,
} GroupTitleColor;

typedef enum WindowPosition {
    WINDOW_POSITION_FORGET_ON_EXIT,
    WINDOW_POSITION_SAVE_ON_EXIT,
} WindowPosition;

typedef enum DNDLook {
    DND_LOOK_ALWAYS_SOLID,
    DND_LOOK_GHOSTED_ON_BOX,
    DND_LOOK_GHOSTED_OUTSIDE_BOX,
    DND_LOOK_ALWAYS_GHOSTED,
} DNDLook;

typedef enum ScrollbarLook {
    SB_LOOK_TOP,
    SB_LOOK_MIDDLE,
    SB_LOOK_BOTTOM,
} ScrollbarLook;

typedef enum BalancingLook {
    BALANCING_SHOW_FRAMES,
    BALANCING_SHOW_OBJECTS,
} BalancingLook;

typedef enum ActiveObjectLook {
    ACTIVE_OBJECT_LOOK_FRAME,
    ACTIVE_OBJECT_LOOK_CORNER,
} ActiveObjectLook;

/*
 * User Prefs for interface drawing
 */
struct ZunePrefs {
    /* all frames */
    struct MUI_FrameSpec frames[MUIV_Frame_Count];
    /* Images */
    struct MUI_ImageSpec *images[MUII_Count];
    /* Pens */
//    GdkColor *muipens;
    /* Fonts */
    struct TextFont *fonts[-MUIV_Font_NegCount];
    /* Buttons */
    STRPTR   textbutton_font;
    WORD     radiobutton_hspacing;
    WORD     radiobutton_vspacing;
    /* Cycles */
    CycleMenuPosition cycle_menu_position;
    WORD     cycle_menu_min_entries;
    WORD     cycle_menu_speed;
    BOOL     cycle_menu_recessed_entries;
    /* Groups */
    GroupTitlePosition group_title_position;
    GroupTitleColor    group_title_color;
    STRPTR   group_title_font;
    WORD     group_hspacing;
    WORD     group_vspacing;
    /* Strings */
    struct MUI_PenSpec string_bg_active;
    struct MUI_PenSpec string_text_active;
    struct MUI_PenSpec string_bg_inactive;
    struct MUI_PenSpec string_text_inactive;
    /* Lists */
    STRPTR   list_font_normal;
    STRPTR   list_font_fixed;
    WORD     list_linespacing;
    /* Navigation */
    BOOL                 dragndrop_left_button;
    STRPTR               dragndrop_left_modifier;
    BOOL                 dragndrop_middle_button;
    STRPTR               dragndrop_middle_modifier;
    LONG                 dragndrop_autostart;
    DNDLook              dragndrop_look;
    BalancingLook        balancing_look;
    ActiveObjectLook     active_object_look;
    struct MUI_PenSpec   active_object_color;

//    ZuneKeySpec muikeys[MUIKEY_COUNT];

    /* Scrollbars */
    ScrollbarLook     sb_look;
    /* Sliders */
    STRPTR               slider_knob_font;
    /* Special */
    /* Windows */
    WindowPosition  window_position;

    STRPTR   window_font_normal;
    STRPTR   window_font_small;
    STRPTR   window_font_big;

    WORD     window_inner_left;
    WORD     window_inner_right;
    WORD     window_inner_top;
    WORD     window_inner_bottom;

    ULONG    app_cfg_spy_delay;

#if 0
    ZStringSet *comments;
#endif
};

extern struct ZunePrefs __zprefs;
//extern GdkColor __mpens[];

void __zune_prefs_init    (struct ZunePrefs *prefs);
void __zune_prefs_release (struct ZunePrefs *prefs);

int __zune_prefs_sys_global_read(struct ZunePrefs *prefs);
int __zune_prefs_sys_app_read(struct ZunePrefs *prefs, STRPTR app_title);
int __zune_prefs_user_global_read(struct ZunePrefs *prefs);
int __zune_prefs_user_app_read(struct ZunePrefs *prefs, STRPTR app_title);
int __zune_prefs_user_global_write(struct ZunePrefs *prefs);
int __zune_prefs_user_app_write(struct ZunePrefs *prefs, STRPTR app_title);
int __zune_prefs_user_global_write_current (void);

int __zune_prefs_read(struct ZunePrefs *prefs, STRPTR path);
int __zune_prefs_write(struct ZunePrefs *prefs, STRPTR path);

#ifndef _AROS
//BOOL __zune_prefs_spy (struct MUI_ApplicationData *data);
#endif

#endif


