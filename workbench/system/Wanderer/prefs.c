/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <string.h>

#include "iconwindow.h"
#include "support.h"
#include "prefs.h"


STRPTR rootBG, dirsBG; // FIXME
extern Object *root_iconwnd; // FIXME

VOID LoadPrefs(VOID)
{
    BPTR fh;
    
    if ((fh = Open("ENV:SYS/Wanderer.prefs", MODE_OLDFILE)) != NULL)
    {
        STRPTR buffer = NULL;
        LONG   size;
        
        Seek(fh, 0, OFFSET_END);
        size = Seek(fh, 0, OFFSET_BEGINNING) + 2;
        
        if ((buffer = AllocVec(size, MEMF_ANY)) != NULL)
        {
            if (!ReadLine(fh, buffer, size)) goto end;
            if (rootBG == NULL)
            {
                rootBG = StrDup(buffer);
            }
            else if (strcmp(rootBG, buffer) != 0)
            {
                FreeVec(rootBG);
                rootBG = StrDup(buffer);
                
                if (rootBG != NULL)
                {
                    SET
                    (
                        (Object *) XGET(root_iconwnd, MUIA_IconWindow_IconList),
                        MUIA_Background, (IPTR) rootBG
                    );
                }
            }
            
            if (!ReadLine(fh, buffer, size)) goto end;
            if (dirsBG == NULL)
            {
                dirsBG = StrDup(buffer);
            }
            else if (strcmp(dirsBG, buffer) != 0)
            {
                FreeVec(dirsBG);
                dirsBG = StrDup(buffer);
                
                if (dirsBG != NULL)
                {
                    Object *cstate = (Object*)(((struct List*)XGET(_app(root_iconwnd), MUIA_Application_WindowList))->lh_Head);
                    Object *child;
        
                    while ((child = NextObject(&cstate)))
                    {
                        if (child != root_iconwnd && XGET(child, MUIA_UserData))
                        {
                            SET
                            (
                                (Object *) XGET(child, MUIA_IconWindow_IconList),
                                MUIA_Background, (IPTR) dirsBG
                            );
                        }
                    }
                }
            }
            
end:        FreeVec(buffer);
        }
        
        Close(fh);
    }
}

VOID FreePrefs(VOID)
{
    if (rootBG != NULL) FreeVec(rootBG);
    if (dirsBG != NULL) FreeVec(dirsBG);
}
