#ifndef ICONCONTAINERCLASS_H
#define ICONCONTAINERCLASS_H


#define ICA_BASE TAG_USER+1000

#define ICA_VertScroller  ICA_BASE+1
#define ICA_HorizScroller ICA_BASE+2
#define ICA_ScrollToHoriz ICA_BASE+3
#define ICA_ScrollToVert  ICA_BASE+4

struct MemberNode
{
	struct MinNode m_Node;
	Object *m_Object;
};

struct IconContainerClassData
{
	/* this list is sorted horizontally (rightwards), then
	   vertically (downwards) */
	struct MinList memberList;
	BOOL searchForGaps;
	ULONG thisColumnWidth, thisColumnHeight;
	ULONG virtualWidth, virtualHeight;
	Object *vertProp, *horizProp;
	LONG xView, yView;
	LONG lastXView, lastYView;
	BOOL horizScroll, vertScroll;
	ULONG visibleWidth, visibleHeight;
	LONG heightAdjusted, widthAdjusted;
};

#define ICONSPACINGX 10
#define ICONSPACINGY 10

#endif

