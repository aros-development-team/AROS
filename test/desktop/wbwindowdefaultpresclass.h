#ifndef WBWINDOWDEFAULTPRESCLASS_H
#define WBWINDOWDEFAULTPRESCLASS_H

#include <exec/types.h>

#define WBA_WinPresentation_HorizSize TAG_USER+1101 /* (I-G) */
#define WBA_WinPresentation_VertSize TAG_USER+1102  /* (I-G) */
#define WBA_WinPresentation_ViewPointX TAG_USER+1104 /* (ISG) */
#define WBA_WinPresentation_ViewPointY TAG_USER+1105 /* (ISG) */
#define WBA_WinPresentation_SemanticObject TAG_USER+1106
#define WBA_WinPresentation_Clicked TAG_USER+1107

struct MemberNode
{
	struct MinNode m_Node;
	Object *m_member;
};

struct WBWinDefPresClassData
{
	struct MinList memberList;
	Object *justRemoved;
	Object *vertProp, *horizProp;
	ULONG viewPointX, viewPointY;
	ULONG virtualWidth, virtualHeight;
	struct IntuiMessage *clicked;
/* summary data - make this better */
	ULONG lastAddedX, lastAddedY, lastAddedW, lastAddedH;
	ULONG biggestWidthInLastCol, lastColH;
	struct Rectangle clippingRectangle;
	Object *semanticObject;
};

#endif

