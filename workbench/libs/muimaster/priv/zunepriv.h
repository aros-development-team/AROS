#ifndef __ZUNE_PRIV_H__
#define __ZUNE_PRIV_H__

#ifdef _AROS


#include <libdefs.h>
#include <exec/memory.h>
#include <aros/debug.h>
#include <muimaster_private.h>

#define GPOINTER_TO_UINT(p) (IPTR)p
#define _U(p) (IPTR)p

#define SLONG_FMT "ld"
#define ULONG_FMT "lu"

#ifndef DoSuperNew
#define DoSuperNew(cl, obj, tags...) \
({ STACKIPTR _tags[] = { tags };\
   STACKIPTR _args[] = { OM_NEW, (STACKIPTR)_tags };\
   DoSuperMethodA(cl, obj, (Msg)_args); })
#endif /* !DoSuperNew */

/*** all of this should really go into glib.h ***/

#define g_malloc(x)               AllocVec((x),MEMF_ANY)
#define g_malloc0(x)              AllocVec((x),MEMF_CLEAR)
#define g_free(x)                 FreeVec(x)
#define g_strdup(x)               strcpy(AllocVec(strlen(x)+1,MEMF_ANY),(x))
#define g_snprintf(s,n,f,args...) snprintf(s,n,f,args)

#define GMemChunk void /* this makes for an APTR */

#define g_mem_chunk_create(type,x,y) CreatePool(MEMF_CLEAR,16384,8192)
#define g_mem_chunk_destroy(pool)    DeletePool(pool)

#define g_chunk_new(type,pool)  (type *)AllocPooled((pool),sizeof(type))
#define g_chunk_new0(type,pool) (type *)AllocPooled((pool),sizeof(type))
#define g_chunk_free(data,pool) FreePooled((pool),(data),sizeof(*(data)))

#else

#include <zune/boopsi.h>
#include <zune/preprotos.h>

/* should use atomic_t, no ? */
extern int __zune_signals;

#define ASSERT(x) g_assert(x)

#endif

#include <gdk/gdktypes.h>
#include <zune/zune_common.h>
#include <shortcuts.h>

/***************************************************************************
** Standard MUI Images & Backgrounds
***************************************************************************/

#define MUII_WindowBack      0   /* These images are configured   */
#define MUII_RequesterBack   1   /* with the preferences program. */
#define MUII_ButtonBack      2
#define MUII_ListBack        3
#define MUII_TextBack        4
#define MUII_PropBack        5
#define MUII_PopupBack       6
#define MUII_SelectedBack    7
#define MUII_ListCursor      8
#define MUII_ListSelect      9
#define MUII_ListSelCur     10
#define MUII_ArrowUp        11
#define MUII_ArrowDown      12
#define MUII_ArrowLeft      13
#define MUII_ArrowRight     14
#define MUII_CheckMark      15
#define MUII_RadioButton    16
#define MUII_Cycle          17
#define MUII_PopUp          18
#define MUII_PopFile        19
#define MUII_PopDrawer      20
#define MUII_PropKnob       21
#define MUII_Drawer         22
#define MUII_HardDisk       23
#define MUII_Disk           24
#define MUII_Chip           25
#define MUII_Volume         26
#define MUII_RegisterBack   27
#define MUII_Network        28
#define MUII_Assign         29
#define MUII_TapePlay       30
#define MUII_TapePlayBack   31
#define MUII_TapePause      32
#define MUII_TapeStop       33
#define MUII_TapeRecord     34
#define MUII_GroupBack      35
#define MUII_SliderBack     36
#define MUII_SliderKnob     37
#define MUII_TapeUp         38
#define MUII_TapeDown       39
#define MUII_PageBack       40
#define MUII_ReadListBack   41
#define MUII_Count          42

#define MUII_BACKGROUND     128    /* These are direct color    */
#define MUII_SHADOW         129    /* combinations and are not  */
#define MUII_SHINE          130    /* affected by users prefs.  */
#define MUII_FILL           131
#define MUII_SHADOWBACK     132    /* Generally, you should     */
#define MUII_SHADOWFILL     133    /* avoid using them. Better  */
#define MUII_SHADOWSHINE    134    /* use one of the customized */
#define MUII_FILLBACK       135    /* images above.             */
#define MUII_FILLSHINE      136
#define MUII_SHINEBACK      137
#define MUII_FILLBACK2      138
#define MUII_HSHINEBACK     139
#define MUII_HSHADOWBACK    140
#define MUII_HSHINESHINE    141
#define MUII_HSHADOWSHADOW  142
#define MUII_MARKSHINE      143
#define MUII_MARKHALFSHINE  144
#define MUII_MARKBACKGROUND 145
#define MUII_LASTPAT        145



/***************************************************************************
** Special values for some methods
***************************************************************************/

#define MUIV_TriggerValue    0x49893131
#define MUIV_NotTriggerValue 0x49893133
#define MUIV_EveryTime       0x49893131

#define MUIV_Notify_Self        1
#define MUIV_Notify_Window      2
#define MUIV_Notify_Application 3
#define MUIV_Notify_Parent      4

#define MUIV_Application_Save_ENV     ((STRPTR) 0)
#define MUIV_Application_Save_ENVARC  ((STRPTR)~0)
#define MUIV_Application_Load_ENV     ((STRPTR) 0)
#define MUIV_Application_Load_ENVARC  ((STRPTR)~0)

#define MUIV_Application_ReturnID_Quit -1

#define MUIV_List_Insert_Top             0
#define MUIV_List_Insert_Active         -1
#define MUIV_List_Insert_Sorted         -2
#define MUIV_List_Insert_Bottom         -3

#define MUIV_List_Remove_First           0
#define MUIV_List_Remove_Active         -1
#define MUIV_List_Remove_Last           -2
#define MUIV_List_Remove_Selected       -3

#define MUIV_List_Select_Off             0
#define MUIV_List_Select_On              1
#define MUIV_List_Select_Toggle          2
#define MUIV_List_Select_Ask             3

#define MUIV_List_GetEntry_Active       -1
#define MUIV_List_Select_Active         -1
#define MUIV_List_Select_All            -2

#define MUIV_List_Redraw_Active         -1
#define MUIV_List_Redraw_All            -2

#define MUIV_List_Move_Top               0
#define MUIV_List_Move_Active           -1
#define MUIV_List_Move_Bottom           -2
#define MUIV_List_Move_Next             -3 /* only valid for second parameter */
#define MUIV_List_Move_Previous         -4 /* only valid for second parameter */

#define MUIV_List_Exchange_Top           0
#define MUIV_List_Exchange_Active       -1
#define MUIV_List_Exchange_Bottom       -2
#define MUIV_List_Exchange_Next         -3 /* only valid for second parameter */
#define MUIV_List_Exchange_Previous     -4 /* only valid for second parameter */

#define MUIV_List_Jump_Top               0
#define MUIV_List_Jump_Active           -1
#define MUIV_List_Jump_Bottom           -2
#define MUIV_List_Jump_Up               -4
#define MUIV_List_Jump_Down             -3

#define MUIV_List_NextSelected_Start    -1
#define MUIV_List_NextSelected_End      -1

#define MUIV_DragQuery_Refuse 0
#define MUIV_DragQuery_Accept 1

#define MUIV_DragReport_Abort    0
#define MUIV_DragReport_Continue 1
#define MUIV_DragReport_Lock     2
#define MUIV_DragReport_Refresh  3




/***************************************************************************
** Control codes for text strings
***************************************************************************/

#define MUIX_R "\033r"    /* right justified */
#define MUIX_C "\033c"    /* centered        */
#define MUIX_L "\033l"    /* left justified  */

#define MUIX_N "\033n"    /* normal     */
#define MUIX_B "\033b"    /* bold       */
#define MUIX_I "\033i"    /* italic     */
#define MUIX_U "\033u"    /* underlined */

#define MUIX_PT "\0332"   /* text pen           */
#define MUIX_PH "\0338"   /* highlight text pen */


/***************************************************************************
**
** Controlling Objects
** -------------------
**
** set() and get() are two short stubs for BOOPSI GetAttr() and SetAttrs()
** calls:
**
** {
**    char *x;
**
**    set(obj,MUIA_String_Contents,"foobar");
**    get(obj,MUIA_String_Contents,&x);
**
**    printf("gadget contains '%s'\n",x);
** }
**
** nnset() sets an attribute without triggering a possible notification.
**
***************************************************************************/

#ifndef __cplusplus

#define get(obj,attr,store) GetAttr(attr,obj,(ULONG *)store)
#define set(obj,attr,value) SetAttrs(obj,attr,value,TAG_DONE)
#define nnset(obj,attr,value) SetAttrs(obj,MUIA_NoNotify,TRUE,attr,value,TAG_DONE)

#define setmutex(obj,n)     set(obj,MUIA_Radio_Active,n)
#define setcycle(obj,n)     set(obj,MUIA_Cycle_Active,n)
#define setstring(obj,s)    set(obj,MUIA_String_Contents,s)
#define setcheckmark(obj,b) set(obj,MUIA_Selected,b)
#define setslider(obj,l)    set(obj,MUIA_Numeric_Value,l)

#endif


/*************************/
/* Area */

/* Methods */

#define MUIM_ConnectParent          0x80429ab9 /* ZV1  */
#define MUIM_DisconnectParent       0x80429aba /* ZV1  */
#define MUIM_Layout                 0x80429abb /* ZV1  */
#define MUIM_ConnectParentWindow    0x80429abd /* ZV1  */

struct  MUIP_ConnectParent          { ULONG MethodID; Object *parent; };
struct  MUIP_DisconnectParent       { ULONG MethodID; };
struct  MUIP_Layout                 { ULONG MethodID; };
struct  MUIP_ConnectParentWindow    { ULONG MethodID; Object *win; struct MUI_RenderInfo *mri; };


/*************************/
/* Application */

/* Methods */

#define MUIM_Application_Iconify       0x80429ab8 /* ZV1  */

/*************************/
/* Window */

/* Methods */

#define MUIM_Window_RecalcDisplay     0x80429abc
struct  MUIP_Window_RecalcDisplay  { ULONG MethodID; };


/*************************/
/* Image */

#define MUIM_Image_ToggleState 0x80429abf
struct  MUIP_Image_ToggleState { ULONG MethodID; };


/*************************/
/* Dataspace */

#define MUIA_Dataspace_Comments 0x80429ac0

#endif
