/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Workbook wrapper for CLI
    Lang: english
*/

#include <proto/exec.h>

extern ULONG WorkbookMain(void);

int main(int argc, char **argv)
{
    return WorkbookMain();
}
