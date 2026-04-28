/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction getfile.gadget - Internal definitions
*/

#ifndef GETFILE_INTERN_H
#define GETFILE_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/getfile.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#define G(obj)  ((struct Gadget *)(obj))

struct GetFileData
{
    STRPTR          gfd_TitleText;      /* Requester title */
    STRPTR          gfd_File;           /* Current file name */
    STRPTR          gfd_Drawer;         /* Current drawer path */
    STRPTR          gfd_Pattern;        /* File pattern */
    BOOL            gfd_DoSaveMode;     /* Save mode requester */
    BOOL            gfd_DoMultiSelect;  /* Allow multiple selection */
    BOOL            gfd_DoPatterns;     /* Show pattern gadget */
    BOOL            gfd_RejectIcons;    /* Reject .info files */
    STRPTR          gfd_FullFile;       /* Full path to file */
    BOOL            gfd_ReadOnly;       /* Read-only text field */
    BOOL            gfd_DrawersOnly;    /* Only show drawers */
};

#endif /* GETFILE_INTERN_H */
