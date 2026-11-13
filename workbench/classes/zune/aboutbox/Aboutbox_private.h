#ifndef _ABOUTBOX_PRIVATE_H
#define _ABOUTBOX_PRIVATE_H

/*
    Copyright © 2011, Thore Böckelmann. All rights reserved.
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/mui.h>

struct ExpandString
{
    char *string;
    ULONG size;
};

struct Data
{
    Object *logoGroup;
    Object *logo;
    Object *appInfoText;
    Object *creditsText;

    ULONG fallbackMode;
    CONST_STRPTR logoFile;
    CONST_APTR logoData;
    CONST_STRPTR credits;
    CONST_STRPTR build;

    struct ExpandString parsedCredits;
    struct ExpandString appInfo;

    char progName[256];
    char title[128];
};

struct Icon_Data
{
    struct DiskObject *icon;
};

extern struct MUI_CustomClass *icon_mcc;

#endif /* _ABOUTBOX_PRIVATE_H_ */
