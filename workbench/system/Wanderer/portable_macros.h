#ifndef __PORTABLE_MACROS__
#define __PORTABLE_MACROS__

#define TAGITEM  struct TagItem **

#ifndef __AROS__

#if !defined(__AROS__) || !defined(__MORPHOS__)
    #ifdef __amigaos4__
    #define IPTR uint32
    #else
    #define IPTR ULONG
    #endif
#endif

#ifndef __AROS__
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

#if defined (__MORPHOS__) || (defined (__AMIGA__) && !defined(__PPC__))
#include <dos/dostags.h>
#endif

#ifndef __AROS__
#include <SDI_compiler.h>
#include <SDI_hook.h>
#include <SDI_stdarg.h>
#endif


#ifndef __AROS__

#ifdef __amigaos4__
#define UQUAD uint64
#define QUAD int64
#else
#define UQUAD ULONG
#define QUAD LONG
#endif

extern struct Library  *MUIMasterBase;
#ifdef __amigaos4__
extern struct MUIMasterIFace   *IMUIMaster;
#endif


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




#define MAX(a,b) (((a) > (b))?(a):(b))
#define MIN(a,b) (((a) > (b))?(b):(a))

#define _AndRectRect(rect1, rect2, intersect)                  \
({                                                             \
    BOOL res;                                                  \
                                                               \
    if (overlap(*(rect1), *(rect2)))                           \
    {                                                          \
    (intersect)->MinX = MAX((rect1)->MinX, (rect2)->MinX);     \
    (intersect)->MinY = MAX((rect1)->MinY, (rect2)->MinY);     \
    (intersect)->MaxX = MIN((rect1)->MaxX, (rect2)->MaxX);     \
    (intersect)->MaxY = MIN((rect1)->MaxY, (rect2)->MaxY);     \
                                                               \
    res = TRUE;                                                \
    }                                                          \
    else                                                       \
        res = FALSE;                                           \
                                                               \
    res;                                                       \
})


#define _DoRectsOverlap(Rect, x1, y1, x2, y2) \
(                                             \
    y1 <= (Rect)->MaxY &&                     \
    y2 >= (Rect)->MinY &&                     \
    x1 <= (Rect)->MaxX &&                     \
    x2 >= (Rect)->MinX                        \
)

#define overlap(a,b) _DoRectsOverlap(&(a), (b).MinX, (b).MinY, (b).MaxX, (b).MaxY)




#define GET(obj,attr,store) get(obj,attr,store)
#define SET(obj,attr,value) set(obj,attr,value)

#define XGET(object, attribute)                 \
({                                              \
    IPTR __storage = 0;                         \
    GetAttr((attribute), (object), &__storage); \
    __storage;                                  \
})

#define NNSET(obj,attr,value) nnset(obj,attr,value)



#if !defined(__amigaos4__)

#define GetHead(_l)  \
({ struct List *l = (struct List *)(_l);  \
    l->lh_Head->ln_Succ ? l->lh_Head : (struct Node *)0;  \
})

#define GetSucc(_n)  \
({ struct Node *n = (struct Node *)(_n);  \
    n->ln_Succ->ln_Succ ? n->ln_Succ : (struct Node *)0;  \
})

#define GetTail(_l)  \
({ struct List *l = (struct List *)(_l);  \
    l->lh_TailPred->ln_Pred ? l->lh_TailPred : (struct Node *)0;  \
})

#define GetPred(_n)  \
({ struct Node *n = (struct Node *)(_n);  \
    n->ln_Pred->ln_Pred ? n->ln_Pred : (struct Node *)0;  \
})
#endif


#define ImageButton(label, imagePath) MUI_MakeObject(MUIO_Button, (IPTR) (label))

#define BOOPSI_DISPATCHER(ret, nameDsp, cls, obj, msg)   DISPATCHER(nameDsp)
#define BOOPSI_DISPATCHER_END



#define SendAppWindowMessage(...) sizeof(NULL)
#define RegisterWorkbench(...)  sizeof(NULL)
#define UnregisterWorkbench(...)  sizeof(NULL)
#define ArosInquire(...)  sizeof(NULL)
#define AROS_LE2LONG(...)  sizeof(NULL)
#define ADD2INIT(...)
#define ADD2EXIT(...)
#define Detach()
#define __showerror

#define DeinitRastPort FreeRastPort


extern struct RastPort *CreateRastPort(void);
extern struct RastPort *CloneRastPort(struct RastPort *rp);
extern void FreeRastPort(struct RastPort *rp);
extern BOOL AndRectRect(struct Rectangle *rect1, struct Rectangle *rect2, struct Rectangle *intersect);



#if defined (__AMIGA__) && !defined(__PPC__)
#define NP_UserData         (NP_Dummy + 26)
    /* optional value to install into task->tc_UserData. */
#endif


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

extern STRPTR StrDup (CONST_STRPTR str);

extern int IconWindowIconDrawerList_Initialize(void);
extern int IconWindowIconVolumeList_Initialize(void);
extern int WandererPrefs_Initialize(void);
extern int Wanderer_Initialize(void);
extern int IconWindow_Initialize(void);


extern void IconWindowIconDrawerList_Deinitialize(void);
extern void IconWindowIconVolumeList_Deinitialize(void);
extern void WandererPrefs_Deinitialize(void);
extern void Wanderer_Deinitialize(void);
extern void IconWindow_Deinitialize(void);



extern struct MUI_CustomClass *IconWindowIconVolumeList_CLASS;
extern struct MUI_CustomClass *IconWindowIconDrawerList_CLASS;
extern struct MUI_CustomClass *WandererPrefs_CLASS;
extern struct MUI_CustomClass *IconWindow_CLASS;


extern int initIconWindowClass(void);
//extern struct MUI_CustomClass  *initIconWindowClass(void);
extern struct MUI_CustomClass  * initIconListClass(void);
extern struct MUI_CustomClass  * initIconDrawerListClass(void);
extern struct MUI_CustomClass  *initIconListviewClass(void);
extern struct MUI_CustomClass  * initIconVolumeListClass(void);


extern struct MUI_CustomClass  *IconWindow_Class;
extern struct MUI_CustomClass  *IconList_Class;
extern struct MUI_CustomClass  *IconDrawerList_Class;
extern struct MUI_CustomClass  *IconListview_Class;
extern struct MUI_CustomClass  *IconVolumeList_Class ;


#endif

#endif

#endif
