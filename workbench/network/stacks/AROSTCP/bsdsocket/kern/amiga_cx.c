#include "conf.h"
#include "version.h"

#include <exec/libraries.h>
#include <libraries/commodities.h>
#include <proto/commodities.h>
#include <proto/exec.h>
#include <kern/amiga_cx.h>
#include <kern/amiga_gui.h>

extern UBYTE *REXX_PORT_NAME;
extern struct Task *AROSTCP_Task;
extern struct Library *SocketBase;

struct Library *CxBase = NULL;
static CxObj *Broker = NULL;

struct NewBroker nb = {
	NB_VERSION,
	NULL,
	STACK_RELEASE,
	"The opensource TCP/IP stack",
	0,
	COF_SHOW_HIDE,
	0,
	0,
	0
};

ULONG
cx_init(void)
{
  if (CxBase = OpenLibrary("commodities.library", 0L)) {
    nb.nb_Port = CreateMsgPort();
    if (nb.nb_Port) {
      nb.nb_Name = REXX_PORT_NAME;
      return (ULONG)1 << nb.nb_Port->mp_SigBit;
    }
  }
  cx_deinit();
  return (0);
}

BOOL
cx_show(void)
{
  if (!Broker && nb.nb_Port) {
    Broker = CxBroker(&nb, NULL);
    if (Broker) {
      ActivateCxObj(Broker, 1);
     return TRUE;
    }
  }
  return FALSE;
}

BOOL
cx_hide(void)
{
    struct Message *msg;
    if (Broker) {
      /*
       * Remove the broker from the system
       */
      DeleteCxObj(Broker);
      /* Empty the message port */
      while (msg = GetMsg(nb.nb_Port))
	ReplyMsg(msg);
      Broker = NULL;
    }
  return TRUE;
}

void cx_deinit(void)
{
  if (CxBase) {
    if (nb.nb_Port) {
      cx_hide();
      DeleteMsgPort(nb.nb_Port);
      nb.nb_Port = NULL;
    }
    CloseLibrary(CxBase);
    CxBase = NULL;
  }
}

BOOL cx_poll(void)
{
  CxMsg *msg;
  ULONG msgid, msgtype;

  if (msg = (CxMsg *)GetMsg(nb.nb_Port)) {
    msgid = CxMsgID(msg);
    msgtype = CxMsgType(msg);
    ReplyMsg((struct Message *)msg);
    if (SocketBase) {
      if (msgtype == CXM_COMMAND) {
	/* Commodities has sent a command */
	switch(msgid) {
	case CXCMD_ENABLE:
	case CXCMD_DISABLE:
	    gui_snapshot();
	    break;
	case CXCMD_APPEAR:
	    gui_open();
	    break;
	case CXCMD_DISAPPEAR:
	    gui_close();
	    break;
	case CXCMD_KILL:
	    /* user clicked Remove button, let's quit */
	    Signal(AROSTCP_Task, SIGBREAKF_CTRL_C);
	    break;
	}
      }
    }
    return TRUE;
  }
  return FALSE;
}
