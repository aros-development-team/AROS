#ifndef WBICONCLASS_H
#define WBICONCLASS_H

#define WBA_Icon_Selected TAG_USER+501 /* ISG */
#define WBA_Icon_Name     TAG_USER+502 /* ISG */
#define WBA_Icon_Type     TAG_USER+503 /* ISG */
#define WBA_Icon_DoubleClicked TAG_USER+504

#define WBM_Icon_HandleInput TAG_USER+601
#define WBM_Icon_Execute TAG_USER+602

struct WBIconClassData
{
	BOOL selected;
	char *name;
	LONG type;
};

struct IconHandleInputMethodData
{
	Msg methodID;
	struct IntuiMessage *imsg;
};

#endif

