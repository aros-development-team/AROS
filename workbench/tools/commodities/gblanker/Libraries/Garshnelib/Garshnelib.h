#ifndef INTERFACE_H
#define INTERFACE_H

#include <graphics/text.h>

typedef struct _IfcObject
{
	LONG io_Type;
	STRPTR io_Label;
	STRPTR io_Char;
	STRPTR *io_Labels;
	LONG io_Active;
	LONG io_Min;
	LONG io_Max;
	LONG io_Level;
	struct TextAttr io_Attr;
	STRPTR io_Value;
}
IfcObject;

typedef struct _PrefObject
{
	LONG po_Type;
	union
	{
		LONG pou_Active;
		LONG pou_Level;
		struct TextAttr pou_Attr;
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
#define po_Attr   po_Union.pou_Attr
#define po_Value  po_Union.pou_Value
#define po_ModeID po_Union.pou_Display.pod_ModeID
#define po_Depth  po_Union.pou_Display.pod_Depth

#define GAD_CYCLE   1
#define GAD_SLIDER  2
#define GAD_FONT    3
#define GAD_STRING  4
#define GAD_DISPLAY 5
#define GAD_DELIM   6

#define ID_SAVE    7
#define ID_TEST    8
#define ID_CANCEL  9

#define CastAndShift( x ) (( SPFix( SPMul( x, SPFlt( Colors )))) << Shift )
#define PREFSIZE ( sizeof( PrefObject ) * 20 )

#define PREF_OK   0
#define PREF_TEST 1
#define PREF_ERR  2

#endif /* INTERFACE_H */
