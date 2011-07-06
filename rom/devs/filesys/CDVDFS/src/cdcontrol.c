/* cdcontrol.c:
 *
 * Control program for the CDVDFS handler.
 *
 * ----------------------------------------------------------------------
 * This code is (C) Copyright 1993,1994 by Frank Munkert.
 *              (C) Copyright 2007 by Pavel Fedin.
 * All rights reserved.
 * This software may be freely distributed and redistributed for
 * non-commercial purposes, provided this notice is included.
 */

#include <stdlib.h>
#include <stdio.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <dos/dosextens.h>
#include <exec/memory.h>

#ifdef LATTICE
extern struct Library *DOSBase;
#endif

#include "cdcontrol.h"

#ifdef __AROS__
struct UtilityBase *UtilityBase = NULL;
#else
struct Library *UtilityBase = NULL;
#endif

struct MsgPort *g_device_proc;

enum partype {
  PAR_NONE,
  PAR_ANY,
  PAR_ON_OFF
};

void Cleanup (void)
{
  if (UtilityBase)
    CloseLibrary ((APTR)UtilityBase);
}

void Send_Packet (int p_cmd, void *p_1, void *p_2)
{
  struct MsgPort *replyport = CreateMsgPort();
  struct StandardPacket *packet = AllocMem (sizeof (struct StandardPacket),
  					    MEMF_CLEAR | MEMF_PUBLIC);
  long res1, res2;

  if (!packet || !replyport) {
    fprintf (stderr, "ERROR: cannot send packet\n");
    if (packet)
        FreeMem (packet, sizeof (struct StandardPacket));
    if (replyport)
        DeleteMsgPort(replyport);
    exit (1);
  }
  
  packet->sp_Msg.mn_Node.ln_Name = (char *) &(packet->sp_Pkt);
  packet->sp_Pkt.dp_Link = &(packet->sp_Msg);

  packet->sp_Pkt.dp_Port = replyport;
  packet->sp_Pkt.dp_Type = ACTION_USER;
  packet->sp_Pkt.dp_Arg1 = p_cmd;
  packet->sp_Pkt.dp_Arg2 = (ULONG) p_1;
  packet->sp_Pkt.dp_Arg3 = (ULONG) p_2;

  PutMsg (g_device_proc, (struct Message *) packet);
  WaitPort (replyport);
  GetMsg (replyport);

  res1 = packet->sp_Pkt.dp_Res1;
  res2 = packet->sp_Pkt.dp_Res2;
  if (res1 == 0)
    PrintFault (res2, (UBYTE *) "ERROR");

  FreeMem (packet, sizeof (struct StandardPacket));
  DeleteMsgPort (replyport);
}

void Cmd_Lowercase (void *p_1, void *p_2)
{
  Send_Packet (CDCMD_LOWERCASE, p_1, p_2);
}

void Cmd_Mactoiso (void *p_1, void *p_2)
{
  Send_Packet (CDCMD_MACTOISO, p_1, p_2);
}

void Cmd_Convertspaces (void *p_1, void *p_2)
{
  Send_Packet (CDCMD_CONVERTSPACES, p_1, p_2);
}

void Cmd_Showversion (void *p_1, void *p_2)
{
  Send_Packet (CDCMD_SHOWVERSION, p_1, p_2);
}

void Cmd_Hfsfirst (void *p_1, void *p_2)
{
  Send_Packet (CDCMD_HFSFIRST, p_1, p_2);
}

void Cmd_Dataext (void *p_1, void *p_2)
{
  Send_Packet (CDCMD_DATAEXT, p_1, p_2);
}

void Cmd_Resourceext (void *p_1, void *p_2)
{
  Send_Packet (CDCMD_RESOURCEEXT, p_1, p_2);
}

void Cmd_Unicodetable (void *p_1, void *p_2)
{
  if (!Stricmp(p_1, "none"))
    p_1 = NULL;
  Send_Packet (CDCMD_UNICODETABLE, p_1, p_2);
}

struct par {
  char *name;
  char *abbrev;
  enum partype par1;
  enum partype par2;
  void (*func)(void *, void *);
} g_par[] = {
  {"lowercase",	   "l",  PAR_ON_OFF,	PAR_NONE,	Cmd_Lowercase},
  {"mactoiso",	   "mi", PAR_ON_OFF,	PAR_NONE,	Cmd_Mactoiso},
  {"convertspaces", "cs", PAR_ON_OFF,	PAR_NONE,	Cmd_Convertspaces},
  {"showversion",   "sv", PAR_ON_OFF,	PAR_NONE,	Cmd_Showversion},
  {"hfsfirst",	   "hf", PAR_ON_OFF,	PAR_NONE,	Cmd_Hfsfirst},
  {"dataext",	   "de", PAR_ANY,	PAR_NONE,	Cmd_Dataext},
  {"resourceext",   "re", PAR_ANY,	PAR_NONE,	Cmd_Resourceext},
  {"unicodetable",  "ut", PAR_ANY,	PAR_NONE,	Cmd_Unicodetable}
};

void Usage (void)
{
  printf (
    "Usage: cdcontrol device command\n"
    "  Commands & parameters    Abbrev.   Meaning\n"
    "  ---------------------    -------   -------\n"
    "  lowercase [on/off]       l         Convert ISO filenames to lowercase\n"
    "  mactoiso [on/off]        mi        Convert Mac to Amiga characters\n"
    "  convertspaces [on/off]   cs        Convert HFS spaces into underscores\n"
    "  showversion [on/off]     sv        Show version number of ISO filenames\n"
    "  hfsfirst [on/off]        hf        Look for a HFS partition first\n"
    "  dataext <extension>      de        Set extension for HFS data forks\n"
    "  resourceext <extension>  re        Set extension for HFS resource forks\n"
    "  unicodetable <name>      ut        Load Joliet character conversion table\n"
    "                                     (\"none\" means default built-in Latin-1 table)\n"
    "Examples:\n"
    "  cdcontrol cd0: showversion on\n"
    "  cdcontrol cd0: sv on\n"
    "  cdcontrol cd0: ut L:FileSystem_Trans/windows-1251.txt\n"
    "  cdcontrol cd0: ut none\n"
    );

  exit (1);
}

int main (int argc, char *argv[])
{
  int i;
  struct DevProc *dvp;

  atexit (Cleanup);

  if (argc != 4)
    Usage ();

  if (!(UtilityBase = (APTR)OpenLibrary ("utility.library", 37))) {
    fprintf (stderr, "cannot open utility.library\n");
    exit (1);
  }  

  dvp = GetDeviceProc(argv[1], NULL);
  if (!dvp) {
    fprintf (stderr, "ERROR: cannot find device \"%s\"\n", argv[1]);
    exit (1);
  }
#ifdef __AROS__
  g_device_proc = dvp->dvp_DevNode->dol_Task;
#else
  g_device_proc = dvp->dvp_Port;
#endif
  FreeDeviceProc(dvp);
#ifdef __AROS__
  if (!g_device_proc) {
    fprintf(stderr, "ERROR: \"%s\" is not a CDVDFS device\n", argv[1]);
    exit(1);
  }
#endif
  for (i=0; i < (sizeof (g_par) / sizeof (struct par)); i++) {
    if (Stricmp ((UBYTE *) argv[2], (UBYTE *) g_par[i].name) == 0 ||
        Stricmp ((UBYTE *) argv[2], (UBYTE *) g_par[i].abbrev) == 0) {
      void *a = argv[3];
      void *b = NULL;

      if (g_par[i].par1 == PAR_ON_OFF) {
        if (Stricmp ((UBYTE *) argv[3], (UBYTE *) "on") == 0)
          a = (void *) 1;
        else if (Stricmp ((UBYTE *) argv[3], (UBYTE *) "off") == 0)
          a = (void *) 0;
	else {
	  fprintf (stderr, "ERROR: \"on\" or \"off\" expected\n");
	  exit (1);
        }
      }
      g_par[i].func (a, b);
      exit (0);
    }
  }
  Usage ();
  return 0;
}

