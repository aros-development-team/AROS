
#ifndef ICONCLASS_H
#define ICONCLASS_H

#define IA_BASE   TAG_USER+3200

#define IA_DiskObject IA_BASE+1
#define IA_Label      IA_BASE+2

struct IconClassData
{
	struct DiskObject *diskObject;
	UBYTE *label;
	Object *imagePart;
	Object *labelPart;
};


#endif
