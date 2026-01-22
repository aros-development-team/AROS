#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <clib/exec_protos.h>
#include <exec/memory.h>
#include <clib/alib_protos.h>

struct ObjApp
{
	APTR	App;
	APTR	WI_Usage;
	APTR	LV_TaskList;
	APTR	SL_UpdateTime;
	APTR	TX_Frequency;
	char *	STR_TX_UpdateTime;
	char *	STR_TX_Frequency;
};


extern struct ObjApp * CreateApp(void);
extern void DisposeApp(struct ObjApp *);
