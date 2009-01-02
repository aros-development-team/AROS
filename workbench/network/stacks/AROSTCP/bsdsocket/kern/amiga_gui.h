#include "conf.h"

extern ULONG guimask;
extern UBYTE GUI_Running;
extern struct timerequest *gui_timerio;
extern ULONG InternalProc;

struct gui_cnf
{
	STRPTR PanelName;
	STRPTR PanelFont;
};

#define GUICMD_SET_INTERFACE_STATE 0x0001
#define GUICMD_CLOSE		   0x0002

#define GUICMD_MASK 0x0000FFFF

void gui_open(void);
void gui_close(void);
void gui_snapshot(void);
void gui_process_refresh(void);
void gui_process_msg(struct SysLogPacket *msg);
void gui_set_interface_state(struct ifnet *ifp, long state);
void error_request(STRPTR Text, ...) VA68K;

