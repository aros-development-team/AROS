/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef ICONCLASS_H
#define ICONCLASS_H

#define IA_BASE   TAG_USER+3200

#define IA_DiskObject IA_BASE+1
#define IA_Label      IA_BASE+2
#define IA_Selected   IA_BASE+3
#define IA_Executed   IA_BASE+4
#define IA_Directory  IA_BASE+5
#define IA_Comment    IA_BASE+6 /* (-SG) */
#define IA_Script       IA_BASE+7
#define IA_Pure         IA_BASE+8
#define IA_Archived     IA_BASE+9
#define IA_Readable     IA_BASE+10
#define IA_Writeable    IA_BASE+11
#define IA_Executable   IA_BASE+12
#define IA_Deleteable   IA_BASE+13

#define _comment(obj)  (iconData(obj)->comment)
#define _script(obj)  (iconData(obj)->script)
#define _pure(obj)  (iconData(obj)->pure)
#define _archived(obj)  (iconData(obj)->archived)
#define _readable(obj)  (iconData(obj)->readable)
#define _writeable(obj)  (iconData(obj)->writeable)
#define _executable(obj)  (iconData(obj)->executable)
#define _deleteable(obj)  (iconData(obj)->deleteable)

struct IconClassData
{
	struct DiskObject *diskObject;
	UBYTE *label, *directory;
	Object *imagePart;
	Object *labelPart;
	BOOL selected;
	ULONG lastClickSecs, lastClickMicros;
	UBYTE *comment;
	BOOL script, pure, archived, readable, writeable, executable, deleteable;
};

// err.. what can i say?  this is temporary
// iconclass will be changed so it inherits from a more generic
// version of iconcontainer instead of group.mui
struct MUI_GroupData
{
    Object      *family;
    struct Hook *layout_hook;
    ULONG        flags;
    ULONG        columns;
    ULONG        rows;
    LONG         active_page;
    ULONG        horiz_spacing;
    ULONG        vert_spacing;
    ULONG        num_childs;
    ULONG        horiz_weight_sum;
    ULONG        vert_weight_sum;
    ULONG        update; /* for MUI_Redraw() 1 - do not redraw the frame, 2 - the virtual pos has changed */
    struct MUI_EventHandlerNode ehn;
    LONG virt_offx, virt_offy; /* diplay offsets */
    LONG old_virt_offx, old_virt_offy; /* Saved virtual positions, used for update == 2 */
    LONG virt_mwidth,virt_mheight; /* The complete width */
    LONG saved_minwidth,saved_minheight;
    LONG dont_forward_get; /* Setted temporary to 1 so that the get method is not forwarded */
    LONG dont_forward_methods; /* Setted temporary to 1, meaning that the methods are not forwarded to the group's children */
};
//

struct __dummyIconData__
{
    struct MUI_NotifyData mnd;
	struct MUI_AreaData   mad;
	struct MUI_GroupData mgd;
	struct IconClassData icd;
};

#define iconData(obj) (&(((struct __dummyIconData__ *)(obj))->icd))

#define _selected(obj)    (iconData(obj)->selected)
#define _diskobject(obj)  (iconData(obj)->diskObject)



#endif
