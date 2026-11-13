/*
    Copyright (C) 2011, The AROS Development Team. All rights reserved.

    Desc: Workbook wrapper for CLI
*/

#include <proto/exec.h>

extern ULONG WorkbookMain(void);

int main(int argc, char **argv)
{
    return WorkbookMain();
}
