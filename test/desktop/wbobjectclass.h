#ifndef WBOBJECTCLASS_H
#define WBOBJECTCLASS_H

#include <exec/types.h>
#include <intuition/classusr.h>

#define WBA_Object_Parent TAG_USER+101 /* I-G */
#define WBA_Object_Presentation TAG_USER+102 /* ISG */
#define WBA_Object_ID TAG_USER+103 /* --G */

#define WBM_Object_Added TAG_USER+104

struct WBObjectClassData
{
	Object *parent;
	Object *presentation;
	ULONG objectID;
};

#endif

