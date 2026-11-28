/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Default icon image classes - internal definitions
*/

#ifndef ICONIMAGE_INTERN_H
#define ICONIMAGE_INTERN_H

#include <exec/types.h>

struct DiskObject;
struct Screen;

struct IconImageData
{
    struct DiskObject  *iid_Icon;       /* the default icon shown */
    struct Screen      *iid_Screen;     /* screen the image is used on */
    ULONG               iid_ScaleBox;   /* size the icon was laid out for */
    BOOL                iid_LaidOut;
};

#endif /* ICONIMAGE_INTERN_H */
