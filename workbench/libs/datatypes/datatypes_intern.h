#ifndef DATATYPES_INTERN_H
#define DATATYPES_INTERN_H

/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal datatypes.library definitions.
    Lang: English.
*/


#ifndef PROTO_EXEC_H
#include  <proto/exec.h>
#endif
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif
#ifndef EXEC_MEMORY_H
#include  <exec/memory.h>
#endif
#ifndef EXEC_LISTS_H
#include  <exec/lists.h>
#endif
#ifndef AROS_ASMCALL_H
#include <aros/asmcall.h>
#endif
#ifndef UTILITY_HOOKS_H
#include <utility/hooks.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif
#ifndef DATATYPES_DATATYPES_H
#include <datatypes/datatypes.h>
#endif
#ifndef DATATYPES_DATATYPESCLASS_H
#include <datatypes/datatypesclass.h>
#endif
#ifndef DOS_DATETIME_H
#include <dos/datetime.h>
#endif
#ifndef INTUITION_CLASSUSR_H
#include <intuition/classusr.h>
#endif
#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif
#ifndef LIBRARIES_LOCALE_H
#include <libraries/locale.h>
#endif

enum
{
    SEM_LIB,
    SEM_ASYNC,
    SEM_MAX
};


struct DataTypesList
{
    struct SignalSemaphore  dtl_Lock;
    struct List		    dtl_SortedList;
    struct List		    dtl_BinaryList;
    struct List 	    dtl_ASCIIList;
    struct List		    dtl_IFFList;
    struct List		    dtl_MiscList;
    ULONG		    dtl_LongestMask;
    struct DateStamp	    dtl_DateStamp;
};	


struct CompoundDatatype
{
    struct DataType  DT;
    ULONG	     FlagLong;
    ULONG	     ParsePatSize;
    UBYTE	    *ParsePatMem;
    UBYTE	    *DTCDChunk;
    ULONG	     DTCDSize;
    BPTR	     SegList;
    BOOL (*Function)(struct DTHookContext *dthc);
    ULONG	     OpenCount;
    struct DataTypeHeader DTH;
};

#define CFLGB_PATTERN_UNUSED 0
#define CFLGF_PATTERN_UNUSED (1<<0)

#define CFLGB_IS_WILD        1
#define CFLGF_IS_WILD        (1<<1)


struct DTObject
{
    UBYTE	         dto_SourceType;
    UBYTE	         dto_pad;
    UBYTE	        *dto_Name;
    APTR		 dto_Handle;
    struct DataType	*dto_DataType;
    struct IBox	         dto_Domain;
    struct IBox	         dto_SelectDomain;
    struct Rectangle     dto_SelectRect;
    ULONG		 dto_TotalPHoriz;
    ULONG		 dto_TotalPVert;
    ULONG		 dto_NominalHoriz;
    ULONG		 dto_NominalVert;
    UBYTE		*dto_ObjName;
    UBYTE		*dto_ObjAuthor;
    UBYTE		*dto_ObjAnnotation;
    UBYTE		*dto_ObjCopyright;
    UBYTE		*dto_ObjVersion;
    ULONG		 dto_ObjectID;
    ULONG		 dto_UserData;
    struct FrameInfo     dto_FrameInfo;
    ULONG		 dto_Flags;
    struct DTSpecialInfo dto_DTSpecialInfo;
    ULONG		 dto_OldTopVert;
    ULONG		 dto_OldTopHoriz;
    WORD		 dto_MouseX;
    WORD		 dto_MouseY;
    WORD		 dto_StartX;
    WORD		 dto_StartY;
    UWORD		 dto_LinePtrn;
    struct Process	*dto_PrinterProc;
    struct Process	*dto_LayoutProc;
};

#define DTOFLGB_HAS_MOVED  0
#define DTOFLGF_HAS_MOVED  (1<<0)


/*
 * Sigh, we need a global SysBase in order to link properly against the
 * functions in amiga.lib such as DoMethodA() which in debugging mode
 * require a SysBase. This used to be called __dt_GlobalSysBase.
 * Interestingly enough, this is a common symbol...
 */
struct ExecBase *SysBase;

struct DataTypesBase
{
    /* Datatypes library structure */
    struct Library dtb_LibNode;
    
    /* Align to long word */
    UWORD dtb_Pad1;
    
    /* libraries used by datatypes system */
    struct ExecBase *dtb_SysBase;
    struct Library  *dtb_DOSBase;
    struct Library  *dtb_IntuitionBase;
    struct Library  *dtb_GfxBase;
    struct Library  *dtb_LayersBase;
    struct Library  *dtb_UtilityBase;
    struct Library  *dtb_IFFParseBase;
    struct Library  *dtb_RexxSysBase;
    struct Library  *dtb_LocaleBase;
    struct Library  *dtb_IconBase;
    struct Library  *dtb_WorkbenchBase;
    struct Catalog  *dtb_LibsCatalog;
    
    /* remember seglist */
    BPTR dtb_SegList;
    
    /* datatypes internal semaphores */
    struct SignalSemaphore dtb_Semaphores[SEM_MAX];
    
    /* list of all known datatypes */
    struct DataTypesList *dtb_DTList;
    
    /* pointer to the datatypesclass baseclass */
    struct IClass *dtb_DataTypesClass;
};

/* named object name */
#define DATATYPESLIST "DataTypesList"

/* datatypes lists */
struct IntDataTypesList
{
    struct SignalSemaphore  dtl_Lock;
    struct List dtl_SortedList;
    struct List dtl_BinaryList;
    struct List dtl_ASCIIList;
    struct List dtl_IFFList;
    struct List dtl_MiscList;
    ULONG  dtl_LongestMask;
    struct DateStamp dtl_DateStamp;
};


/* shortcuts to the internal structures */
#define INTDT(dt)      ((struct IntDataType *)(dt))
#define INTDTLt(dtl)   ((struct IntDataTypesList *)(dtl))


/* Prototypes for help functions */
struct Node *FindNameNoCase(struct Library *DataTypesBase, struct List *list,
			    STRPTR name);
struct DataTypesList *GetDataTypesList(struct DataTypesBase *DataTypesBase);
struct CompoundDatatype *ExamineData(struct Library *DataTypesBase,
				     struct DTHookContext *dthc,
				     UBYTE *CheckArray, UWORD CheckSize,
				     UBYTE *Filename, ULONG Size);
struct CompoundDatatype *ExamineLock(BPTR lock, struct FileInfoBlock *fib,
				     struct Library *DataTypesBase);
void dt_sprintf(struct Library *DataTypesBase, UBYTE *buffer, UBYTE *format, ...);

ULONG setattrs(struct Library *DataTypesBase, Object *object, Tag firstTag,...);
ULONG Do_OM_NOTIFY(struct Library *DataTypesBase, Object *object, struct GadgetInfo *ginfo, ULONG flags, Tag firstTag,...);
ULONG DoGad_OM_NOTIFY(struct Library *DataTypesBase, Object *object,
		      struct Window *window, struct Requester *req,
		      ULONG flags, Tag firstTag, ...);
ULONG dogadgetmethod(struct Library *DataTypesBase, struct Gadget *gad,
		     struct Window *win, struct Requester *req,
		     ULONG MethodID, ...);
struct Catalog *opencatalog(struct Library *DataTypesBase, struct Locale *locale,
			    STRPTR name, Tag firstTag, ...);
BPTR NewOpen(struct Library *DataTypesBase, STRPTR name, ULONG SourceType,
	     ULONG Length);


BOOL InstallClass(struct Library *DataTypesBase);
BOOL TryRemoveClass(struct Library *DataTypesBase);


typedef struct IntuitionBase IntuiBase;

#ifndef GLOBAL_INTUIBASE
#undef IntuitionBase
#define IntuitionBase ((struct DataTypesBase *)DataTypesBase)->dtb_IntuitionBase
#endif

#define GPB(x) ((struct DataTypesBase *)x)


#define UtilityBase ((struct DataTypesBase *)DataTypesBase)->dtb_UtilityBase
#define DOSBase ((struct DataTypesBase *)DataTypesBase)->dtb_DOSBase
/* We cannot define this t */
/* #define IntuitionBase ((struct DataTypesBase *)DataTypesBase)->dtb_IntuitionBase */
#define SysBase ((struct DataTypesBase *)DataTypesBase)->dtb_SysBase
#define IFFParseBase ((struct DataTypesBase *)DataTypesBase)->dtb_IFFParseBase
#define LocaleBase (GPB(DataTypesBase)->dtb_LocaleBase)
#define GfxBase (GPB(DataTypesBase)->dtb_GfxBase)
#define IconBase (GPB(DataTypesBase)->dtb_IconBase)
#define WorkbenchBase (GPB(DataTypesBase)->dtb_WorkbenchBase)

#define expunge() \
AROS_LC0(BPTR, expunge, struct DataTypesBase *, DataTypesBase, 3, DataTypes)
     

#endif /* DATATYPES_INTERN_H */
