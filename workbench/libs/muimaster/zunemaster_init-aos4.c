/*
    Copyright (C) 2003, The AROS Development Team. 
    All rights reserved.
    
    $Id$
    
    This file provides the library initialization for OS4
*/

#undef __USE_INLINE__

#include <stdarg.h>
#include <exec/types.h>
#include <proto/exec.h>

#include "muimaster_intern.h"

#undef SysBase

#include <interfaces/zunemaster.h>


/* Version Tag */
#define VERSION  0
#define REVISION 1
#define VSTRING "zunemaster.library 0.1 (25.02.2005)"
//#include "zunemaster.library_rev.h"

struct Library *DataTypesBase;
struct Library *TextClibBase;
struct Library *MUIMasterBase; /* this library */

struct Interface *IGraphics;
struct Interface *IIntuition;
struct Interface *IUtility;
struct Interface *ILayers;
struct Interface *IGadTools;
struct Interface *ICyberGfx;
struct Interface *ICommodities;
struct Interface *IIFFParse;
struct Interface *IColorWheel;
struct Interface *IDiskfont;
struct Interface *ITextClip;
struct Interface *IDOS;
struct Interface *IAsl;
struct Interface *IKeymap;
struct Interface *IDataTypes;
struct Interface *IIcon;
struct ZuneMasterIFace *IZuneMaster;


static struct SignalSemaphore OpenSemaphore;

/*************************************************************************
 The start function
*************************************************************************/
#ifndef COMPILE_STATIC
LONG _start(void)
{
    return 20;
}
#endif

/*************************************************************************
 Open a library and its interface easily
*************************************************************************/
struct Library *OpenLibraryInterface(STRPTR name, int version, void *interface_ptr)
{
	struct Library *lib = IExec->OpenLibrary(name,version);
	struct Interface *iface;
	if (!lib) return NULL;

	iface = IExec->GetInterface(lib,"main",1,NULL);
	if (!iface)
	{
		IExec->CloseLibrary(lib);
		return NULL;
	}
	*((struct Interface**)interface_ptr) = iface;
	return lib;
}

/*************************************************************************
 Close a library and its interface easily
*************************************************************************/
void CloseLibraryInterface(struct Library *lib, void *interface)
{
	IExec->DropInterface(interface);
	IExec->CloseLibrary(lib);
}

/****************************************************************************/

struct Library *libOpen(struct LibraryManagerInterface *Self, ULONG version)
{
	struct Library *libBase = (struct Library *)Self->Data.LibBase;

	/* Add any specific open code here
	 * Return 0 before incrementing OpenCnt to fail opening */
	IExec->ObtainSemaphore(&OpenSemaphore);
 	/* Add up the open count */
	((struct Library *)libBase)->lib_OpenCnt++;
	IExec->ReleaseSemaphore(&OpenSemaphore);

	return libBase;
}

/* Close the library */
APTR libClose(struct LibraryManagerInterface *Self)
{
    struct Library *libBase = (struct Library *)Self->Data.LibBase;
    /* Make sure to undo what open did */
    IExec->ObtainSemaphore(&OpenSemaphore);
	/* Make the close count */
	libBase->lib_OpenCnt--;
	IExec->ReleaseSemaphore(&OpenSemaphore);
	return 0;
}


/* Expunge the library */
APTR libExpunge(struct LibraryManagerInterface *Self)
{
    /* If your library cannot be expunged, return 0 */
    struct ExecIFace *IExec = (struct ExecIFace *)(*(struct ExecBase **)4)->MainInterface;
    APTR result = (APTR)0;
    struct Library *libBase = (struct Library *)Self->Data.LibBase;

    if (libBase->lib_OpenCnt == 0)
    {
    	result = (APTR)MUIMB(libBase)->seglist;

	CloseLibraryInterface((struct Library*)MUIMB(libBase)->utilitybase,IUtility);
	CloseLibraryInterface((struct Library*)MUIMB(libBase)->dosbase,IDOS);
	CloseLibraryInterface((struct Library*)MUIMB(libBase)->gfxbase,IGraphics);
	CloseLibraryInterface(MUIMB(libBase)->aslbase,IAsl);
	CloseLibraryInterface(MUIMB(libBase)->layersbase,ILayers);
	CloseLibraryInterface((struct Library*)MUIMB(libBase)->intuibase,IIntuition);
	CloseLibraryInterface((struct Library*)MUIMB(libBase)->cxbase,ICommodities);
	CloseLibraryInterface(MUIMB(libBase)->gadtoolsbase,IGadTools);
	CloseLibraryInterface(MUIMB(libBase)->keymapbase,IKeymap);
	CloseLibraryInterface(DataTypesBase,IDataTypes);
	CloseLibraryInterface(MUIMB(libBase)->iffparsebase,IIFFParse);
	CloseLibraryInterface(MUIMB(libBase)->diskfontbase,IDiskfont);
	CloseLibraryInterface(MUIMB(libBase)->iconbase,IIcon);

        IExec->Remove((struct Node *)libBase);
        IExec->DeleteLibrary(libBase);
    }
    else
    {
        result = (APTR)0;
        libBase->lib_Flags |= LIBF_DELEXP;
    }
    return result;
}

/* The ROMTAG Init Function */
struct Library *libInit(struct Library *libBase, APTR seglist, struct Interface *exec)
{
    IExec = (struct ExecIFace *)exec;
    SysBase = exec->Data.LibBase;

    if ((SysBase->lib_Version < 51) || (SysBase->lib_Version == 51 && SysBase->lib_Revision < 19))
	return NULL;

    libBase->lib_Node.ln_Type = NT_LIBRARY;
    libBase->lib_Node.ln_Pri  = 0;
    libBase->lib_Node.ln_Name = "zunemaster.library";
    libBase->lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;
    libBase->lib_Version      = VERSION;
    libBase->lib_Revision     = REVISION;
    libBase->lib_IdString     = VSTRING;

    /* Store seg list */
    MUIMB(libBase)->seglist = (BPTR)seglist;

    IExec->InitSemaphore(&OpenSemaphore);

    /* Fill in rest of the library data */
    MUIMB(libBase)->sysbase = (struct ExecBase*)SysBase;
    if (!(MUIMB(libBase)->utilitybase = (void*)OpenLibraryInterface("utility.library",50,&IUtility)))
	goto out;
    if (!(MUIMB(libBase)->dosbase = (void*)OpenLibraryInterface("dos.library",50,&IDOS)))
	goto out;
    if (!(MUIMB(libBase)->gfxbase = (void*)OpenLibraryInterface("graphics.library",50,&IGraphics)))
	goto out;
    if (!(MUIMB(libBase)->aslbase = OpenLibraryInterface("asl.library",50,&IAsl)))
	goto out;
    if (!(MUIMB(libBase)->layersbase = OpenLibraryInterface("layers.library",50,&ILayers)))
	goto out;
    if (!(MUIMB(libBase)->intuibase = (void*)OpenLibraryInterface("intuition.library",50,&IIntuition)))
	goto out;
    if (!(MUIMB(libBase)->cxbase = OpenLibraryInterface("commodities.library",50,&ICommodities)))
 	goto out;
    if (!(MUIMB(libBase)->gadtoolsbase = OpenLibraryInterface("gadtools.library",37,&IGadTools)))
	goto out;
    if (!(MUIMB(libBase)->keymapbase = OpenLibraryInterface("keymap.library",37,&IKeymap)))
	goto out;
    if (!(DataTypesBase = OpenLibraryInterface("datatypes.library",39,&IDataTypes)))
	goto out;
    if (!(MUIMB(libBase)->iffparsebase = OpenLibraryInterface("iffparse.library",39,&IIFFParse)))
	goto out;
    if (!(MUIMB(libBase)->diskfontbase = OpenLibraryInterface("diskfont.library",39,&IDiskfont)))
	goto out;
    if (!(MUIMB(libBase)->iconbase = OpenLibraryInterface("icon.library",39,&IIcon)))
	goto out;

    /* continue even if cybergraphics.library is not available */
    MUIMB(libBase)->cybergfxbase = OpenLibraryInterface("cybergraphics.library",39,&ICyberGfx);

#ifdef HAVE_COOLIMAGES
    MUIMB(libbase)->coolimagesbase = OpenLibraryInterface("coolimages.library",0,&ICoolImages);
#endif
    
    IExec->InitSemaphore(&MUIMB(libBase)->ZuneSemaphore);
    
    IExec->NewMinList(&MUIMB(libBase)->BuiltinClasses);
    IExec->NewMinList(&MUIMB(libBase)->Applications);

    /* Store the libraries base */
    MUIMasterBase = libBase;

    return libBase;
out:
    CloseLibraryInterface((struct Library*)MUIMB(libBase)->utilitybase,IUtility);
    CloseLibraryInterface((struct Library*)MUIMB(libBase)->dosbase,IDOS);
    CloseLibraryInterface((struct Library*)MUIMB(libBase)->gfxbase,IGraphics);
    CloseLibraryInterface(MUIMB(libBase)->aslbase,IAsl);
    CloseLibraryInterface(MUIMB(libBase)->layersbase,ILayers);
    CloseLibraryInterface((struct Library*)MUIMB(libBase)->intuibase,IIntuition);
    CloseLibraryInterface((struct Library*)MUIMB(libBase)->cxbase,ICommodities);
    CloseLibraryInterface(MUIMB(libBase)->gadtoolsbase,IGadTools);
    CloseLibraryInterface(MUIMB(libBase)->keymapbase,IKeymap);
    CloseLibraryInterface(DataTypesBase,IDataTypes);
    CloseLibraryInterface(MUIMB(libBase)->iffparsebase,IIFFParse);
    CloseLibraryInterface(MUIMB(libBase)->diskfontbase,IDiskfont);
    CloseLibraryInterface(MUIMB(libBase)->iconbase,IIcon);
    return NULL;
}

/* ----------- Implementation of main Interface ----------------- */
ASM APTR LIB_MUI_AddClipping(REG(a0, struct MUI_RenderInfo *mri), REG(d0, WORD left), REG(d1, WORD top), REG(d2, WORD width), REG(d3, WORD height));
ASM APTR LIB_MUI_AddClipRegion(REG(a0,struct MUI_RenderInfo *mri), REG(a1, struct Region *r));
ASM APTR LIB_MUI_AllocAslRequest(REG(d0,unsigned long reqType), REG(a0,struct TagItem *tagList));
ASM BOOL LIB_MUI_AslRequest(REG(a0, APTR requester), REG(a1, struct TagItem *tagList));
ASM BOOL LIB_MUI_BeginRefresh(REG(a0, struct MUI_RenderInfo *mri), REG(d0, ULONG flags));
ASM struct MUI_CustomClass *LIB_MUI_CreateCustomClass(REG(a0, struct Library *base), REG(a1, char *supername), REG(a2, struct MUI_CustomClass *supermcc), REG(d0,int datasize), REG(a3, APTR dispatcher));
ASM BOOL LIB_MUI_DeleteCustomClass(REG(a0,struct MUI_CustomClass *mcc));
ASM VOID LIB_MUI_DisposeObject(REG(a0,Object *obj));
ASM VOID LIB_MUI_EndRefresh(REG(a0, struct MUI_RenderInfo *mri), REG(d0, ULONG flags));
ASM LONG LIB_MUI_Error(VOID);
ASM VOID LIB_MUI_FreeAslRequest(REG(a0, APTR requester));
ASM VOID LIB_MUI_FreeClass(REG(a0, struct IClass *classptr));
ASM struct IClass *LIB_MUI_GetClass(REG(a0,char *classname));
ASM BOOL LIB_MUI_Layout(REG(a0,Object *obj), REG(d0, LONG left), REG(d1,LONG top), REG(d2,LONG width), REG(d3,LONG height), REG(d4, ULONG flags));
ASM Object *LIB_MUI_MakeObjectA(REG(d0, LONG type), REG(a0, ULONG *params));
ASM Object *LIB_MUI_NewObjectA(REG(a0, char *classname), REG(a1, struct TagItem *tags));
ASM LONG LIB_MUI_ObtainPen(REG(a0, struct MUI_RenderInfo *mri), REG(a1, struct MUI_PenSpec *spec), REG(d0, ULONG flags));
ASM VOID LIB_MUI_Redraw(REG(a0, Object *obj), REG(d0, ULONG flags));
ASM VOID LIB_MUI_RejectIDCMP(REG(a0, Object *obj), REG(d0, ULONG flags));
ASM VOID LIB_MUI_ReleasePen(REG(a0, struct MUI_RenderInfo *mri), REG(d0, LONG pen));
ASM VOID LIB_MUI_RemoveClipping(REG(a0, struct MUI_RenderInfo *mri), REG(a1,APTR handle));
ASM VOID LIB_MUI_RemoveClipRegion(REG(a0, struct MUI_RenderInfo *mri), REG(a1, APTR handle));
ASM LONG LIB_MUI_RequestA(REG(d0, APTR app), REG(d1, APTR win), REG(d2, LONGBITS flags), REG(a0, CONST_STRPTR title), REG(a1, CONST_STRPTR gadgets), REG(a2, CONST_STRPTR format), REG(a3, APTR params));
ASM VOID LIB_MUI_RequestIDCMP(REG(a0, Object *obj), REG(d0, ULONG flags));
ASM LONG LIB_MUI_SetError(REG(d0,LONG num));

ULONG VARARGS68K _ZuneMaster_Obtain(struct ZuneMasterIFace *Self)
{
    return Self->Data.RefCount++;
}

ULONG VARARGS68K _ZuneMaster_Release(struct ZuneMasterIFace *Self)
{
    return Self->Data.RefCount++;
}

Object *VARARGS68K _ZuneMaster_MUI_NewObjectA(struct ZuneMasterIFace *iface, CONST_STRPTR classname, struct TagItem * tags)
{
    return LIB_MUI_NewObjectA((STRPTR)classname, tags);
}

Object *VARARGS68K _ZuneMaster_MUI_NewObject(struct ZuneMasterIFace *Self, CONST_STRPTR classname, ...)
{
    Object *obj;
    va_list ap;
    struct TagItem *tags;

    va_startlinear(ap, classname);
    tags = va_getlinearva(ap, struct TagItem *);
    obj = Self->MUI_NewObjectA(classname, tags);
    va_end(ap);
    return obj;
}

VOID VARARGS68K _ZuneMaster_MUI_DisposeObject(struct ZuneMasterIFace *iface, Object * obj)
{
    LIB_MUI_DisposeObject(obj);
}

LONG VARARGS68K _ZuneMaster_MUI_RequestA(struct ZuneMasterIFace *iface, APTR app, APTR win, ULONG flags, CONST_STRPTR title, CONST_STRPTR gadgets, CONST_STRPTR format, APTR params)
{
    return LIB_MUI_RequestA(app, win, flags, title, gadgets, format, params);
}

LONG VARARGS68K _ZuneMaster_MUI_Request(struct ZuneMasterIFace *Self, APTR app, APTR win, ULONG flags, CONST_STRPTR title, CONST_STRPTR gadgets, CONST_STRPTR format, ...)
{
    LONG res;
    va_list ap;
    APTR params;

    va_startlinear(ap, format);
    params = va_getlinearva(ap, APTR);
    res = Self->MUI_RequestA(app,win,flags,title,gadgets,format,params);
    va_end(ap);
    return res;
}

APTR VARARGS68K _ZuneMaster_MUI_AllocAslRequest(struct ZuneMasterIFace *iface, ULONG reqType, struct TagItem * tagList)
{
    return LIB_MUI_AllocAslRequest(reqType,tagList);
}

APTR VARARGS68K _ZuneMaster_MUI_AllocAslRequestTags(struct ZuneMasterIFace *Self, ULONG reqType, ...)
{
    APTR res;
    va_list ap;
    struct TagItem *tags;

    va_startlinear(ap, reqType);
    tags = va_getlinearva(ap, struct TagItem *);
    res = Self->MUI_AllocAslRequest(reqType, tags);
    va_end(ap);
    return res;
}

BOOL VARARGS68K _ZuneMaster_MUI_AslRequest(struct ZuneMasterIFace *iface, APTR requester, struct TagItem * tagList)
{
    return LIB_MUI_AslRequest(requester, tagList);
}

BOOL VARARGS68K _ZuneMaster_MUI_AslRequestTags(struct ZuneMasterIFace *Self, APTR requester, ...)
{
    BOOL res;
    va_list ap;
    struct TagItem *tags;

    va_startlinear(ap, requester);
    tags = va_getlinearva(ap, struct TagItem *);
    res = Self->MUI_AslRequest(requester, tags);
    va_end(ap);
    return res;
}

VOID VARARGS68K _ZuneMaster_MUI_FreeAslRequest(struct ZuneMasterIFace *iface, APTR requester)
{
    LIB_MUI_FreeAslRequest(requester);
}

LONG VARARGS68K _ZuneMaster_MUI_Error(struct ZuneMasterIFace *iface)
{
    return LIB_MUI_Error();
}

LONG VARARGS68K _ZuneMaster_MUI_SetError(struct ZuneMasterIFace *iface, LONG num)
{
    return LIB_MUI_SetError(num);
}

struct IClass *VARARGS68K _ZuneMaster_MUI_GetClass(struct ZuneMasterIFace *iface, CONST_STRPTR classname)
{
    return LIB_MUI_GetClass(classname);
}

VOID VARARGS68K _ZuneMaster_MUI_FreeClass(struct ZuneMasterIFace *iface, struct IClass * classptr)
{
    LIB_MUI_FreeClass(classptr);
}

VOID VARARGS68K _ZuneMaster_MUI_RequestIDCMP(struct ZuneMasterIFace *iface, Object * obj, ULONG flags)
{
    LIB_MUI_RequestIDCMP(obj,flags);
}

VOID VARARGS68K _ZuneMaster_MUI_RejectIDCMP(struct ZuneMasterIFace *iface, Object * obj, ULONG flags)
{
    LIB_MUI_RejectIDCMP(obj,flags);
}

VOID VARARGS68K _ZuneMaster_MUI_Redraw(struct ZuneMasterIFace *iface, Object * obj, ULONG flags)
{
    LIB_MUI_Redraw(obj,flags);
}

struct MUI_CustomClass *VARARGS68K _ZuneMaster_MUI_CreateCustomClass(struct ZuneMasterIFace *iface, struct Library * base, CONST_STRPTR supername, struct MUI_CustomClass * supermcc, LONG datasize, APTR dispatcher)
{
    return LIB_MUI_CreateCustomClass(base,(STRPTR)supername,supermcc,datasize,dispatcher);
}

BOOL VARARGS68K _ZuneMaster_MUI_DeleteCustomClass(struct ZuneMasterIFace *iface, struct MUI_CustomClass * mcc)
{
    return LIB_MUI_DeleteCustomClass(mcc);
}

Object *VARARGS68K _ZuneMaster_MUI_MakeObjectA(struct ZuneMasterIFace *iface, LONG type, ULONG * params)
{
    return LIB_MUI_MakeObjectA(type,params);
}

Object *VARARGS68K _ZuneMaster_MUI_MakeObject(struct ZuneMasterIFace *Self, LONG type, ...)
{
    Object *res;
    va_list ap;
    ULONG *params;

    va_startlinear(ap, type);
    params = va_getlinearva(ap, ULONG *);
    res = Self->MUI_MakeObjectA(type,params);
    va_end(ap);
    return res;

}

BOOL VARARGS68K _ZuneMaster_MUI_Layout(struct ZuneMasterIFace *iface, Object * obj, LONG left, LONG top, LONG width, LONG height, ULONG flags)
{
    return LIB_MUI_Layout(obj,left,top,width,height,flags);
}

LONG VARARGS68K _ZuneMaster_MUI_ObtainPen(struct ZuneMasterIFace *iface, struct MUI_RenderInfo * mri, struct MUI_PenSpec * spec, ULONG flags)
{
    return LIB_MUI_ObtainPen(mri,spec,flags);
}

VOID VARARGS68K _ZuneMaster_MUI_ReleasePen(struct ZuneMasterIFace *iface, struct MUI_RenderInfo * mri, LONG pen)
{
    LIB_MUI_ReleasePen(mri,pen);
}

APTR VARARGS68K _ZuneMaster_MUI_AddClipping(struct ZuneMasterIFace *iface, struct MUI_RenderInfo * mri, WORD left, WORD top, WORD width, WORD height)
{
    return LIB_MUI_AddClipping(mri,left,top,width,height);
}

VOID VARARGS68K _ZuneMaster_MUI_RemoveClipping(struct ZuneMasterIFace *iface, struct MUI_RenderInfo * mri, APTR handle)
{
    LIB_MUI_RemoveClipping(mri,handle);
}

APTR VARARGS68K _ZuneMaster_MUI_AddClipRegion(struct ZuneMasterIFace *iface, struct MUI_RenderInfo * mri, struct Region * r)
{
    return LIB_MUI_AddClipRegion(mri,r);
}

VOID VARARGS68K _ZuneMaster_MUI_RemoveClipRegion(struct ZuneMasterIFace *iface, struct MUI_RenderInfo * mri, APTR handle)
{
    LIB_MUI_RemoveClipRegion(mri,handle);
}

BOOL VARARGS68K _ZuneMaster_MUI_BeginRefresh(struct ZuneMasterIFace *iface, struct MUI_RenderInfo * mri, ULONG flags)
{
    return LIB_MUI_BeginRefresh(mri,flags);
}

VOID VARARGS68K _ZuneMaster_MUI_EndRefresh(struct ZuneMasterIFace *iface, struct MUI_RenderInfo * mri, ULONG flags)
{
    LIB_MUI_EndRefresh(mri,flags);
}

/* ------------------- Manager Interface ------------------------ */
/* These are generic. Replace if you need more fancy stuff */
static LONG _manager_Obtain(struct LibraryManagerInterface *Self)
{
    return Self->Data.RefCount++;
}

static ULONG _manager_Release(struct LibraryManagerInterface *Self)
{
    return Self->Data.RefCount--;
}

/* Manager interface vectors */
static CONST void *lib_manager_vectors[] =
{
    (void *)_manager_Obtain,
    (void *)_manager_Release,
    (void *)0,
    (void *)0,
    (void *)libOpen,
    (void *)libClose,
    (void *)libExpunge,
    (void *)0,
    (void *)-1,
};

/* "__library" interface tag list */
static CONST struct TagItem lib_managerTags[] =
{
    {MIT_Name,             (ULONG)"__library"},
    {MIT_VectorTable,      (ULONG)lib_manager_vectors},
    {MIT_Version,          1},
    {TAG_DONE,             0}
};

/* ------------------- Library Interface(s) ------------------------ */

#include "zunemaster_vectors.c"

/* Uncomment this line (and see below) if your library has a 68k jump table */
extern ULONG VecTable68K;

static CONST struct TagItem mainTags[] =
{
    {MIT_Name,              (uint32)"main"},
    {MIT_VectorTable,       (uint32)main_vectors},
    {MIT_Version,           1},
    {TAG_DONE,              0}
};

static CONST uint32 libInterfaces[] =
{
    (uint32)lib_managerTags,
    (uint32)mainTags,
    (uint32)0
};

static CONST struct TagItem libCreateTags[] =
{
    {CLT_DataSize,         (uint32)(sizeof(struct MUIMasterBase_intern))},
    {CLT_InitFunc,         (uint32)libInit},
    {CLT_Interfaces,       (uint32)libInterfaces},
    {CLT_Vector68K,        (uint32)&VecTable68K},
    {TAG_DONE,             0}
};


/* ------------------- ROM Tag ------------------------ */
static const __attribute__((used)) struct Resident lib_res =
{
    RTC_MATCHWORD,
    (struct Resident *)&lib_res,
    (APTR)&lib_res+1,
    RTF_NATIVE|RTF_AUTOINIT,
    VERSION,
    NT_LIBRARY,
    0, /* PRI */
    "zunemaster.library",
    &"\0$VER: "VSTRING[7],
    (APTR)libCreateTags
};
