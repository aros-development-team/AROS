/***************************************************************************
**
** Object Generation
** -----------------
**
** The xxxObject (and xChilds) macros generate new instances of MUI classes.
** Every xxxObject can be followed by tagitems specifying initial create
** time attributes for the new object and must be terminated with the
** End macro:
**
** obj = StringObject,
**          MUIA_String_Contents, "foo",
**          MUIA_String_MaxLen  , 40,
**          End;
**
** With the Child, SubWindow and WindowContents shortcuts you can
** construct a complete GUI within one command:
**
** app = ApplicationObject,
**
**          ...
**
**          SubWindow, WindowObject,
**             WindowContents, VGroup,
**                Child, String("foo",40),
**                Child, String("bar",50),
**                Child, HGroup,
**                   Child, CheckMark(TRUE),
**                   Child, CheckMark(FALSE),
**                   End,
**                End,
**             End,
**
**          SubWindow, WindowObject,
**             WindowContents, HGroup,
**                Child, ...,
**                Child, ...,
**                End,
**             End,
**
**          ...
**
**          End;
**
***************************************************************************/

#define MenustripObject     MUI_NewObject(MUIC_Menustrip
#define MenuObject          MUI_NewObject(MUIC_Menu
#define MenuObjectT(name)   MUI_NewObject(MUIC_Menu,MUIA_Menu_Title,name
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
#define HGroup              MUI_NewObject(MUIC_Group,MUIA_Group_Horiz,TRUE
#define ColGroup(cols)      MUI_NewObject(MUIC_Group,MUIA_Group_Columns,(cols)
#define RowGroup(rows)      MUI_NewObject(MUIC_Group,MUIA_Group_Rows   ,(rows)
#define PageGroup           MUI_NewObject(MUIC_Group,MUIA_Group_PageMode,TRUE
#define VGroupV             MUI_NewObject(MUIC_Virtgroup
#define HGroupV             MUI_NewObject(MUIC_Virtgroup,MUIA_Group_Horiz,TRUE
#define ColGroupV(cols)     MUI_NewObject(MUIC_Virtgroup,MUIA_Group_Columns,(cols)
#define RowGroupV(rows)     MUI_NewObject(MUIC_Virtgroup,MUIA_Group_Rows   ,(rows)
#define PageGroupV          MUI_NewObject(MUIC_Virtgroup,MUIA_Group_PageMode,TRUE
#define RegisterGroup(t)    MUI_NewObject(MUIC_Register,MUIA_Register_Titles,(t)
#define End                 TAG_DONE)

#define Child             MUIA_Group_Child
#define SubWindow         MUIA_Application_Window
#define WindowContents    MUIA_Window_RootObject



/***************************************************************************
**
** Frame Types
** -----------
**
** These macros may be used to specify one of MUI's different frame types.
** Note that every macro consists of one { ti_Tag, ti_Data } pair.
**
** GroupFrameT() is a special kind of frame that contains a centered
** title text.
**
** HGroup, GroupFrameT("Horiz Groups"),
**    Child, RectangleObject, TextFrame  , End,
**    Child, RectangleObject, StringFrame, End,
**    Child, RectangleObject, ButtonFrame, End,
**    Child, RectangleObject, ListFrame  , End,
**    End,
**
***************************************************************************/

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
#define GroupFrameT(s)   MUIA_Frame, MUIV_Frame_Group, MUIA_FrameTitle, s, MUIA_Background, MUII_GroupBack



/***************************************************************************
**
** Spacing Macros
** --------------
**
***************************************************************************/

#define HVSpace           MUI_NewObject(MUIC_Rectangle,TAG_DONE)
#define HSpace(x)         MUI_MakeObject(MUIO_HSpace,x)
#define VSpace(x)         MUI_MakeObject(MUIO_VSpace,x)
#define HCenter(obj)      (HGroup, GroupSpacing(0), Child, HSpace(0), Child, (obj), Child, HSpace(0), End)
#define VCenter(obj)      (VGroup, GroupSpacing(0), Child, VSpace(0), Child, (obj), Child, VSpace(0), End)
#define InnerSpacing(h,v) MUIA_InnerLeft,(h),MUIA_InnerRight,(h),MUIA_InnerTop,(v),MUIA_InnerBottom,(v)
#define GroupSpacing(x)   MUIA_Group_Spacing,x



#ifdef MUI_OBSOLETE

/***************************************************************************
**
** String-Object
** -------------
**
** The following macro creates a simple string gadget.
**
***************************************************************************/

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

/***************************************************************************
**
** CheckMark-Object
** ----------------
**
** The following macro creates a checkmark gadget.
**
***************************************************************************/

#define CheckMark(selected)\
	ImageObject,\
		ImageButtonFrame,\
		MUIA_InputMode        , MUIV_InputMode_Toggle,\
		MUIA_Image_Spec       , MUII_CheckMark,\
		MUIA_Image_FreeVert   , TRUE,\
		MUIA_Selected         , selected,\
		MUIA_Background       , MUII_ButtonBack,\
		MUIA_ShowSelState     , FALSE,\
		End

#define KeyCheckMark(selected,control)\
	ImageObject,\
		ImageButtonFrame,\
		MUIA_InputMode        , MUIV_InputMode_Toggle,\
		MUIA_Image_Spec       , MUII_CheckMark,\
		MUIA_Image_FreeVert   , TRUE,\
		MUIA_Selected         , selected,\
		MUIA_Background       , MUII_ButtonBack,\
		MUIA_ShowSelState     , FALSE,\
		MUIA_ControlChar      , control,\
		End

#endif


/***************************************************************************
**
** Button-Objects
** --------------
**
** Note: Use small letters for KeyButtons, e.g.
**       KeyButton("Cancel",'c')  and not  KeyButton("Cancel",'C') !!
**
***************************************************************************/

#define SimpleButton(label) MUI_MakeObject(MUIO_Button,GPOINTER_TO_UINT(label))

#ifdef MUI_OBSOLETE

#define KeyButton(name,key)\
	TextObject,\
		ButtonFrame,\
		MUIA_Font, MUIV_Font_Button,\
		MUIA_Text_Contents, name,\
		MUIA_Text_PreParse, "\33c",\
		MUIA_Text_HiChar  , key,\
		MUIA_ControlChar  , key,\
		MUIA_InputMode    , MUIV_InputMode_RelVerify,\
		MUIA_Background   , MUII_ButtonBack,\
		End

#endif


#ifdef MUI_OBSOLETE

/***************************************************************************
**
** Cycle-Object
** ------------
**
***************************************************************************/

#define Cycle(entries)        CycleObject, MUIA_Font, MUIV_Font_Button, MUIA_Cycle_Entries, entries, End
#define KeyCycle(entries,key) CycleObject, MUIA_Font, MUIV_Font_Button, MUIA_Cycle_Entries, entries, MUIA_ControlChar, key, End



/***************************************************************************
**
** Radio-Object
** ------------
**
***************************************************************************/

#define Radio(name,array)\
	RadioObject,\
		GroupFrameT(name),\
		MUIA_Radio_Entries,array,\
		End

#define KeyRadio(name,array,key)\
	RadioObject,\
		GroupFrameT(name),\
		MUIA_Radio_Entries,array,\
		MUIA_ControlChar, key,\
		End



/***************************************************************************
**
** Slider-Object
** -------------
**
***************************************************************************/


#define Slider(min,max,level)\
	SliderObject,\
		MUIA_Numeric_Min  , min,\
		MUIA_Numeric_Max  , max,\
		MUIA_Numeric_Value, level,\
		End

#define KeySlider(min,max,level,key)\
	SliderObject,\
		MUIA_Numeric_Min  , min,\
		MUIA_Numeric_Max  , max,\
		MUIA_Numeric_Value, level,\
		MUIA_ControlChar , key,\
		End

#endif



/***************************************************************************
**
** Button to be used for popup objects
**
***************************************************************************/

#define PopButton(img) MUI_MakeObject(MUIO_PopButton,img)



/***************************************************************************
**
** Labeling Objects
** ----------------
**
** Labeling objects, e.g. a group of string gadgets,
**
**   Small: |foo   |
**  Normal: |bar   |
**     Big: |foobar|
**    Huge: |barfoo|
**
** is done using a 2 column group:
**
** ColGroup(2),
** 	Child, Label2("Small:" ),
**    Child, StringObject, End,
** 	Child, Label2("Normal:"),
**    Child, StringObject, End,
** 	Child, Label2("Big:"   ),
**    Child, StringObject, End,
** 	Child, Label2("Huge:"  ),
**    Child, StringObject, End,
**    End,
**
** Note that we have three versions of the label macro, depending on
** the frame type of the right hand object:
**
** Label1(): For use with standard frames (e.g. checkmarks).
** Label2(): For use with double high frames (e.g. string gadgets).
** Label() : For use with objects without a frame.
**
** These macros ensure that your label will look fine even if the
** user of your application configured some strange spacing values.
** If you want to use your own labeling, you'll have to pay attention
** on this topic yourself.
**
***************************************************************************/

#define Label(label)   MUI_MakeObject(MUIO_Label,label,0)
#define Label1(label)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_SingleFrame)
#define Label2(label)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_DoubleFrame)
#define LLabel(label)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_LeftAligned)
#define LLabel1(label) MUI_MakeObject(MUIO_Label,label,MUIO_Label_LeftAligned|MUIO_Label_SingleFrame)
#define LLabel2(label) MUI_MakeObject(MUIO_Label,label,MUIO_Label_LeftAligned|MUIO_Label_DoubleFrame)
#define CLabel(label)  MUI_MakeObject(MUIO_Label,_U(label),MUIO_Label_Centered)

#define CLabel1(label) MUI_MakeObject(MUIO_Label,label,MUIO_Label_Centered|MUIO_Label_SingleFrame)
#define CLabel2(label) MUI_MakeObject(MUIO_Label,label,MUIO_Label_Centered|MUIO_Label_DoubleFrame)

#define FreeLabel(label)   MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert)
#define FreeLabel1(label)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_SingleFrame)
#define FreeLabel2(label)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_DoubleFrame)
#define FreeLLabel(label)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_LeftAligned)
#define FreeLLabel1(label) MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_LeftAligned|MUIO_Label_SingleFrame)
#define FreeLLabel2(label) MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_LeftAligned|MUIO_Label_DoubleFrame)
#define FreeCLabel(label)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_Centered)
#define FreeCLabel1(label) MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_Centered|MUIO_Label_SingleFrame)
#define FreeCLabel2(label) MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_Centered|MUIO_Label_DoubleFrame)

#define KeyLabel(label,key)   MUI_MakeObject(MUIO_Label,label,key)
#define KeyLabel1(label,key)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_SingleFrame|(key))
#define KeyLabel2(label,key)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_DoubleFrame|(key))
#define KeyLLabel(label,key)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_LeftAligned|(key))
#define KeyLLabel1(label,key) MUI_MakeObject(MUIO_Label,label,MUIO_Label_LeftAligned|MUIO_Label_SingleFrame|(key))
#define KeyLLabel2(label,key) MUI_MakeObject(MUIO_Label,label,MUIO_Label_LeftAligned|MUIO_Label_DoubleFrame|(key))
#define KeyCLabel(label,key)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_Centered|(key))
#define KeyCLabel1(label,key) MUI_MakeObject(MUIO_Label,label,MUIO_Label_Centered|MUIO_Label_SingleFrame|(key))
#define KeyCLabel2(label,key) MUI_MakeObject(MUIO_Label,label,MUIO_Label_Centered|MUIO_Label_DoubleFrame|(key))

#define FreeKeyLabel(label,key)   MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|(key))
#define FreeKeyLabel1(label,key)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_SingleFrame|(key))
#define FreeKeyLabel2(label,key)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_DoubleFrame|(key))
#define FreeKeyLLabel(label,key)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_LeftAligned|(key))
#define FreeKeyLLabel1(label,key) MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_LeftAligned|MUIO_Label_SingleFrame|(key))
#define FreeKeyLLabel2(label,key) MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_LeftAligned|MUIO_Label_DoubleFrame|(key))
#define FreeKeyCLabel(label,key)  MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_Centered|(key))
#define FreeKeyCLabel1(label,key) MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_Centered|MUIO_Label_SingleFrame|(key))
#define FreeKeyCLabel2(label,key) MUI_MakeObject(MUIO_Label,label,MUIO_Label_FreeVert|MUIO_Label_Centered|MUIO_Label_DoubleFrame|(key))

