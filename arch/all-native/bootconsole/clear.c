#include <bootconsole.h>

/* Clear the console */
void con_Clear(void)
{
    switch (scr_Type)
    {
    case SCR_TEXT:
    	txt_Clear();
    	break;

    case SCR_GFX:
    	fb_Clear();
    	break;
    }
}
