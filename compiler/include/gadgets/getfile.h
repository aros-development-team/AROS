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

#define GETFILE_TitleText       (GETFILE_Dummy + 0x0001) /* Requester title */
#define GETFILE_File            (GETFILE_Dummy + 0x0002) /* Selected filename */
#define GETFILE_Drawer          (GETFILE_Dummy + 0x0003) /* Initial directory */
#define GETFILE_Pattern         (GETFILE_Dummy + 0x0004) /* File filter pattern */
#define GETFILE_DoSaveMode      (GETFILE_Dummy + 0x0005) /* Save-mode requester */
#define GETFILE_DoMultiSelect   (GETFILE_Dummy + 0x0006) /* Allow multi-select */
#define GETFILE_DoPatterns      (GETFILE_Dummy + 0x0007) /* Show pattern gadget */
#define GETFILE_RejectIcons     (GETFILE_Dummy + 0x0008) /* Hide .info files */
#define GETFILE_FullFile        (GETFILE_Dummy + 0x0009) /* Full path result */
#define GETFILE_ReadOnly        (GETFILE_Dummy + 0x000A) /* Non-editable path */
#define GETFILE_FilterFunc      (GETFILE_Dummy + 0x000B) /* Custom filter hook */
#define GETFILE_DrawersOnly     (GETFILE_Dummy + 0x000C) /* Drawers only mode */
#define GETFILE_FullFileExpand  (GETFILE_Dummy + 0x000D) /* Expand assigns in path */

#ifndef GetFileObject
#define GetFileObject   NewObject(NULL, GETFILE_CLASSNAME
#endif
#ifndef GetFileEnd
#define GetFileEnd      TAG_END)
#endif

#endif /* GADGETS_GETFILE_H */
