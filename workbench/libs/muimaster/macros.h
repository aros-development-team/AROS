/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/


/* This file contains the macros which orgignial MUI has provied + some extra one */

#ifndef _MUI_MACROS_H
#define _MUI_MACROS_H

/* Some nice macrodefinitions for creating your object tree */
#define MenustripObject     MUI_NewObject(MUIC_Menustrip
#define MenuObject          MUI_NewObject(MUIC_Menu
#define MenuObjectT(name)   MUI_NewObject(MUIC_Menu, MUIA_Menu_Title, name
#define MenuitemObject      MUI_NewObject(MUIC_Menuitem
#define WindowObject        MUI_NewObject(MUIC_Window
#define ImageObject         MUI_NewObject(MUIC_Image
#define BitmapObject        MUI_NewObject(MUIC_Bitmap
#define BodychunkObject     MUI_NewObject(MUIC_Bodychunk
#define NotifyObject        MUI_NewObject(MUIC_Notify
#define ApplicationObject   MUI_NewObject(MUIC_Application
#define TextObject          MUI_NewObject(MUIC_Text
#define RectangleObject     MUI_NewObject(MUIC_Rectangle
#define BalanceObject       MUI_NewObject(MUIC_Balance
#define ListObject          MUI_NewObject(MUIC_List
#define PropObject          MUI_NewObject(MUIC_Prop
#define StringObject        MUI_NewObject(MUIC_String
#define ScrollbarObject     MUI_NewObject(MUIC_Scrollbar
#define ListviewObject      MUI_NewObject(MUIC_Listview
#define RadioObject         MUI_NewObject(MUIC_Radio
#define VolumelistObject    MUI_NewObject(MUIC_Volumelist
#define FloattextObject     MUI_NewObject(MUIC_Floattext
#define DirlistObject       MUI_NewObject(MUIC_Dirlist
#define CycleObject         MUI_NewObject(MUIC_Cycle
#define GaugeObject         MUI_NewObject(MUIC_Gauge
#define ScaleObject         MUI_NewObject(MUIC_Scale
#define NumericObject       MUI_NewObject(MUIC_Numeric
#define SliderObject        MUI_NewObject(MUIC_Slider
#define NumericbuttonObject MUI_NewObject(MUIC_Numericbutton
#define KnobObject          MUI_NewObject(MUIC_Knob
#define LevelmeterObject    MUI_NewObject(MUIC_Levelmeter
#define BoopsiObject        MUI_NewObject(MUIC_Boopsi
#define ColorfieldObject    MUI_NewObject(MUIC_Colorfield
#define PenadjustObject     MUI_NewObject(MUIC_Penadjust
#define ColoradjustObject   MUI_NewObject(MUIC_Coloradjust
#define PaletteObject       MUI_NewObject(MUIC_Palette
#define GroupObject         MUI_NewObject(MUIC_Group
#define RegisterObject      MUI_NewObject(MUIC_Register
#define VirtgroupObject     MUI_NewObject(MUIC_Virtgroup
#define ScrollgroupObject   MUI_NewObject(MUIC_Scrollgroup
#define PopstringObject     MUI_NewObject(MUIC_Popstring
#define PopobjectObject     MUI_NewObject(MUIC_Popobject
#define PoplistObject       MUI_NewObject(MUIC_Poplist
#define PopaslObject        MUI_NewObject(MUIC_Popasl
#define PendisplayObject    MUI_NewObject(MUIC_Pendisplay
#define PoppenObject        MUI_NewObject(MUIC_Poppen
#define AboutmuiObject      MUI_NewObject(MUIC_Aboutmui
#define ScrmodelistObject   MUI_NewObject(MUIC_Scrmodelist
#define KeyentryObject      MUI_NewObject(MUIC_Keyentry
#define VGroup              MUI_NewObject(MUIC_Group
#define HGroup              MUI_NewObject(MUIC_Group, MUIA_Group_Horiz, TRUE
#define ColGroup(columns)   MUI_NewObject(MUIC_Group, MUIA_Group_Columns, (columns)
#define RowGroup(rows)      MUI_NewObject(MUIC_Group, MUIA_Group_Rows   , (rows)
#define PageGroup           MUI_NewObject(MUIC_Group, MUIA_Group_PageMode, TRUE
#define VGroupV             MUI_NewObject(MUIC_Virtgroup
#define HGroupV             MUI_NewObject(MUIC_Virtgroup, MUIA_Group_Horiz, TRUE
#define ColGroupV(columns)  MUI_NewObject(MUIC_Virtgroup, MUIA_Group_Columns, (columns)
#define RowGroupV(rows)     MUI_NewObject(MUIC_Virtgroup, MUIA_Group_Rows   , (rows)
#define PageGroupV          MUI_NewObject(MUIC_Virtgroup, MUIA_Group_PageMode, TRUE
#define RegisterGroup(ts)   MUI_NewObject(MUIC_Register, MUIA_Register_Titles, (ts)
#define End                 TAG_DONE)

#define Child               MUIA_Group_Child
#define SubWindow           MUIA_Application_Window
#define WindowContents      MUIA_Window_RootObject


/**************************************************************************
 Zune/MUI's differnt frame types. Use one per object
**************************************************************************/
#define NoFrame          MUIA_Frame, MUIV_Frame_None
#define ButtonFrame      MUIA_Frame, MUIV_Frame_Button
#define ImageButtonFrame MUIA_Frame, MUIV_Frame_ImageButton
#define TextFrame        MUIA_Frame, MUIV_Frame_Text
#define StringFrame      MUIA_Frame, MUIV_Frame_String
#define ReadListFrame    MUIA_Frame, MUIV_Frame_ReadList
#define InputListFrame   MUIA_Frame, MUIV_Frame_InputList
#define PropFrame        MUIA_Frame, MUIV_Frame_Prop
#define SliderFrame      MUIA_Frame, MUIV_Frame_Slider
#define GaugeFrame       MUIA_Frame, MUIV_Frame_Gauge
#define VirtualFrame     MUIA_Frame, MUIV_Frame_Virtual
#define GroupFrame       MUIA_Frame, MUIV_Frame_Group
#define GroupFrameT(t)   MUIA_Frame, MUIV_Frame_Group, MUIA_FrameTitle, t, MUIA_Background, MUII_GroupBack


/**************************************************************************
 Space objects
**************************************************************************/
#define HVSpace           MUI_NewObject(MUIC_Rectangle,TAG_DONE)
#define HSpace(x)         MUI_MakeObject(MUIO_HSpace,x)
#define VSpace(x)         MUI_MakeObject(MUIO_VSpace,x)
#define HCenter(obj)      (HGroup, GroupSpacing(0), Child, HSpace(0), Child, (obj), Child, HSpace(0), End)
#define VCenter(obj)      (VGroup, GroupSpacing(0), Child, VSpace(0), Child, (obj), Child, VSpace(0), End)
#define InnerSpacing(h,v) MUIA_InnerLeft,(h),MUIA_InnerRight,(h),MUIA_InnerTop,(v),MUIA_InnerBottom,(v)
#define GroupSpacing(x)   MUIA_Group_Spacing,x

#ifdef MUI_OBSOLETE
/**************************************************************************
 These macros will create a simple string gadget. Don't use this in
 new code. Use MUI_MakeObject() instead.
**************************************************************************/
#define String(contents,maxlen)\
    StringObject,\
	StringFrame,\
	MUIA_String_MaxLen  , maxlen,\
	MUIA_String_Contents, contents,\
	End

#define KeyString(contents,maxlen,controlchar)\
    StringObject,\
	StringFrame,\
	MUIA_ControlChar    , controlchar,\
	MUIA_String_MaxLen  , maxlen,\
	MUIA_String_Contents, contents,\
	End

#endif

#ifdef MUI_OBSOLETE
/**************************************************************************
 These macros will create a simple checkmark gadget. Don't use this in
 new code. Use MUI_MakeObject() instead.
**************************************************************************/
#define CheckMark(sel) ImageObject, ImageButtonFrame, MUIA_InputMode, MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark, MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack, MUIA_ShowSelState, FALSE, MUIA_Selected, sel, End
#define KeyCheckMark(sel,ctrl) ImageObject, ImageButtonFrame, MUIA_InputMode, MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark, MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack, MUIA_ShowSelState, FALSE, MUIA_Selected, sel, MUIA_ControlChar, ctrl, End
#endif


/**************************************************************************
 These macros will create a simple button. It's simply calling
 MUI_MakeObject()
**************************************************************************/
#define SimpleButton(label) MUI_MakeObject(MUIO_Button,label)

#ifdef MUI_OBSOLETE
/**************************************************************************
 A Keybutton macro. The key should be in lower case.
 Don't use this in new code. Use MUI_MakeObject() instead.
**************************************************************************/
#define KeyButton(name,key) TextObject, ButtonFrame, MUIA_Font, MUIV_Font_Button, MUIA_Text_Contents, name, MUIA_Text_PreParse, "\33c", MUIA_Text_HiChar, key, MUIA_ControlChar, key, MUIA_InputMode, MUIV_InputMode_RelVerify, MUIA_Background, MUII_ButtonBack, End
#endif


#ifdef MUI_OBSOLETE
/**************************************************************************
 Obsolette Cycle macros
**************************************************************************/
#define Cycle(ent)        CycleObject, MUIA_Font, MUIV_Font_Button, MUIA_Cycle_Entries, ent, End
#define KeyCycle(ent,key) CycleObject, MUIA_Font, MUIV_Font_Button, MUIA_Cycle_Entries, ent, MUIA_ControlChar, key, End

/**************************************************************************
 Obsolette Radios macros
**************************************************************************/
#define Radio(name,array) RadioObject, GroupFrameT(name), MUIA_Radio_Entries,array, End
#define KeyRadio(name,array,key) RadioObject, GroupFrameT(name), MUIA_Radio_Entries,array, MUIA_ControlChar, key, End

/**************************************************************************
 Obsolette Slider macros
**************************************************************************/
#define Slider(min,max,level) SliderObject, MUIA_Numeric_Min, min, MUIA_Numeric_Max, max, MUIA_Numeric_Value, level, End
#define KeySlider(min,max,level,key) SliderObject, MUIA_Numeric_Min, min, MUIA_Numeric_Max, max, MUIA_Numeric_Value, level, MUIA_ControlChar, key, End
#endif



/**************************************************************************
 Use this for getting a pop button
**************************************************************************/
#define PopButton(img) MUI_MakeObject(MUIO_PopButton,img)


/**************************************************************************
 Macros for Labelobjects
 Use them for example in a group containing 2 columns, in the first
 columns the label and in the second columns the object.

 These objects should be uses because the user might have set strange
 values.

 xxxLabel() is suited for Objects without frame
 xxxLabel1() is suited for objects with a single frame, like buttons
 xxxLabel2() is suited for objects with with double frames, like string gadgets
**************************************************************************/

/* Right aligned */
#define Label(label)   MUI_MakeObject(MUIO_Label,label,0)
#define Label1(label)  MUI_MakeObject(MUIO_Label,label, MUIO_Label_SingleFrame)
#define Label2(label)  MUI_MakeObject(MUIO_Label,label, MUIO_Label_DoubleFrame)

/* Left aligned */
#define LLabel(label)  MUI_MakeObject(MUIO_Label, label, MUIO_Label_LeftAligned)
#define LLabel1(label) MUI_MakeObject(MUIO_Label, label, MUIO_Label_LeftAligned | MUIO_Label_SingleFrame)
#define LLabel2(label) MUI_MakeObject(MUIO_Label, label, MUIO_Label_LeftAligned | MUIO_Label_DoubleFrame)

/* Centered */
#define CLabel(label)  MUI_MakeObject(MUIO_Label, label, MUIO_Label_Centered)
#define CLabel1(label) MUI_MakeObject(MUIO_Label, label, MUIO_Label_Centered | MUIO_Label_SingleFrame)
#define CLabel2(label) MUI_MakeObject(MUIO_Label, label, MUIO_Label_Centered | MUIO_Label_DoubleFrame)

/* Freevert - Right aligned */
#define FreeLabel(label)   MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert)
#define FreeLabel1(label)  MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_SingleFrame)
#define FreeLabel2(label)  MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_DoubleFrame)

/* Freevert - Left aligned */
#define FreeLLabel(label)  MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_LeftAligned)
#define FreeLLabel1(label) MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_LeftAligned | MUIO_Label_SingleFrame)
#define FreeLLabel2(label) MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_LeftAligned | MUIO_Label_DoubleFrame)

/* Freevert - Centered */
#define FreeCLabel(label)  MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_Centered)
#define FreeCLabel1(label) MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_Centered | MUIO_Label_SingleFrame)
#define FreeCLabel2(label) MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_Centered | MUIO_Label_DoubleFrame)

/* The same as above + keys */
#define KeyLabel(label,key)   MUI_MakeObject(MUIO_Label, label, key)
#define KeyLabel1(label,key)  MUI_MakeObject(MUIO_Label, label, MUIO_Label_SingleFrame | (key))
#define KeyLabel2(label,key)  MUI_MakeObject(MUIO_Label, label, MUIO_Label_DoubleFrame | (key))
#define KeyLLabel(label,key)  MUI_MakeObject(MUIO_Label, label, MUIO_Label_LeftAligned | (key))
#define KeyLLabel1(label,key) MUI_MakeObject(MUIO_Label, label, MUIO_Label_LeftAligned | MUIO_Label_SingleFrame|(key))
#define KeyLLabel2(label,key) MUI_MakeObject(MUIO_Label, label, MUIO_Label_LeftAligned | MUIO_Label_DoubleFrame|(key))
#define KeyCLabel(label,key)  MUI_MakeObject(MUIO_Label, label, MUIO_Label_Centered | (key))
#define KeyCLabel1(label,key) MUI_MakeObject(MUIO_Label, label, MUIO_Label_Centered | MUIO_Label_SingleFrame|(key))
#define KeyCLabel2(label,key) MUI_MakeObject(MUIO_Label, label, MUIO_Label_Centered | MUIO_Label_DoubleFrame|(key))

#define FreeKeyLabel(label,key)   MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | (key))
#define FreeKeyLabel1(label,key)  MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_SingleFrame | (key))
#define FreeKeyLabel2(label,key)  MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_DoubleFrame | (key))
#define FreeKeyLLabel(label,key)  MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_LeftAligned | (key))
#define FreeKeyLLabel1(label,key) MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_LeftAligned | MUIO_Label_SingleFrame | (key))
#define FreeKeyLLabel2(label,key) MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_LeftAligned | MUIO_Label_DoubleFrame | (key))
#define FreeKeyCLabel(label,key)  MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_Centered | (key))
#define FreeKeyCLabel1(label,key) MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_Centered | MUIO_Label_SingleFrame | (key))
#define FreeKeyCLabel2(label,key) MUI_MakeObject(MUIO_Label, label, MUIO_Label_FreeVert | MUIO_Label_Centered | MUIO_Label_DoubleFrame | (key))


/* Some macros */
#define get(obj,attr,store) GetAttr(attr,obj,(IPTR *)store)
#define set(obj,attr,value) SetAttrs(obj,attr,value,TAG_DONE)
#define nnset(obj,attr,value) SetAttrs(obj,MUIA_NoNotify,TRUE,attr,value,TAG_DONE)

/* Zune */
#define nfset(obj,attr,value) SetAttrs(obj,MUIA_Group_Forward,FALSE,attr,value,TAG_DONE)
/* Zune */
#define nnfset(obj,attr,value) SetAttrs(obj,MUIA_Group_Forward,FALSE,MUIA_NoNotify,TRUE,attr,value,TAG_DONE)

#define setmutex(obj,n)     set(obj,MUIA_Radio_Active,n)
#define setcycle(obj,n)     set(obj,MUIA_Cycle_Active,n)
#define setstring(obj,s)    set(obj,MUIA_String_Contents,s)
#define setcheckmark(obj,b) set(obj,MUIA_Selected,b)
#define setslider(obj,l)    set(obj,MUIA_Numeric_Value,l)

/* We need the notify and area Instace Data at least here, but this stuff should be placed at the button anywhy */
#ifndef _MUI_CLASSES_NOTIFY_H
#include "classes/notify.h"
#endif

#ifndef _MUI_CLASSES_AREA_H
#include "classes/area.h"
#endif

struct __dummyAreaData__
{
    struct MUI_NotifyData mnd;
    struct MUI_AreaData   mad;
};

#define muiNotifyData(obj) (&(((struct __dummyAreaData__ *)(obj))->mnd))
#define muiAreaData(obj)   (&(((struct __dummyAreaData__ *)(obj))->mad))

#define muiGlobalInfo(obj) (((struct __dummyAreaData__ *)(obj))->mnd.mnd_GlobalInfo)
#define muiUserData(obj)   (((struct __dummyAreaData__ *)(obj))->mnd.mnd_UserData)
#define muiRenderInfo(obj) (((struct __dummyAreaData__ *)(obj))->mad.mad_RenderInfo)


/* the following macros are only valid inbetween MUIM_Setup and MUIM_Cleanuü */
#define _app(obj)          (muiGlobalInfo(obj)->mgi_ApplicationObject)
#define _win(obj)          (muiRenderInfo(obj)->mri_WindowObject)
#define _dri(obj)          (muiRenderInfo(obj)->mri_DrawInfo)
#define _screen(obj)       (muiRenderInfo(obj)->mri_Screen)
#define _pens(obj)         (muiRenderInfo(obj)->mri_Pens)
#define _font(obj)         (muiAreaData(obj)->mad_Font)

/* the following macros are only valid during MUIM_Draw */
#define _left(obj)         (muiAreaData(obj)->mad_Box.Left)
#define _top(obj)          (muiAreaData(obj)->mad_Box.Top)
#define _width(obj)        (muiAreaData(obj)->mad_Box.Width)
#define _height(obj)       (muiAreaData(obj)->mad_Box.Height)
#define _right(obj)        (_left(obj) + _width(obj) - 1)
#define _bottom(obj)       (_top(obj) + _height(obj) - 1)
#define _addleft(obj)      (muiAreaData(obj)->mad_addleft  )
#define _addtop(obj)       (muiAreaData(obj)->mad_addtop   )
#define _subwidth(obj)     (muiAreaData(obj)->mad_subwidth )
#define _subheight(obj)    (muiAreaData(obj)->mad_subheight)
#define _mleft(obj)        (_left(obj) + _addleft(obj))
#define _mtop(obj)         (_top(obj) + _addtop(obj))
#define _mwidth(obj)       (_width(obj) - _subwidth(obj))
#define _mheight(obj)      (_height(obj) - _subheight(obj))
#define _mright(obj)       (_mleft(obj) + _mwidth(obj) - 1)
#define _mbottom(obj)      (_mtop(obj) + _mheight(obj) - 1)

/* the following macros are only valid inbetween MUIM_Show and MUIM_Hide */
#define _window(obj)       (muiRenderInfo(obj)->mri_Window)
#define _rp(obj)           (muiRenderInfo(obj)->mri_RastPort)
#define _minwidth(obj)     (muiAreaData(obj)->mad_MinMax.MinWidth)
#define _minheight(obj)    (muiAreaData(obj)->mad_MinMax.MinHeight)
#define _maxwidth(obj)     (muiAreaData(obj)->mad_MinMax.MaxWidth)
#define _maxheight(obj)    (muiAreaData(obj)->mad_MinMax.MaxHeight)
#define _defwidth(obj)     (muiAreaData(obj)->mad_MinMax.DefWidth)
#define _defheight(obj)    (muiAreaData(obj)->mad_MinMax.DefHeight)
#define _flags(obj)        (muiAreaData(obj)->mad_Flags)



#endif
