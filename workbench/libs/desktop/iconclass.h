
#ifndef ICONCLASS_H
#define ICONCLASS_H

#define IA_BASE   TAG_USER+3200

#define IA_DiskObject IA_BASE+1
#define IA_Label      IA_BASE+2
#define IA_Selected   IA_BASE+3
#define IA_Executed   IA_BASE+4

struct IconClassData
{
	struct DiskObject *diskObject;
	UBYTE *label;
	Object *imagePart;
	Object *labelPart;
	BOOL selected;
	ULONG lastClickSecs, lastClickMicros;
};

struct __dummyIconData__
{
    struct MUI_NotifyData mnd;
    struct IconClassData icd;
};

#define iconData(obj) (&(((struct __dummyIconData__ *)(obj))->icd))

#define _selected(obj)    (iconData(obj)->selected)




#endif
