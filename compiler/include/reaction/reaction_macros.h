/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - Convenience macros for ReAction/ClassAct programming
*/

#ifndef REACTION_REACTION_MACROS_H
#define REACTION_REACTION_MACROS_H

/****************************************************************************
 *  Convenience macros for building BOOPSI object trees inline.
 *
 *  Each FooObject macro begins an object creation via NewObject().
 *  Nest them as tag values to build hierarchical layouts. Terminate
 *  each object with the matching End macro. Use a comma after nested
 *  objects (to continue the parent's taglist) and a semicolon for the
 *  outermost object.
 *
 *  Example:
 *
 *   layout = LayoutObject,
 *                LAYOUT_AddChild,
 *                    ButtonObject, GA_ID, 1, GA_Text, "OK", ButtonEnd,
 *                LAYOUT_AddChild,
 *                    ButtonObject, GA_ID, 2, GA_Text, "Cancel", ButtonEnd,
 *            LayoutEnd;
 */

/*
 * Pull in class tag definitions so the macros are self-contained.
 * These headers only define tag constants and macros — no heavy deps.
 */
#ifndef GADGETS_LAYOUT_H
#include <gadgets/layout.h>
#endif
#ifndef GADGETS_BUTTON_H
#include <gadgets/button.h>
#endif
#ifndef GADGETS_CHECKBOX_H
#include <gadgets/checkbox.h>
#endif
#ifndef GADGETS_CHOOSER_H
#include <gadgets/chooser.h>
#endif
#ifndef GADGETS_CLICKTAB_H
#include <gadgets/clicktab.h>
#endif
#ifndef GADGETS_FUELGAUGE_H
#include <gadgets/fuelgauge.h>
#endif
#ifndef GADGETS_GETFILE_H
#include <gadgets/getfile.h>
#endif
#ifndef GADGETS_GETFONT_H
#include <gadgets/getfont.h>
#endif
#ifndef GADGETS_GETSCREENMODE_H
#include <gadgets/getscreenmode.h>
#endif
#ifndef GADGETS_INTEGER_H
#include <gadgets/integer.h>
#endif
#ifndef GADGETS_LISTBROWSER_H
#include <gadgets/listbrowser.h>
#endif
#ifndef GADGETS_PALETTE_H
#include <gadgets/palette.h>
#endif
#ifndef GADGETS_RADIOBUTTON_H
#include <gadgets/radiobutton.h>
#endif
#ifndef GADGETS_SCROLLER_H
#include <gadgets/scroller.h>
#endif
#ifndef GADGETS_SLIDER_H
#include <gadgets/slider.h>
#endif
#ifndef GADGETS_SPACE_H
#include <gadgets/space.h>
#endif
#ifndef GADGETS_SPEEDBAR_H
#include <gadgets/speedbar.h>
#endif
#ifndef GADGETS_STRING_H
#include <gadgets/string.h>
#endif
#ifndef GADGETS_TEXTFIELD_H
#include <gadgets/textfield.h>
#endif
#ifndef IMAGES_BEVEL_H
#include <images/bevel.h>
#endif
#ifndef IMAGES_BITMAP_H
#include <images/bitmap.h>
#endif
#ifndef IMAGES_DRAWLIST_H
#include <images/drawlist.h>
#endif
#ifndef IMAGES_GLYPH_H
#include <images/glyph.h>
#endif
#ifndef IMAGES_LABEL_H
#include <images/label.h>
#endif
#ifndef IMAGES_PENMAP_H
#include <images/penmap.h>
#endif
#ifndef CLASSES_WINDOW_H
#include <classes/window.h>
#endif
#ifndef CLASSES_REQUESTER_H
#include <classes/requester.h>
#endif

/****************************************************************************
 * Generic End macro — closes any *Object macro's argument list
 */
#ifndef End
#define End                 TAG_END)
#endif

/****************************************************************************
 * Gadget Object Creation Macros
 *
 * These use string-based class names so no proto/*.h includes are needed.
 * The individual class headers define the same macros; #ifndef guards
 * prevent conflicts regardless of include order.
 */

#ifndef ButtonObject
#define ButtonObject        NewObject(NULL, "button.gadget"
#endif

#ifndef ToggleObject
#define ToggleObject        NewObject(NULL, "button.gadget",\
                                       GA_ToggleSelect, TRUE
#endif

#ifndef CheckBoxObject
#define CheckBoxObject      NewObject(NULL, "checkbox.gadget"
#endif

#ifndef ChooserObject
#define ChooserObject       NewObject(NULL, "chooser.gadget"
#endif

#ifndef ClickTabObject
#define ClickTabObject      NewObject(NULL, "clicktab.gadget"
#endif
#ifndef ClickTabsObject
#define ClickTabsObject     ClickTabObject
#endif

#ifndef PopUpObject
#define PopUpObject         NewObject(NULL, "chooser.gadget",\
                                       CHOOSER_PopUp, TRUE
#endif
#ifndef DropDownObject
#define DropDownObject      NewObject(NULL, "chooser.gadget",\
                                       CHOOSER_DropDown, TRUE
#endif

#ifndef FuelGaugeObject
#define FuelGaugeObject     NewObject(NULL, "fuelgauge.gadget"
#endif
#ifndef FuelObject
#define FuelObject          FuelGaugeObject
#endif

#ifndef GetFileObject
#define GetFileObject       NewObject(NULL, "getfile.gadget"
#endif

#ifndef GetFontObject
#define GetFontObject       NewObject(NULL, "getfont.gadget"
#endif

#ifndef GetScreenModeObject
#define GetScreenModeObject NewObject(NULL, "getscreenmode.gadget"
#endif

#ifndef IntegerObject
#define IntegerObject       NewObject(NULL, "integer.gadget"
#endif

#ifndef PaletteObject
#define PaletteObject       NewObject(NULL, "palette.gadget"
#endif

#ifndef PageObject
#define PageObject          NewObject(NULL, "page.gadget"
#endif

#ifndef PenMapObject
#define PenMapObject        NewObject(NULL, "penmap.image"
#endif

#ifndef LayoutObject
#define LayoutObject        NewObject(NULL, "layout.gadget"
#endif

#ifndef VLayoutObject
#define VLayoutObject       NewObject(NULL, "layout.gadget", LAYOUT_Orientation, LAYOUT_ORIENT_VERT
#endif

#ifndef HLayoutObject
#define HLayoutObject       NewObject(NULL, "layout.gadget"
#endif
#ifndef VGroupObject
#define VGroupObject        VLayoutObject
#endif
#ifndef HGroupObject
#define HGroupObject        HLayoutObject
#endif

#ifndef ListBrowserObject
#define ListBrowserObject   NewObject(NULL, "listbrowser.gadget"
#endif

#ifndef RadioButtonObject
#define RadioButtonObject   NewObject(NULL, "radiobutton.gadget"
#endif
#ifndef MxObject
#define MxObject            RadioButtonObject
#endif

#ifndef ScrollerObject
#define ScrollerObject      NewObject(NULL, "scroller.gadget"
#endif

#ifndef SpeedBarObject
#define SpeedBarObject      NewObject(NULL, "speedbar.gadget"
#endif

#ifndef SliderObject
#define SliderObject        NewObject(NULL, "slider.gadget"
#endif

#ifndef StringObject
#define StringObject        NewObject(NULL, "string.gadget"
#endif

#ifndef SpaceObject
#define SpaceObject         NewObject(NULL, "space.gadget"
#endif

#ifndef TextFieldObject
#define TextFieldObject     NewObject(NULL, "textfield.gadget"
#endif

/****************************************************************************
 * Image Object Creation Macros
 */
#ifndef BevelObject
#define BevelObject         NewObject(NULL, "bevel.image"
#endif

#ifndef BitMapObject
#define BitMapObject        NewObject(NULL, "bitmap.image"
#endif

#ifndef DrawListObject
#define DrawListObject      NewObject(NULL, "drawlist.image"
#endif

#ifndef GlyphObject
#define GlyphObject         NewObject(NULL, "glyph.image"
#endif

#ifndef LabelObject
#define LabelObject         NewObject(NULL, "label.image"
#endif

/****************************************************************************
 * Class Object Creation Macros
 */
#ifndef WindowObject
#define WindowObject        NewObject(NULL, "window.class"
#endif

#ifndef RequesterObject
#define RequesterObject     NewObject(NULL, "requester.class"
#endif

/****************************************************************************
 * Window class method macros
 */
#ifndef CLASSES_WINDOW_H
/* Only define if window.h wasn't already included (it has the WM_* values) */
#define WM_HANDLEINPUT  (0x570001L)
#define WM_OPEN         (0x570002L)
#define WM_CLOSE        (0x570003L)
#define WM_ICONIFY      (0x570005L)
#endif

#ifndef RA_OpenWindow
#define RA_OpenWindow(win)       (struct Window *)DoMethod(win, WM_OPEN, NULL)
#endif
#ifndef RA_CloseWindow
#define RA_CloseWindow(win)      DoMethod(win, WM_CLOSE, NULL)
#endif
#ifndef RA_HandleInput
#define RA_HandleInput(win,code) DoMethod(win, WM_HANDLEINPUT, code)
#endif
#ifndef RA_Iconify
#define RA_Iconify(win)          (BOOL)DoMethod(win, WM_ICONIFY, NULL)
#endif
#ifndef RA_Uniconify
#define RA_Uniconify(win)        RA_OpenWindow(win)
#endif

#ifndef RA_SetUpHook
#define RA_SetUpHook(apphook,func,data) {                     \
                                        apphook.h_Entry = (HOOKFUNC)func; \
                                        apphook.h_SubEntry = NULL;        \
                                        apphook.h_Data = (APTR)data; }
#endif

/****************************************************************************
 * Additional BOOPSI Classes (not part of core Reaction)
 */
#ifndef ColorWheelObject
#define ColorWheelObject    NewObject(NULL, "colorwheel.gadget"
#endif
#ifndef GradientObject
#define GradientObject      NewObject(NULL, "gradientslider.gadget"
#endif
#ifndef LedObject
#define LedObject           NewObject(NULL, "led.image"
#endif

/****************************************************************************
 * End macro synonyms — named after their matching Object macro
 */
#ifndef WindowEnd
#define WindowEnd           End
#endif

#define BitMapEnd           End
#define ButtonEnd           End
#define CheckBoxEnd         End
#define ChooserEnd          End
#define ClickTabEnd         End
#define ClickTabsEnd        End
#define FuelGaugeEnd        End
#define IntegerEnd          End
#define PaletteEnd          End
#define LayoutEnd           End
#define ListBrowserEnd      End
#define PageEnd             End
#define RadioButtonEnd      End
#define ScrollerEnd         End
#define SpeedBarEnd         End
#define SliderEnd           End
#define StringEnd           End
#define SpaceEnd            End
#define TextFieldEnd        End

#define BevelEnd            End
#define DrawListEnd         End
#define GlyphEnd            End
#define LabelEnd            End

#define ColorWheelEnd       End
#define GradientSliderEnd   End
#define LedEnd              End

/****************************************************************************
 * Vector Glyph Image Aliases
 */
#define GetPath             GLYPH_POPDRAWER
#define GetFile             GLYPH_POPFILE
#define GetScreen           GLYPH_POPSCREENMODE
#define GetTime             GLYPH_POPTIME
#define CheckMark           GLYPH_CHECKMARK
#define PopUp               GLYPH_POPUP
#define DropDown            GLYPH_DROPDOWN
#define ArrowUp             GLYPH_ARROWUP
#define ArrowDown           GLYPH_ARROWDOWN
#define ArrowLeft           GLYPH_ARROWLEFT
#define ArrowRight          GLYPH_ARROWRIGHT

/****************************************************************************
 * Bevel Frame Type Aliases
 */
#define ThinFrame           BVS_THIN
#define ButtonFrame         BVS_BUTTON
#define StandardFrame       BVS_STANDARD
#define RidgeFrame          BVS_FIELD
#define StringFrame         BVS_FIELD
#define GroupFrame          BVS_GROUP
#define DropBoxFrame        BVS_DROPBOX
#define HBarFrame           BVS_SBAR_HORIZ
#define VBarFrame           BVS_SBAR_VERT
#define RadioFrame          BVS_RADIOBUTTON
#define MxFrame             BVS_RADIOBUTTON

/****************************************************************************
 * Shorthand gadget macros
 */
#define Label(text)        CHILD_Label, LabelObject, LABEL_Text, text, End
#define Button(text,id)    ButtonObject, GA_Text, text, GA_ID, id, GA_RelVerify, TRUE, End
#define PushButton(text,id) ButtonObject, GA_Text, text, GA_ID, id, GA_RelVerify, TRUE, BUTTON_PushButton, TRUE, End
#define TextLine(text)     ButtonObject, GA_Text, text, GA_ReadOnly, TRUE, End
#define LabelTextLine(text,label) TextLine(text), Label(label)
#define String(text,id,maxchars)   StringObject, STRINGA_TextVal, text, STRINGA_MaxChars, maxchars, GA_ID, id, GA_RelVerify, TRUE, GA_TabCycle, TRUE, End
#define LabelString(text,id,maxchars,label) String(text,id,maxchars), Label(label)
#define PopString(text,id,maxchars,image) LAYOUT_AddChild, HLayoutObject, String(text,0,maxchars), ButtonObject, BUTTON_AutoButton, image, GA_RelVerify, TRUE, GA_ID, id, End, End

/****************************************************************************
 * BGUI-style Layout Group Macros
 */
#define StartMember         LAYOUT_AddChild
#define StartImage          LAYOUT_AddImage
#define StartHLayout        LAYOUT_AddChild, HLayoutObject
#define StartVLayout        LAYOUT_AddChild, VLayoutObject
#define StartHGroup         StartHLayout
#define StartVGroup         StartVLayout
#ifndef EndWindow
#define EndWindow           End
#endif
#define EndMember           End
#define EndImage            End
#ifndef EndObject
#define EndObject           End
#endif
#define EndHGroup           End
#define EndVGroup           End
#define EndGroup            End

/****************************************************************************
 * BGUI-inspired alignment and spacing shorthands
 */
#define HAlign(p)           LAYOUT_HorizAlignment, p
#define VAlign(p)           LAYOUT_VertAlignment, p
#define Spacing(p)          LAYOUT_InnerSpacing, p
#define LOffset(p)          LAYOUT_LeftSpacing, p
#define ROffset(p)          LAYOUT_RightSpacing, p
#define TOffset(p)          LAYOUT_TopSpacing, p
#define BOffset(p)          LAYOUT_BottomSpacing, p

#define VCentered           LAYOUT_VertAlignment, LALIGN_CENTER
#define TAligned            LAYOUT_VertAlignment, LALIGN_TOP
#define BAligned            LAYOUT_VertAlignment, LALIGN_BOTTOM
#define HCentered           LAYOUT_HorizAlignment, LALIGN_CENTER
#define LAligned            LAYOUT_HorizAlignment, LALIGN_LEFT
#define RAligned            LAYOUT_HorizAlignment, LALIGN_RIGHT
#define Offset(x1,y1,x2,y2) LAYOUT_LeftSpacing, x1, LAYOUT_TopSpacing, y1, LAYOUT_RightSpacing, x2, LAYOUT_BottomSpacing, y2
#define EvenSized           LAYOUT_EvenSize, TRUE
#define MemberLabel(a)      CHILD_Label, LabelObject, LABEL_Text, a, LabelEnd

/****************************************************************************
 * Easy Menu Macros
 */
#define Title(t)\
        { NM_TITLE, t, NULL, 0, 0, NULL }
#define Item(t,s,i)\
        { NM_ITEM, t, s, 0, 0, (APTR)i }
#define ItemBar\
        { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL }
#define SubItem(t,s,i)\
        { NM_SUB, t, s, 0, 0, (APTR)i }
#define SubBar\
        { NM_SUB, NM_BARLABEL, NULL, 0, 0, NULL }
#define EndMenu\
        { NM_END, NULL, NULL, 0, 0, NULL }

#endif /* REACTION_REACTION_MACROS_H */
