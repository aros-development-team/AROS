#ifndef WBAPPLICATIONCLASS_H
#define WBAPPLICATIONCLASS_H

#define WBA_Application_MsgPort TAG_USER+201

#define WBM_Application_Input TAG_USER+1201

struct WBInterMessage
{
	struct Message im_message;
	Object *requester;
	ULONG methodID;
	ULONG numArgs;
	ULONG *args;
};

struct WBAppInputData
{
	Msg msg;
	LONGBITS *signals;
	ULONG breakSig;
};

struct WindowNode
{
	struct MinNode wn_Node;
	struct Object *wn_window;
};

struct WBApplicationClassData
{
	Object *zuneApplication;
	struct MsgPort *wbPort;
	struct MinList windowList;
};

#endif

