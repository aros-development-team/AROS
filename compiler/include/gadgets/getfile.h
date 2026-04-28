/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/getfile.h
*/

#ifndef GADGETS_GETFILE_H
#define GADGETS_GETFILE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define GETFILE_CLASSNAME   "gadgets/getfile.gadget"
#define GETFILE_VERSION     44

#define GETFILE_Dummy       (TAG_USER + 0x80000)

#define GETFILE_TitleText       (GETFILE_Dummy + 0x0001)
#define GETFILE_File            (GETFILE_Dummy + 0x0002)
#define GETFILE_Drawer          (GETFILE_Dummy + 0x0003)
#define GETFILE_Pattern         (GETFILE_Dummy + 0x0004)
#define GETFILE_DoSaveMode      (GETFILE_Dummy + 0x0005)
#define GETFILE_DoMultiSelect   (GETFILE_Dummy + 0x0006)
#define GETFILE_DoPatterns      (GETFILE_Dummy + 0x0007)
#define GETFILE_RejectIcons     (GETFILE_Dummy + 0x0008)
#define GETFILE_FullFile        (GETFILE_Dummy + 0x0009)
#define GETFILE_ReadOnly        (GETFILE_Dummy + 0x000A)
#define GETFILE_FilterFunc      (GETFILE_Dummy + 0x000B)
#define GETFILE_DrawersOnly     (GETFILE_Dummy + 0x000C)
#define GETFILE_FullFileExpand  (GETFILE_Dummy + 0x000D)

#define GetFileObject   NewObject(NULL, GETFILE_CLASSNAME
#define GetFileEnd      TAG_END)

#endif /* GADGETS_GETFILE_H */
