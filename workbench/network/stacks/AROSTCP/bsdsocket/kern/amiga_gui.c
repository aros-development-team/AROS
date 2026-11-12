/*
 * Copyright (C) 2005-2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * The control panel (a MiamiPanel library), in the administration module
 * (kern/admin_ops.h). Everything here runs in the NETTRACE task
 * (kern/amiga_log.c) - but the panel's call-back, which the panel library
 * calls, and the helper process it starts to take an interface on or off
 * line.
 *
 * The variables NETTRACE and the configuration read (GUI_Running, guimask,
 * gui_timerio, MiamiPanelBase, InternalProc, gui_cnf, gui_show,
 * gui_refresh) are the core's (kern/admin_stubs.c): they must be there
 * whether the module is loaded or not. So are gui_set_interface_state(),
 * which the interfaces call, and the part of a panel message that is not
 * about the panel (the DHCP client of an interface that goes off line).
 * The core calls the functions below through the module's table only
 * while they have something to do: while the panel is open, or to open it.
 *
 * The module is not stopped while the panel is open or a helper process
 * of it runs (admin_gui_procs): kern/admin_module.c.
 */

#include "conf.h"

#include <devices/timer.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#if !defined(__AROS__)
#include <emul/emulinterface.h>
#endif
#include <exec/libraries.h>
#include <exec/rawfmt.h>
#include <libraries/miamipanel.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/miamipanel.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_log.h>
#include <sys/synch.h>
#include <syslog.h>

#include <net/netcore.h>            /* the core's names, as the module's table has them */
#include <kern/admin_ops.h>

#include <net/netcore_map.h>        /* always the last include */

/* where the panel opens; not part of the configuration */
static long panelx = 50;
static long panely = 50;

/* the panel's helper processes that run this module's code now */
volatile LONG admin_gui_procs = 0;

static STRPTR strings[] = {
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

    DGUI(log(LOG_DEBUG, "gui_async_op(%lu, %lu) called", code, unit);)
    for(ifp = ifnet_list; ifp; ifp = ifp->if_next) {
        if(ifp->if_index == unit) {
            DGUI(log(LOG_DEBUG, "Found ifp = 0x%p", ifp);)
            ifupdown(ifp, (code == MIAMIPANELV_CallBack_Code_UnitOffline));
        }
    }
    /* the process ends in Forbid(): the module is not unloaded under its last instructions */
    Forbid();
    InternalProc--;
    admin_gui_procs--;
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

    switch(code) {
    case MIAMIPANELV_CallBack_Code_Localize:
        nstr = getarg(args, ULONG) - 5000;
        DGUI(KPrintF("Callback: localize(%lu), result: %s", nstr, strings[nstr]);)
        /* TODO: full localization */
        return (long)strings[nstr];
    case MIAMIPANELV_CallBack_Code_UnitOnline:
    case MIAMIPANELV_CallBack_Code_UnitOffline:
        nstr = getarg(args, ULONG);
        Forbid();
        InternalProc++;
        admin_gui_procs++;
        Permit();
        if(!(CreateNewProcTags(NP_Entry, (IPTR)&gui_async_op,
                               NP_Name, (IPTR)"AROSTCP interface control",
#ifdef __MORPHOS__
                               NP_CodeType, CODETYPE_PPC,
                               NP_PPC_Arg1, code,
                               NP_PPC_Arg2, nstr,
#else
                               /* TODO: Implement passing arguments to gui_async_op() */
#endif
                               TAG_DONE))) {
            Forbid();
            InternalProc--;
            admin_gui_procs--;
            Permit();
        }
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
        if(msg = (struct SysLogPacket *)GetLogMsg(&logReplyPort)) {
            msg->Level = LOG_GUIMSG | GUICMD_CLOSE;
            PutMsg(logPort, (struct Message *)msg);
        }
        break;
    case MIAMIPANELV_CallBack_Code_QuitMiami:
        Signal(AROSTCP_Task, SIGBREAKF_CTRL_C);
        break;
    default:
        DGUI(KPrintF("Bad callback code %ld from panel\n", code));
    }
    return 0;
}

#ifdef __MORPHOS__
struct EmulLibEntry callback = {
    TRAP_LIB, 0, (void (*)(void)) callback_function
};
#endif

static TEXT panel_path[FILENAME_MAX];

/* the speed as the panel shows it: "100M"; buf of 12 at least */
static void getbaud(char *buf, unsigned long baud)
{
    char unit = '\0';

    if(baud > 1000000000) {
        baud /= 1000000000;
        unit = 'G';
    } else if(baud > 1000000) {
        baud /= 1000000;
        unit = 'M';
    } else if(baud > 1000) {
        baud /= 1000;
        unit = 'K';
    }
    buf += ultoa(baud, buf);
    *buf++ = unit;
    *buf = '\0';
}

void admin_gui_open(void)
{
    struct ifnet *ifp;
    DGUI(long PanelVersion;)
    long showflags = 0;
    char ifname[IFNAMSIZ + 2];
    long ifstate;
    char ifspeed[16];
    int  i, len;

#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_gui.c) %s()\n", __func__));
#endif

    if(GUI_Running) {
        DGUI(KPrintF("GUI is already opened\n");)
        return;
    }
    if(!gui_config.PanelName) {
        DGUI(KPrintF("GUI is disabled\n");)
        return;
    }
    gui_timerio = CreateIORequest(logPort, sizeof(struct timerequest));
    if(gui_timerio) {
        if(!OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)gui_timerio, 0)) {
//	      NameFromLock(GetProgramDir(), panel_path, FILENAME_MAX);
            AddPart(panel_path, "LIBS:", FILENAME_MAX);
            AddPart(panel_path, gui_config.PanelName, FILENAME_MAX);
            len = strnlen(panel_path, FILENAME_MAX);
            if(len + sizeof(".MiamiPanel") <= FILENAME_MAX)
                memcpy(panel_path + len, ".MiamiPanel", sizeof(".MiamiPanel"));
            DGUI(KPrintF("Opening GUI: %s...\n", panel_path);)
#if defined(__AROS__)
            D(bug("[AROSTCP](amiga_gui.c) %s: Attempting to use '%s'\n", __func__, panel_path));
#endif
            MiamiPanelBase = OpenLibrary(panel_path, 0);
            DGUI(KPrintF("Panel library opened, base = 0x%p\n", MiamiPanelBase);)
            if(MiamiPanelBase) {
                DGUI(PanelVersion = MiamiPanelGetVersion();)
                DGUI(KPrintF("Panel API version: %lu\n", PanelVersion);)
                MiamiPanelInhibitRefresh(TRUE);
                DGUI(KPrintF("Panel inhibited\n");)
                for(i = 0; i < 6; i++)
                    if(gui_show[i])
                        showflags |= 1 << i;
                /* Screen is always default */
                if(MiamiPanelInit((IPTR)&callback, (IPTR)&callback, showflags, gui_config.PanelFont, NULL, panelx, panely,
                                  (IPTR)&guimask)) {
                    DGUI(KPrintF("Panel initialized, signals = 0x%08lx\n", guimask);)
                    GUI_Running = 1;
                    for(ifp = ifnet_list; ifp; ifp = ifp->if_next) {
                        if(!(ifp->if_flags & IFF_LOOPBACK)) {
                            if_fullname(ifp, ifname, sizeof(ifname));
                            DGUI(KPrintF("Adding interface %s, index %lu, baud rate %lu\n", ifname, ifp->if_index, ifp->if_baudrate);)
                            if(ifp->if_flags & IFF_UP) {
                                ifstate = MIAMIPANELV_AddInterface_State_Online;
                                getbaud(ifspeed, ifp->if_baudrate);
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
                    SendIO((struct IORequest *)gui_timerio);
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

void admin_gui_close(void)
{
#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_gui.c) %s()\n", __func__));
#endif
    DGUI(KPrintF("Closing GUI...\n");)
    if(gui_timerio) {
        if(MiamiPanelBase) {
            if(GUI_Running) {
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

void admin_gui_process_refresh(void)
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
    D(bug("[AROSTCP](amiga_gui.c) %s()\n", __func__));
#endif

    if(GUI_Running) {
        WaitIO((struct IORequest *)gui_timerio);
        GetSysTime(&now);
        MiamiPanelInterfaceReport(-1, 0, now.tv_secs, 0, 0);
        for(ifp = ifnet_list; ifp; ifp = ifp->if_next) {
            if((ifp->if_flags & (IFF_LOOPBACK | IFF_UP)) == IFF_UP) {
                total.q = ifp->if_ibytes + ifp->if_obytes;
                /* bytes per second over the refresh interval: the difference
                 * in 32 bits (a 64-bit division would bring 1.7 KB of
                 * libgcc into the library on m68k); it saturates instead of
                 * wrapping above 4 GB per interval */
                {
                    UQUAD d = total.q - ifp->if_data.ifi_aros_lasttotal;

                    rate = (d > 0xffffffffULL ? 0xffffffffUL : (unsigned long)d) /
                           (gui_refresh ? (unsigned long)gui_refresh : 1);
                }
                ifp->if_data.ifi_aros_lasttotal = total.q;
                MiamiPanelInterfaceReport(ifp->if_index, rate, now.tv_secs, total.l.hi, total.l.lo);
            }
        }
        gui_timerio->tr_node.io_Command = TR_ADDREQUEST;
        gui_timerio->tr_time.tv_secs = gui_refresh;
        gui_timerio->tr_time.tv_micro = 0;
        SendIO((struct IORequest *)gui_timerio);
    }
}

/* the panel's part of a panel message; the core's gui_process_msg() does the rest */
void admin_gui_process_msg(struct SysLogPacket *msg)
{
    struct ifnet *ifp = NULL;
    long state = 0;
    char ifspeed[16];

#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_gui.c) %s()\n", __func__));
#endif

    if(GUI_Running) {
        switch(msg->Level & GUICMD_MASK) {
        case GUICMD_SET_INTERFACE_STATE:
            ifp = (struct ifnet *)msg->Time;
            state = (long)msg->Tag;
            DGUI(KPrintF("Setting interface state for %s%u: 0x%08lx\n", ifp->if_name, ifp->if_unit, state);)
            MiamiPanelSetInterfaceState(ifp->if_index, state, ifp->if_data.ifi_aros_ontime.tv_secs);
            DGUI(KPrintF("Interface state set\n");)
            if(state & (MIAMIPANELV_AddInterface_State_Online | MIAMIPANELV_AddInterface_State_Offline)) {
                if(state == MIAMIPANELV_AddInterface_State_Online) {
                    getbaud(ifspeed, ifp->if_baudrate);
                } else
                    ifspeed[0] = 0;
                MiamiPanelSetInterfaceSpeed(ifp->if_index, ifspeed);
            }
            break;
        case GUICMD_CLOSE:
            admin_gui_close();
            break;
        }
    }
}
