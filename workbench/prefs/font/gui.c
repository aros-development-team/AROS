#include <proto/gadtools.h>
#include <libraries/gadtools.h>

#include <stdio.h>

#include "global.h"

extern struct Library *GadToolsBase;

struct NewMenu windowMenus[] =
{
 { NM_TITLE,	"Project",	  0, 0, 0, 0, },
 { NM_ITEM,	"Open...",	"O", 0, 0, MENU_ID_OPEN, },
 { NM_ITEM,	"Save As...",	"A", 0, 0, MENU_ID_SAVE, },
 { NM_ITEM,	NM_BARLABEL,	  0, 0, 0, 0, },
 { NM_ITEM,	"Quit",		"Q", 0, 0, MENU_ID_QUIT, },
 { NM_END,	NULL,		  0, 0, 0, 0, },
};

/* setupMenus() - expects an APTR VisualInfo pointer - returns a menu pointer
   if successful, otherwise FALSE */
struct Menu *setupMenus(APTR visualInfo)
{
 struct Menu *menuPtr = NULL; /* Better set this to zero just in case */

 if(menuPtr = CreateMenus(windowMenus, TAG_END))
 {
  if(LayoutMenus(menuPtr, visualInfo, TAG_END))
   return(menuPtr);
  else
  {
   printf("Unable to layout menus!\n");
   FreeMenus(menuPtr);
   return(FALSE);
  }
 }
 else
 {
  printf("Unable to create menus!\n");
  return(FALSE);
 }
}
