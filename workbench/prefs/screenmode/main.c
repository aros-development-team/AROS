#define MUIMASTER_YES_INLINE_STDARG
#define DEBUG 0

#include <dos/dos.h>

#include <proto/muimaster.h>
#include <proto/intuition.h>

#include <zune/systemprefswindow.h>

#include "smeditor.h"

int main()
{
    Object *app, *win;
    
    app = ApplicationObject,
        MUIA_Application_Title, (IPTR)"Test List Class",

        SubWindow, (IPTR)(win = SystemPrefsWindowObject,
            WindowContents, (IPTR) SMEditorObject,
            End,
	End),
    End;

    if (app)
    {	     
        set(win, MUIA_Window_Open, TRUE);
	
        DoMethod(app, MUIM_Application_Execute);
    
        MUI_DisposeObject(app);
        
	return RETURN_ERROR;        
    }
    
    return RETURN_OK;
}


