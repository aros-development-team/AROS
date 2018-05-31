#include "conf.h"

#include <proto/intuition.h>

#include <devices/timer.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#if !defined(__AROS__)
#include <emul/emulinterface.h>
#endif
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <libraries/miamipanel.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/miamipanel.h>
#include <kern/amiga_dhcp.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_log.h>
#include <sys/synch.h>
#include <stdio.h>
#include <syslog.h>

/* Panel preferences */
struct gui_cnf gui_cnf = {NULL};
LONG gui_show[6] = {TRUE, TRUE, TRUE, TRUE, TRUE, TRUE};
ULONG gui_refresh = 1;
long panelx = 50;
long panely = 50;

UBYTE GUI_Running = 0;
ULONG guimask = 0;
struct Library *MiamiPanelBase = NULL;
struct MsgPort *gui_msgport = NULL;
struct timerequest *gui_timerio = NULL;
ULONG InternalProc = 0;

extern struct ifnet *ifnet;
extern struct Task *AmiTCP_Task;
extern TEXT panels_path[];

STRPTR strings[] =
{
	">On",
	">Of",
	">Su",
	"Onl",
	"Off",
	"Sus",
	"Show",
	"Hide",
	"Quit",
	"Onl",
	"Off"
};

void SAVEDS gui_async_op(ULONG code, ULONG unit)
{
	struct ifnet *ifp;

	DGUI(log(LOG_DEBUG,"gui_async_op(%lu, %lu) called", code, unit);)
	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_index == unit) {
			DGUI(log(LOG_DEBUG,"Found ifp = 0x%08lx", ifp);)
			ifupdown(ifp, (code == MIAMIPANELV_CallBack_Code_UnitOffline));
		}
	}
	Forbid();
	InternalProc--;
}

#ifdef __MORPHOS__
#define getarg(x, y) (y)*x++
long callback_function(void)
{
	long *m68k_stack = (ULONG *)REG_A7;
	long code = m68k_stack[1];
	long count = m68k_stack[2];
	ULONG *args = (ULONG *)m68k_stack[3];
#else
#define getarg va_arg
long callback(long code, long count, va_list args)
{
#endif
	ULONG nstr;
	struct SysLogPacket *msg;

	switch (code) {
	case MIAMIPANELV_CallBack_Code_Localize:
		nstr = getarg(args, ULONG) - 5000;
		DGUI(KPrintF("Callback: localize(%lu), result: %s", nstr, strings[nstr]);)
		/* TODO: full localization */
		return (long)strings[nstr];
	case MIAMIPANELV_CallBack_Code_UnitOnline:
	case MIAMIPANELV_CallBack_Code_UnitOffline:
		nstr = getarg(args, ULONG);
		InternalProc++;
		if (!(CreateNewProcTags(NP_Entry, (IPTR)&gui_async_op,
				  NP_Name, (IPTR)"AROSTCP interface control",
#ifdef __MORPHOS__
			          NP_CodeType, CODETYPE_PPC,
			          NP_PPC_Arg1, code,
			          NP_PPC_Arg2, nstr,
#else
/* TODO: Implement passing arguments to gui_async_op() */
#endif
				  TAG_DONE)))
			InternalProc--;
		break;
	case MIAMIPANELV_CallBack_Code_ShowMainGUI:
		/* TODO: Run configuration editor */
		break;
	case MIAMIPANELV_CallBack_Code_HideMainGUI:
		/* In fact we don't have main GUI so we have nothing to hide here.
		   However many panels (MUI.MiamiPanel for example) have "Hide"
		   button and in order to make it usable we simply make it doing
		   the same as "Close panel" */
	case MIAMIPANELV_CallBack_Code_ClosePanel:
		if (msg = (struct SysLogPacket *)GetLogMsg(&logReplyPort)) {
			msg->Level = LOG_GUIMSG | GUICMD_CLOSE;
			PutMsg(logPort, (struct Message *)msg);
		}
		break;
	case MIAMIPANELV_CallBack_Code_QuitMiami:
		Signal(AmiTCP_Task, SIGBREAKF_CTRL_C);
		break;
	default:
		DGUI(KPrintF("Bad callback code %ld from panel\n", code));
	}
	return 0;
}

#ifdef __MORPHOS__
struct EmulLibEntry callback =
{
	TRAP_LIB, 0, (void (*)(void)) callback_function
};
#endif

TEXT panel_path[FILENAME_MAX];

void gui_open()
{
	struct ifnet *ifp;
	DGUI(long PanelVersion;)
	long showflags = 0;
	char ifname[IFNAMSIZ+2];
	long ifstate;
  	char ifspeed[8];
	int  i;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_gui.c) gui_open()\n"));
#endif

	if (GUI_Running) {
		DGUI(KPrintF("GUI is already opened\n");)
		return;
	}
	if (!gui_cnf.PanelName) {
		DGUI(KPrintF("GUI is disabled\n");)
		return;
	}
	gui_timerio = CreateIORequest(logPort, sizeof(struct timerequest));
	if (gui_timerio) {
	  if (!OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)gui_timerio, 0)) {
//	      NameFromLock(GetProgramDir(), panel_path, FILENAME_MAX);
	      AddPart(panel_path,"LIBS:", FILENAME_MAX);
	      AddPart(panel_path,gui_cnf.PanelName, FILENAME_MAX);
	      strcat(panel_path, ".MiamiPanel");
	      DGUI(KPrintF("Opening GUI: %s...\n", panel_path);)
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_gui.c) gui_open: Attempting to use '%s'\n", panel_path));
#endif
	      MiamiPanelBase = OpenLibrary(panel_path, 0);
	      DGUI(KPrintF("Panel library opened, base = 0x%08lx\n", MiamiPanelBase);)
	      if (MiamiPanelBase) {
		DGUI(PanelVersion = MiamiPanelGetVersion();)
		DGUI(KPrintF("Panel API version: %lu\n", PanelVersion);)
		MiamiPanelInhibitRefresh(TRUE);
		DGUI(KPrintF("Panel inhibited\n");)
		for (i=0; i<6; i++)
			if (gui_show[i])
				showflags |= 1<<i;
		/* Screen is always default */
		if (MiamiPanelInit((IPTR)&callback, (IPTR)&callback, showflags, gui_cnf.PanelFont, NULL, panelx, panely, (IPTR)&guimask)) {
			DGUI(KPrintF("Panel initialized, signals = 0x%08lx\n", guimask);)
			GUI_Running = 1;
			for (ifp = ifnet; ifp; ifp = ifp->if_next) {
				if (!(ifp->if_flags & IFF_LOOPBACK)) {
					sprintf(ifname, "%s%d", ifp->if_name, ifp->if_unit);
					DGUI(KPrintF("Adding interface %s, index %lu, baud rate %lu\n", ifname, ifp->if_index, ifp->if_baudrate);)
					if (ifp->if_flags & IFF_UP) {
						ifstate = MIAMIPANELV_AddInterface_State_Online;
						if (ifp->if_baudrate > 1000000)
							sprintf(ifspeed,"%luM", ifp->if_baudrate / 1000000);
						else
							sprintf(ifspeed,"%lu", ifp->if_baudrate);
					} else {
						ifstate = MIAMIPANELV_AddInterface_State_Offline;
						ifspeed[0] = 0;
					}
					MiamiPanelAddInterface(ifp->if_index, ifname, ifstate, ifp->if_data.ifi_aros_ontime.tv_secs, ifspeed);
				}
			}
			DGUI(KPrintF("Opening the panel...\n");)
			MiamiPanelInhibitRefresh(FALSE);
			DGUI(KPrintF("Panel opened\n");)
			gui_timerio->tr_node.io_Command = TR_ADDREQUEST;
			gui_timerio->tr_time.tv_secs = gui_refresh;
			gui_timerio->tr_time.tv_micro = 0;
			SendIO ((struct IORequest *)gui_timerio);
			return;
		}
		CloseLibrary(MiamiPanelBase);
		MiamiPanelBase = NULL;
	      }
	      CloseDevice((struct IORequest *)gui_timerio);
	  }
	  DeleteIORequest((struct IORequest *)gui_timerio);
	  gui_timerio = NULL;
	}
}

void gui_close()
{
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_gui.c) gui_close()\n"));
#endif
	DGUI(KPrintF("Closing GUI...\n");)
	if (gui_timerio) {
	    if (MiamiPanelBase) {
		if (GUI_Running) {
			DGUI(KPrintF("Cleaning up the panel\n");)
			AbortIO((struct IORequest *)gui_timerio);
			WaitIO((struct IORequest *)gui_timerio);
			GUI_Running = 0;
			guimask = 0;
			MiamiPanelCleanup();
		}
		CloseLibrary(MiamiPanelBase);
		MiamiPanelBase = NULL;
		DGUI(KPrintF("Panel library closed\n");)
	    }
	    CloseDevice((struct IORequest *)gui_timerio);
	    DeleteIORequest((struct IORequest *)gui_timerio);
	    gui_timerio = NULL;
	}
}

void gui_snapshot()
{
#if defined(__AROS__)
D(bug("[AROSTCP](amiga_gui.c) gui_snapshot()\n"));
#endif
	/* TODO */
}

void gui_process_refresh()
{
	struct ifnet *ifp;
	struct timeval now;
	unsigned long rate;
	union {
		unsigned long long q;
		struct {
			unsigned long hi;
			unsigned long lo;
		} l;
	} total;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_gui.c) gui_process_refresh()\n"));
#endif
	
	if (GUI_Running) {
		WaitIO ((struct IORequest *)gui_timerio);
		GetSysTime(&now);
		MiamiPanelInterfaceReport(-1, 0, now.tv_secs, 0, 0);
		for (ifp = ifnet; ifp; ifp = ifp->if_next) {
			if ((ifp->if_flags & (IFF_LOOPBACK | IFF_UP)) == IFF_UP) {
				total.q = ifp->if_ibytes + ifp->if_obytes;
				rate = (total.q - ifp->if_data.ifi_aros_lasttotal)/gui_refresh;
				ifp->if_data.ifi_aros_lasttotal = total.q;
				MiamiPanelInterfaceReport(ifp->if_index, rate, now.tv_secs, total.l.hi, total.l.lo);
			}
		}
		gui_timerio->tr_node.io_Command = TR_ADDREQUEST;
		gui_timerio->tr_time.tv_secs = gui_refresh;
		gui_timerio->tr_time.tv_micro = 0;
		SendIO ((struct IORequest *)gui_timerio);
	}
}

void gui_process_msg(struct SysLogPacket *msg)
{
	struct ifnet *ifp;
	long state;
  	char ifspeed[8];

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_gui.c) gui_process_msg()\n"));
#endif
	
	if (GUI_Running) {
		switch (msg->Level & GUICMD_MASK) {
		case GUICMD_SET_INTERFACE_STATE:
			ifp = (struct ifnet *)msg->Time;
			state = (long)msg->Tag;
			DGUI(KPrintF("Setting interface state for %s%u: 0x%08lx\n", ifp->if_name, ifp->if_unit, state);)
			MiamiPanelSetInterfaceState(ifp->if_index, state, ifp->if_data.ifi_aros_ontime.tv_secs);
			DGUI(KPrintF("Interface state set\n");)
			if (state & (MIAMIPANELV_AddInterface_State_Online | MIAMIPANELV_AddInterface_State_Offline)) {
				if (state == MIAMIPANELV_AddInterface_State_Online) {
					if (ifp->if_baudrate > 1000000)
						sprintf(ifspeed,"%luM", ifp->if_baudrate / 1000000);
					else
						sprintf(ifspeed,"%lu", ifp->if_baudrate);
				} else
					ifspeed[0] = 0;
				MiamiPanelSetInterfaceSpeed(ifp->if_index, ifspeed);
			}
			break;
		case GUICMD_CLOSE:
			gui_close();
			break;
		}
	}
	if ((msg->Level & GUICMD_MASK) == GUICMD_SET_INTERFACE_STATE) {
		if (state & (MIAMIPANELV_AddInterface_State_Online | MIAMIPANELV_AddInterface_State_GoingOffline)) {
			if ((state == MIAMIPANELV_AddInterface_State_Online) && (ifp->if_data.ifi_aros_usedhcp))
				run_dhclient(ifp);
		if (state == MIAMIPANELV_AddInterface_State_GoingOffline)
			kill_dhclient(ifp);
		}
	}
}

void gui_set_interface_state(struct ifnet *ifp, long state)
{
	struct SysLogPacket *msg;

#if defined(__AROS__)
D(bug("[AROSTCP](amiga_gui.c) gui_set_interface_state()\n"));
#endif
	
	if (GUI_Running) {
		if (msg = (struct SysLogPacket *)GetLogMsg(&logReplyPort)) {
			msg->Level = LOG_GUIMSG | GUICMD_SET_INTERFACE_STATE;
			msg->Time = (IPTR)ifp;
			msg->Tag = (STRPTR)state;
			PutMsg(logPort, (struct Message *)msg);
		}
	}
}

void error_request(STRPTR Text, ...)
{
	struct EasyStruct es = {
		sizeof(struct EasyStruct),
		0,
		"AROSTCP error",
		NULL,
		"Ok"
	};

	es.es_TextFormat = Text;
	AROS_SLOWSTACKFORMAT_PRE(Text);
	EasyRequestArgs(NULL, &es, NULL, AROS_SLOWSTACKFORMAT_ARG(Text));
	AROS_SLOWSTACKFORMAT_POST(Text);
}

