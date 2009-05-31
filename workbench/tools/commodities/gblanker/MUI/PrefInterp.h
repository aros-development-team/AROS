#ifndef INTERFACE_H
#define INTERFACE_H

#include <graphics/text.h>

typedef struct _PrefObject
{
	LONG po_Type;
	union
	{
		LONG pou_Active;
		LONG pou_Level;
		struct
		{
			struct TextAttr pof_Attr;
			BYTE pof_Name[128-sizeof( struct TextAttr )];
		}
		pou_Font;
		BYTE pou_Value[128];
		struct
		{
			LONG pod_ModeID;
			LONG pod_Depth;
		}
		pou_Display;
	}
	po_Union;
}
PrefObject;

#define po_Active po_Union.pou_Active
#define po_Level  po_Union.pou_Level
#define po_Attr   po_Union.pou_Font.pof_Attr
#define po_Name   po_Union.pou_Font.pof_Name
#define po_Value  po_Union.pou_Value
#define po_ModeID po_Union.pou_Display.pod_ModeID
#define po_Depth  po_Union.pou_Display.pod_Depth

#define ID_SAVE    7
#define ID_TEST    8
#define ID_CANCEL  9

#endif /* INTERFACE_H */
