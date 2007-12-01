/***************************************************************************
** The Hacker's include to MUI v1.8 :-)
**
** Copyright 1997-98 by Alessandro Zummo
** azummo@ita.flashnet.it
**
** This include is unofficial, use at your own risk!
**
** You can also find other undocumented tags in libraries/mui.h :-)
**
****************************************************************************
** Class Tree
****************************************************************************
**
** rootclass                   (BOOPSI's base class)
** +--Notify                   (implements notification mechanism)
** !  +--Area                  (base class for all GUI elements)
** !     +--Framedisplay       (displays frame specification)
** !     !  \--Popframe        (popup button to adjust a frame spec)
** !     +--Imagedisplay       (displays image specification)
** !     !  \--Popimage        (popup button to adjust an image spec)
** !     +--Pendisplay         (displays a pen specification)
** !     !  \--Poppen          (popup button to adjust a pen spec)
** !     +--Group              (groups other GUI elements)
** !        +--Register        (handles page groups with titles)
** !        !  \--Penadjust    (group to adjust a pen)
** !        +--Frameadjust     (group to adjust a frame)
** !        +--Imageadjust     (group to adjust an image)
**
*/


#ifndef MUI_UNDOC_H
#define MUI_UNDOC_H

#if defined(__GNUC__)
# pragma pack(2)
#endif


//Uncomment this if you want be able to use all the undocumented features
//But remember to modify your libraries/mui.h include

//#define UNDOC_HACK


/*************************************************************************
** Black box specification structures for images, pens, frames
*************************************************************************/

/* Defined in mui.h
struct MUI_PenSpec
{
    char buf[32];
};
*/

struct MUI_ImageSpec
{
  char buf[64];
};

struct MUI_FrameSpec
{
  char buf[32];
};


// I'm not sure if MUI_ImageSpec and MUI_FrameSpec are 32 or 64 bytes wide.

/*************************************************************************
** The real MUI_NotifyData structure
*************************************************************************/

#ifdef UNDOC_HACK

struct MUI_NotifyData
{
    struct MUI_GlobalInfo *mnd_GlobalInfo;
    ULONG                  mnd_UserData;
    ULONG                  mnd_ObjectID;
    ULONG priv1;
    Object                *mnd_ParentObject; // The name may not be the real one
    ULONG priv3;
    ULONG priv4;
};

#define _parent(obj)    (muiNotifyData(obj)->mnd_ParentObject) /* valid between MUIM_Setup/Cleanup */

#else

#define _parent(obj)    xget(obj,MUIA_Parent)

#endif


// The use of _parent(obj) macro is strictly forbidden! Use xget(obj,MUIA_Parent) instead.


/****************************************************************************/
/** Flags                                                                  **/
/****************************************************************************/

#define MADF_OBJECTVISIBLE     (1<<14) // The object is visible

#define MUIMRI_INVIRTUALGROUP  (1<<29) // The object is inside a virtual group
#define MUIMRI_ISVIRTUALGROUP  (1<<30) // The object is a virtual group


/****************************************************************************/
/** Crawling                                                               **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Crawling[];
#else
#define MUIC_Crawling "Crawling.mcc"
#endif

#define CrawlingObject MUI_NewObject(MUIC_Crawling


/****************************************************************************/
/** Application                                                            **/
/****************************************************************************/

/* Attributes */

#ifndef MUIA_Application_UsedClasses
#define MUIA_Application_UsedClasses    0x8042E9A7 /* V20 (!) */
#endif


/****************************************************************************/
/** Window                                                                 **/
/****************************************************************************/

/* Methods */

#define MUIM_Window_ActionIconify 0x80422cc0 /* V18 */

#ifndef MUIM_Window_Cleanup
#define MUIM_Window_Cleanup       0x8042ab26 /* Custom Class */ /* V18 */
struct  MUIP_Window_Cleanup       { ULONG MethodID; }; /* Custom Class */
#endif

#ifndef MUIM_Window_Setup
#define MUIM_Window_Setup         0x8042c34c /* Custom Class */ /* V18 */
struct  MUIP_Window_Setup         { ULONG MethodID; }; /* Custom Class */
#endif

/* Attributes */

#define MUIA_Window_DisableKeys   0x80424c36 /* V15 isg ULONG */


/****************************************************************************/
/** Area                                                                   **/
/****************************************************************************/

/* Methods */

#ifndef MUIM_DoDrag
#define MUIM_DoDrag          0x804216bb /* V18 */ /* Custom Class */
struct  MUIP_DoDrag          { ULONG MethodID; LONG touchx; LONG touchy; ULONG flags; }; /* Custom Class */
#endif

#define MUIM_CreateDragImage 0x8042eb6f /* V18 */ /* Custom Class */
#define MUIM_DeleteDragImage 0x80423037 /* V18 */ /* Custom Class */
#define MUIM_GoActive        0x8042491a
#define MUIM_GoInactive      0x80422c0c
#define MUIM_CustomBackfill  0x80428d73

struct  MUIP_CustomBackfill  { ULONG MethodID; LONG left; LONG top; LONG right; LONG bottom; LONG xoffset; LONG yoffset; };
struct  MUIP_DeleteDragImage { ULONG MethodID; struct MUI_DragImage *di; };              /* Custom Class */
struct  MUIP_CreateDragImage { ULONG MethodID; LONG touchx; LONG touchy; ULONG flags; }; /* Custom Class */

/* Attributes */

#define MUIA_CustomBackfill  0x80420a63


#define MUIV_CreateBubble_DontHidePointer (1<<0)

struct MUI_DragImage
{
    struct BitMap *bm;
    WORD width;  /* exact width and height of bitmap */
    WORD height;
    WORD touchx; /* position of pointer click relative to bitmap */
    WORD touchy;
    ULONG flags; /* must be set to 0 */
};


/****************************************************************************/
/** Imagedisplay                                                           **/
/****************************************************************************/

/* Attributes */

#define MUIA_Imagedisplay_Spec 0x8042a547 /* V11 isg struct MUI_ImageSpec * */


/****************************************************************************/
/** Imageadjust                                                            **/
/****************************************************************************/

/* Attributes */

#define MUIA_Imageadjust_Type  0x80422f2b /* V11 i.. LONG */


/****************************************************************************/
/** Framedisplay                                                           **/
/****************************************************************************/

/* Attributes */

#define MUIA_Framedisplay_Spec 0x80421794 /* isg struct MUI_FrameSpec * */


/****************************************************************************/
/** Prop                                                                   **/
/****************************************************************************/

/* Attributes */

#ifndef MUIA_Prop_DeltaFactor
#define MUIA_Prop_DeltaFactor 0x80427c5e /* V4 .s. LONG */
#endif
#define MUIA_Prop_DoSmooth    0x804236ce /* V4 i.. LONG */
#define MUIA_Prop_Release     0x80429839 /* V? g BOOL */ /* private */
#define MUIA_Prop_Pressed     0x80422cd7 /* V6 g BOOL */ /* private */


/****************************************************************************/
/** Group                                                                  **/
/****************************************************************************/

/* Attributes */

#define MUIA_Group_Forward    0x80421422 /* V11 .s. BOOL */

/****************************************************************************/
/** List                                                                   **/
/****************************************************************************/

/* Attributes */

#define MUIA_List_Prop_Entries  0x8042a8f5 /* V? ??? */
#define MUIA_List_Prop_Visible  0x804273e9 /* V? ??? */
#define MUIA_List_Prop_First    0x80429df3 /* V? ??? */


/****************************************************************************/
/** Text                                                                   **/
/****************************************************************************/

/* Attributes */

#define MUIA_Text_HiCharIdx   0x804214f5


/****************************************************************************/
/** Dtpic                                                                  **/
/****************************************************************************/

/* Attributes */

#define MUIA_Dtpic_Name 0x80423d72

#define MUIV_Application_OCW_ScreenPage (1<<1) /* show just the screen page of the config window */

#ifndef MUIA_Window_ShowPopup
#define MUIA_Window_ShowPopup 0x8042324e
#endif

#ifndef MUIA_Window_ShowSnapshot
#define MUIA_Window_ShowSnapshot 0x80423c55
#endif

#ifndef MUIA_Window_ShowPrefs
#define MUIA_Window_ShowPrefs 0x8042e262
#endif

#ifndef MUIA_Window_ShowIconify
#define MUIA_Window_ShowIconify 0x8042bc26
#endif

#ifndef MUIA_Window_ShowAbout
#define MUIA_Window_ShowAbout 0x80429c1e
#endif

#ifndef MUIA_Window_ShowJump
#define MUIA_Window_ShowJump 0x80422f40
#endif

#ifndef MUIA_Window_Frontdrop
#define MUIA_Window_Frontdrop 0x80426411
#endif

#ifndef MUIA_Window_AllowTopMenus
#define MUIA_Window_AllowTopMenus 0x8042fe69
#endif

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* MUI_UNDOC_H */



