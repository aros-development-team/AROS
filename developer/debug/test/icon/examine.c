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
    const char * const typeNames[] =
    {
        NULL,        /* not used */
        "WBDISK",    /* 1 */
        "WBDRAWER",  /* 2 */
        "WBTOOL",    /* 3 */
        "WBPROJECT", /* 4 */
        "WBGARBAGE", /* 5 */
        "WBDEVICE",  /* 6 */
        "WBKICK",    /* 7 */
        "WBAPPICON"  /* 8 */
    };
            
    if (argc == 2)
    {
        LONG               isDefault = FALSE;
        struct DiskObject *icon      = GetIconTags
        (
            argv[1],
            ICONGETA_FailIfUnavailable,        FALSE, 
            ICONGETA_IsDefaultIcon,     (IPTR) &isDefault,
            TAG_DONE
        );
        
        if (icon != NULL)
        {
            printf
            (
                "Name:          %s\n"
                "Fake icon:     %s\n"
                "Type:          %s (%d)\n"
                "Default tool:  %s\n"
                "Position:      x = %ld (0x%lx), y = %ld (0x%lx)\n"
                "Drawer data:   %s\n"
                "Stack size:    %ld\n"
                "Tooltypes:     %s\n\n",
                argv[1], isDefault ? "yes" : "no",
                typeNames[icon->do_Type], icon->do_Type, icon->do_DefaultTool,
                (long)icon->do_CurrentX, (unsigned long)icon->do_CurrentX, 
                (long)icon->do_CurrentY, (unsigned long)icon->do_CurrentY,
                icon->do_DrawerData != NULL ? "yes" : "no",
                (long)icon->do_StackSize,
                icon->do_ToolTypes != NULL ? "yes" : "no"
            );
            
            if (icon->do_ToolTypes != NULL)
            {
                char *tt = NULL;
                int   i  = 0;
                
                printf("Tooltype data:\n");
                
                while ((tt = icon->do_ToolTypes[i]) != NULL)
                {
                    printf("%s\n", tt);
                    i++;
                }
            }
            
            FreeDiskObject(icon);
        }
        else
        {
            printf("ERROR: Failed to open icon.\n");
            return 20;
        }
    }
    else
    {
        printf("Usage: %s <name>    (without trailing \".info\")\n", argv[0]);
    }
    
    return 0;
}
