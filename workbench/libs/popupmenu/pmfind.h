//
// pmfind.h
//
// PopupMenu Library - Menu item searching
//
// Copyright (C)2000 Henrik Isaksson <henrik@boing.nu>
// All Rights Reserved.
//

#ifndef PM_FIND_H
#define PM_FIND_H

struct PM_Window;
struct PopupMenu;

// Find a selectable item in a menu, no submenu recursion, but group recursion.
struct PopupMenu *PM_FindNextSelectable(struct PM_Window *a, struct PopupMenu *pm, BOOL *found);
struct PopupMenu *PM_FindFirstSelectable(struct PopupMenu *pm);
struct PopupMenu *PM_FindPrevSelectable(struct PM_Window *a, struct PopupMenu *pm, BOOL *found);
struct PopupMenu *PM_FindLastSelectable(struct PopupMenu *pm);

// Find item 'ID' in menu 'base'. Recursive.
struct PopupMenu *PM_FindID(struct PopupMenu *base, ULONG ID);

// Find last item in a menu. Recursive.
struct PopupMenu *PM_FindLast(struct PopupMenu *base);

// Find item with command key 'key' in menu 'base'. Recursive.
struct PopupMenu *PM_FindItemCommKey(struct PopupMenu *base, UBYTE key);

// Find item before item 'ID' in menu 'base'. Recursive.
struct PopupMenu *PM_FindBeforeID(struct PopupMenu *base, ULONG ID);

// Find item before item 'item' :) in menu 'base'. Recursive.
struct PopupMenu *PM_FindBefore(struct PopupMenu *base, struct PopupMenu *item);

// Find item 'ID' and return its checkmark state. Returns -5L if not found.
BOOL __saveds ASM PM_ItemChecked(register __a1 struct PopupMenu *pm GNUCREG(a1),
    register __d1 ULONG ID GNUCREG(d1));
    
// Alter the states of a list of items. (Execute an IDList program)
void __saveds ASM PM_AlterState(register __a1 struct PopupMenu *base GNUCREG(a1),
    register __a2 struct PM_IDLst *ids GNUCREG(a2),
    register __d1 UWORD action GNUCREG(d1));

// Exported PM_FindID
struct PopupMenu * __saveds ASM PM_FindItem(register __a1 struct PopupMenu *menu GNUCREG(a1),
    register __d1 ULONG ID GNUCREG(d1));
    
struct PopupMenu *PM_FindSortedInsertPoint(struct PopupMenu *pm, struct PopupMenu *fm);

#endif /* PM_FIND_H */
