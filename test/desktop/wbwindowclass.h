#ifndef WBWINDOWCLASS_H
#define WBWINDOWCLASS_H

#include <dos/dos.h>
#include <dos/exall.h>

#define WBA_Window_Directory TAG_USER+1 /* I-G */
#define WBA_Window_Open TAG_USER+2   /* -SG */
#define WBA_Window_Window TAG_USER+3 /* --G */

#define WBM_Window_AddFile TAG_USER+401
#define WBM_Window_ProcessIconSelection TAG_USER+402
#define WBM_Window_HandleInput TAG_USER+403

struct WBAddFileMethodData
{
	Msg msg;
	ULONG numArgs;
	struct ExAllData **args;
};

struct HandleInputMethodData
{
	Msg methodID;
	struct IntuiMessage *imsg;
};

struct IconSelectedNode
{
	struct Node is_Node;
	Object *is_member;
};

struct WBProcessIconSelectionData
{
	Msg methodID;
	Object selected;
};

struct IconNode
{
	struct Node i_Node;
	Object *i_member;
};

struct WBWindowClassData
{
	Object *zuneWindow;
	char *directoryPath;
	BOOL open;
	BOOL multiSelectMode;
	struct List selectionList;
	struct List iconList;
};

#endif

