/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/getfile.h
*/

#ifndef GADGETS_GETFILE_H
#define GADGETS_GETFILE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define GETFILE_CLASSNAME   "getfile.gadget"
#define GETFILE_VERSION     44

#define GETFILE_Dummy       (REACTION_Dummy + 0x60000)

#define GETFILE_TitleText       (GETFILE_Dummy + 1)   /* Requester title text */
#define GETFILE_LeftEdge        (GETFILE_Dummy + 2)   /* Requester left position */
#define GETFILE_TopEdge         (GETFILE_Dummy + 3)   /* Requester top position */
#define GETFILE_Width           (GETFILE_Dummy + 4)   /* Requester width */
#define GETFILE_Height          (GETFILE_Dummy + 5)   /* Requester height */
#define GETFILE_File            (GETFILE_Dummy + 6)   /* File gadget contents */
#define GETFILE_Drawer          (GETFILE_Dummy + 7)   /* Drawer gadget contents */
#define GETFILE_FullFile        (GETFILE_Dummy + 8)   /* Complete path and filename */
#define GETFILE_FullFileExpand  (GETFILE_Dummy + 9)   /* Expand relative paths via NameFromLock */
#define GETFILE_Pattern         (GETFILE_Dummy + 10)  /* Pattern gadget contents */
#define GETFILE_DoSaveMode      (GETFILE_Dummy + 11)  /* Save mode requester */
#define GETFILE_DoMultiSelect   (GETFILE_Dummy + 12)  /* Allow multiple selection */
#define GETFILE_DoPatterns      (GETFILE_Dummy + 13)  /* Display pattern gadget */
#define GETFILE_DrawersOnly     (GETFILE_Dummy + 14)  /* Show only drawers */
#define GETFILE_FilterFunc      (GETFILE_Dummy + 15)  /* Hook to filter files */
#define GETFILE_RejectIcons     (GETFILE_Dummy + 16)  /* Hide .info files */
#define GETFILE_RejectPattern   (GETFILE_Dummy + 17)  /* Reject files matching pattern */
#define GETFILE_AcceptPattern   (GETFILE_Dummy + 18)  /* Accept only matching files */
#define GETFILE_FilterDrawers   (GETFILE_Dummy + 19)  /* Apply pattern filter to drawers */
#define GETFILE_Filelist        (GETFILE_Dummy + 20)  /* List of selected files (multi-select) */
#define GETFILE_LBNodeStructs   (GETFILE_Dummy + 21)  /* Filelist uses ListBrowserNode structs */
#define GETFILE_ReadOnly        (GETFILE_Dummy + 22)  /* Non-editable display mode */
#define GETFILE_FilePartOnly    (GETFILE_Dummy + 23)  /* Show file part only (readonly mode) */

/* getfile.gadget methods */
#define GFILE_REQUEST   (0x620001L)
#define GFILE_FREELIST  (0x620002L)

/* Method structures */
struct gfileRequest
{
    ULONG MethodID;
    struct Window *gfile_Window;
};

struct gfileFreelist
{
    ULONG MethodID;
    struct List *gfile_Filelist;
};

/* Convenience macros */
#define gfRequestFile(obj, win)     DoMethod(obj, GFILE_REQUEST, win)
#define gfRequestDir(obj, win)      DoMethod(obj, GFILE_REQUEST, win)
#define gfFreeFilelist(obj, list)   DoMethod(obj, GFILE_FREELIST, list)

#ifndef GetFileObject
#define GetFileObject   NewObject(NULL, GETFILE_CLASSNAME
#endif
#ifndef GetFileEnd
#define GetFileEnd      TAG_END)
#endif

#endif /* GADGETS_GETFILE_H */
