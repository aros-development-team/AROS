#ifndef GUI_H
#define GUI_H

#include <exec/nodes.h>
#include <intuition/classusr.h>

enum {
	GB_FIRST=0,
	GB_ADD_ENTRY=GB_FIRST,
	GB_REMOVE_ENTRY,
	GB_CREATE_TABLE,
	GB_CHANGE_TYPE,
	GB_RESIZE_MOVE,
	GB_PARENT,
	GB_RENAME,
	GB_DOSENVEC,
	GB_SWITCHES,
	GB_SAVE_CHANGES,
	GB_EXIT,
	GB_LAST=GB_EXIT
};

struct GUIGadgets {
	Object *text;
	Object *leftlv;
	Object *rightlv;
	Object *buttons[11];
};


struct ListNode {
	struct Node ln;
	struct List list;
	struct List history;
	struct ListNode *parent;
	ULONG flags;              /* see below */
	UWORD change_count;       /* number of changes on this node or children*/
};

#define LNT_Root      (0)
#define LNT_Parent    (1)
#define LNT_Device    (2)
#define LNT_Harddisk  (3)
#define LNT_Partition (4)

#define LNF_Listable   (1<<0)
#define LNF_Unused1    (1<<1)
#define LNF_ToSave     (1<<2) /* entries has been changed and must be saved */
#define LNF_Invalid    (1<<3)
#define LNF_IntermedChange (1<<4)

LONG initGUI(void);
void deinitGUI(void);
BOOL QuitGUI(ULONG *);
LONG InitListNode(struct ListNode *, struct ListNode *);
void UninitListNode(struct ListNode *);
void InsertList(Object *, struct ListNode *);
void ShowList(Object *, struct List *);

#endif /* GUI_H */

