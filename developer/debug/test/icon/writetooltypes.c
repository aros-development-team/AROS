/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <workbench/workbench.h>
#include <workbench/icon.h>

#include <proto/icon.h>

#include <stdio.h>

int main(int argc, char **argv)
{
    STRPTR newToolTypes[] =
    {
        "AROS=RULES",
        "Tooltypes are actually free-form text",
        "Ain't that neat?",
        NULL
    };
    
    struct DiskObject *icon = GetIconTags
    (
        "writetooltypes", ICONGETA_FailIfUnavailable, FALSE, TAG_DONE
    );
    
    if (icon != NULL)
    {
        STRPTR *oldToolTypes = icon->do_ToolTypes;
        
        icon->do_ToolTypes = newToolTypes;
        if (!PutIconTags("writetooltypes", icon, TAG_DONE))
        {
            printf("ERROR: Failed to write icon.\n");
        }
        icon->do_ToolTypes = oldToolTypes;
        
        FreeDiskObject(icon);
    }
    else
    {
        printf("ERROR: Failed to open icon for file\n");
        return 20;
    }
        

    return 0;
}
