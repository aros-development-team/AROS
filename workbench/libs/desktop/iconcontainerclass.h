#ifndef ICONCONTAINERCLASS_H
#define ICONCONTAINERCLASS_H

#define ICA_BASE TAG_USER+1000

#define ICA_VertScroller  ICA_BASE+1
#define ICA_HorizScroller ICA_BASE+2
#define ICA_ScrollToHoriz ICA_BASE+3
#define ICA_ScrollToVert  ICA_BASE+4
#define ICA_JustSelected  ICA_BASE+5

#define ICM_UnselectAll   ICA_BASE+10

struct MemberNode
{
	struct MinNode m_Node;
	Object *m_Object;
};

struct IconContainerClassData
{
	// icons are ordered in the order they were added to the
	// container... the layouter will lay icons in columns
	// and will start a new column when there is no room
	// left (for icon view)
	struct MinList memberList;

	// this is true if the user hasn't moved any icons about
	// enables us to use a more optimized icon layouter if
	// we know where everything is.. otherwise it will search
	// for gaps
	BOOL perfectLayout;

	// only valid when perfectLayout is TRUE.. the current
	// width & height of the
	ULONG thisColumnWidth, thisColumnHeight;

	Object *vertProp, *horizProp;
	LONG xView, yView;
	LONG lastXView, lastYView;

	// one of these is set to true after a scroll
	BOOL horizScroll, vertScroll;

	// visible size of the iconcontainer - same as _mwidth()/_mheight()
	ULONG visibleWidth, visibleHeight;
	// total size of the iconcontainer
	ULONG virtualWidth, virtualHeight;

	LONG heightAdjusted, widthAdjusted;
	BOOL iconSelected;
	BOOL justSelected;
};

#define ICONSPACINGX 10
#define ICONSPACINGY 10

#endif

