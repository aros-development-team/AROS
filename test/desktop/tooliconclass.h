#ifndef TOOLICONCLASS_H
#define TOOLICONCLASS_H

#include <exec/types.h>
#include <intuition/classusr.h>

#define WBA_ToolIcon_NamePart TAG_USER+1 /* ISG */
#define WBA_ToolIcon_PathPart TAG_USER+2 /* ISG */
#define WBA_ToolIcon_Selected TAG_USER+3 /* ISG */
#define WBA_ToolIcon_ParentIcon TAG_USER+4 /* ISG */
//#define WBA_Object_Parent TAG_USER+5 /* ISG */

struct ToolIconClassData
{
	char *namepart, *pathpart;
	BOOL selected;
	Object *parentIcon;
	Object *presentation;
};

#endif

