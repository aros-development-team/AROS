#ifndef __PORTABLE_MACROS__
#define __PORTABLE_MACROS__


#ifdef __AROS__
  #define TAGITEM const struct TagItem **
#else
  #define TAGITEM  struct TagItem **
#endif

#ifndef __AROS__
#ifdef __amigaos4__
#define IPTR uint32
#else
#define IPTR ULONG
#endif
#define STACKED
#endif

#ifndef __AROS__
#include <proto/exec.h>
#include <proto/utility.h>
#endif


#ifndef __AROS__
#include <proto/wb.h>
#endif


#ifndef __AROS__
#define AROS_BIG_ENDIAN 1
#endif


#ifndef __AROS__
#include <exec_AROS/lists.h>
#endif


#ifndef __AROS__
#include <workbench/icon.h>
#endif

#ifdef __MORPHOS__
#include <dos/dostags.h>
#endif

#ifndef __AROS__
#include <SDI_compiler.h>
#include <SDI_hook.h>
#include <SDI_stdarg.h>
#endif


#ifndef __AROS__
#define AROS_UFHA(type,name,reg)  type name


#define AROS_UFH0(rettype,name) rettype name()

#define AROS_UFH1(rettype,name,a1) rettype name(a1)

#define AROS_UFH2(rettype,name,a1,a2) rettype name (a1, a2)

#define AROS_UFH3(rettype,name,a1,a2,a3) rettype name (a1, a2, a3)


#define AROS_USERFUNC_INIT
#define AROS_USERFUNC_EXIT

#define MUIB_MUI  (TAG_USER)                /* Base for legacy MUI identifiers   */
#define MUIB_RSVD (MUIB_MUI  | 0x10400000)  /* Base for AROS reserved range      */
#define MUIB_ZUNE (MUIB_RSVD | 0x00020000)  /* Base for Zune core reserved range */
#define MUIB_AROS (MUIB_RSVD | 0x00070000)  /* Base for AROS core reserved range */


#ifdef __amigaos4__
#define UQUAD uint64
#define QUAD int64
#else
#define UQUAD ULONG
#define QUAD LONG
#endif

#define Detach()
#define __showerror

#define DeinitRastPort(rp)
#define CloneRastPort(rp) (rp)
#define FreeRastPort(rp)

#define GET(obj,attr,store) get(obj,attr,store)
#define SET(obj,attr,value) set(obj,attr,value)

#define XGET(object, attribute)                 \
({                                              \
    IPTR __storage = 0;                         \
    GetAttr((attribute), (object), &__storage); \
    __storage;                                  \
})

#define NNSET(obj,attr,value) nnset(obj,attr,value)


#define ImageButton(label, imagePath) MUI_MakeObject(MUIO_Button, (IPTR) (label))

extern struct Library  *MUIMasterBase;
#ifdef __amigaos4__
extern struct MUIMasterIFace   *IMUIMaster;
#endif

#define SendAppWindowMessage(...) sizeof(NULL)
#define RegisterWorkbench(...)  sizeof(NULL)
#define UnregisterWorkbench(...)  sizeof(NULL)
#define ArosInquire(...)  sizeof(NULL)
#define AROS_LE2LONG(...)  sizeof(NULL)
#define CreateRastPort(...)  sizeof(NULL)
#define AndRectRect(...)  sizeof(NULL)

#define ADD2INIT(...)
#define ADD2EXIT(...)

#define BOOPSI_DISPATCHER(ret, nameDsp, cls, obj, msg)   DISPATCHER(nameDsp)
#define BOOPSI_DISPATCHER_END

struct MUIP_CreateDragImage     {STACKED ULONG MethodID; STACKED LONG touchx; STACKED LONG touchy; STACKED ULONG flags;};

struct MUI_DragImage
{
  struct BitMap *bm;
  WORD width;  /* exact width and height of bitmap */
  WORD height;
  WORD touchx; /* position of pointer click relative to bitmap */
  WORD touchy;
  ULONG flags; /* must be set to 0 */
};

struct MUIP_DeleteDragImage     {STACKED ULONG MethodID; STACKED struct MUI_DragImage *di;};

struct  MUIP_Layout                 {STACKED ULONG MethodID;};

#define MUIA_Prop_DeltaFactor    (MUIB_MUI|0x00427c5e) /* MUI:    is. LONG */
#define MUIM_CreateDragImage      (MUIB_MUI|0x0042eb6f) /* MUI: V18 */ /* For Custom Classes only */ /* Undoc */
#define MUIM_DeleteDragImage      (MUIB_MUI|0x00423037) /* MUI: V18 */ /* For Custom Classes only */ /* Undoc */

#ifndef __AROS__
#define MUIM_Application_Execute   (MUIB_Wanderer | 0x000000011)
#endif

extern struct MUI_CustomClass *IconWindowIconVolumeList_CLASS;
extern struct MUI_CustomClass *IconWindowIconDrawerList_CLASS;
extern struct MUI_CustomClass *WandererPrefs_CLASS;
extern struct MUI_CustomClass *IconWindow_CLASS;

extern struct MUI_CustomClass  *IconList_Class;
extern struct MUI_CustomClass  *IconDrawerList_Class;
extern struct MUI_CustomClass  *IconListview_Class;
extern struct MUI_CustomClass  *IconVolumeList_Class ;

int initIconWindowClass(void);     

#endif

#endif
