/*
    Copyright © 2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Structures necessary to manage the Desktop
    Lang: English
*/

/*********************************************************************************************/

int OpenDesktop(ULONG what);
int CloseDesktop(ULONG what);

/*********************************************************************************************/

struct Desktop
{
    BOOL Backdrop;
};

#define DESKTOP_All		~0
#define DESKTOP_Main		(1<<0)
#define DESKTOP_Filemanager	(1<<1)

/*********************************************************************************************/

extern struct Desktop Desktop;

/*********************************************************************************************/
