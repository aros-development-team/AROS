#ifndef DEFAULT_ICONPRESENTATION_H
#define DEFAULT_ICONPRESENTATION_H

#define WBA_IconPresentation_Target TAG_USER+1001
#define WBA_IconPresentation_Border TAG_USER+1002
#define WBA_IconPresentation_Label TAG_USER+1003
#define WBA_IconPresentation_Image TAG_USER+1004
#define WBA_IconPresentation_Placed TAG_USER+1005
#define WBA_IconPresentation_DoubleClicked TAG_USER+1006

struct IconDefPresClassData
{
	Object *target;
	BOOL border;
	char *label;
	BOOL placed;
	Object *imageObject, *labelObject;
	BOOL doubleClicked;
};

#endif
