/*
 * AmigaOS D-BUS connection handling using a slave process as the main
 * loop.
 */

#include <dos/dostags.h>
#include <exec/tasks.h>

#include <clib/alib_protos.h>
#define DBUS_API_SUBJECT_TO_CHANGE
#include <proto/dbus.h>
#include <proto/exec.h>
#include <proto/dos.h>

#define DEBUG 1
#include <aros/debug.h>

#include "dbus-amiga.h"

struct WatchData {
    struct MinNode node;
    BOOL           enabled;
    DBusWatch*     watch;
};

struct ConnectionData {
    struct Task*    main;
    struct Task*    creator;
    BYTE            signal;
    struct MinList  watches;
    DBusConnection* connection;
};

static dbus_int32_t Slot = -1;

static void DeleteConnectionData(struct ConnectionData* c);
static void CleanupAmigaConnection(void* data);

static void MainLoop() {
  struct ConnectionData* c = (struct ConnectionData*) FindTask(NULL)->tc_UserData;
  kprintf("MainLoop started %08lx (cd %08lx)\n", FindTask(NULL), c);

  c->signal = AllocSignal(-1);

  if (c->signal == -1) {
    goto exit;
  }
  
  Signal(c->creator, SIGF_SINGLE);
  
  while(TRUE) {
    ULONG signals = Wait(SIGBREAKF_CTRL_C | (1UL << c->signal));

    kprintf("MainLoop got a signal %lx\n", signals);

    if (signals & SIGBREAKF_CTRL_C) {
      break;
    }

    if (signals & (1UL << c->signal)) {
      struct WatchData* w;
      
//      dbus_connection_ref(c->connection);

      kprintf("Checking watches\n");
      for(w = (struct WatchData*) c->watches.mlh_Head;
	  w->node.mln_Succ != NULL;
	  w = (struct WatchData*) w->node.mln_Succ) {
	kprintf("%s watch on fd %ld, flags %lx\n",
		w->enabled ? "Enabled" : "Disabled",
		dbus_watch_get_fd(w->watch), dbus_watch_get_flags(w->watch));
	if (w->enabled) {
	  dbus_watch_handle(w->watch, dbus_watch_get_flags(w->watch));
	}
      }
      
      kprintf("Dispatching messages\n");
      /* Dispatch messages */
      while (dbus_connection_dispatch(c->connection) == DBUS_DISPATCH_DATA_REMAINS) {
	kprintf("More messages available\n");
      }

//      dbus_connection_unref(c->connection);
    }
  }

exit:
  c->main = NULL;
  Signal(c->creator, SIGF_SINGLE);
  kprintf("MainLoop terminating\n");
}

static void* CreateWatchData(DBusWatch* watch) {
  struct WatchData* w = AllocVec(sizeof(struct WatchData), MEMF_ANY|MEMF_CLEAR);

  if (w != NULL) {
    w->enabled = dbus_watch_get_enabled(watch);
    w->watch   = watch;
  }

  return w;
}


static void DeleteWatchData(void* memory) {
  struct WatchData* w = (struct WatchData*) memory;

  if (w != NULL) {
    if (w->node.mln_Succ != NULL) {
      Remove((struct Node*) w);
    }

    FreeVec(w);
  }
}

static void* CreateConnectionData(DBusConnection* connection) {
  struct ConnectionData* c = AllocVec(sizeof(struct ConnectionData), MEMF_ANY|MEMF_CLEAR);

  kprintf("CreateConnectionData %08lx\n", c);
  
  if (c != NULL) {
    c->connection = connection;
    c->creator = FindTask(NULL);
    NewList((struct List*) &c->watches);
    
    Forbid();
    kprintf("creating mainloop\n");
    c->main   = (struct Task*) CreateNewProcTags(NP_Entry,    (ULONG) MainLoop,
						 NP_Name,     (ULONG) "dbus.library main loop",
						 NP_Priority, 0,
						 TAG_DONE);
    kprintf("created mainloop %08lx\n", c->main);
    if (c->main != NULL) {
      c->main->tc_UserData = c;
    }

    SetSignal(0, SIGF_SINGLE);
    Permit();

    Wait(SIGF_SINGLE);

    if (c->main == NULL) {
      DeleteConnectionData(c);
      c = NULL;
    }
  }

  return c;
}


static void DeleteConnectionData(struct ConnectionData* c) {
  kprintf("DeleteConnectionData %08lx\n", c);
  
  if (c != NULL) {
    if (c->main != NULL) {
      SetSignal(0, SIGF_SINGLE);
      Signal(c->main, SIGBREAKF_CTRL_C);
      Wait(SIGF_SINGLE);
    }
    
    FreeVec(c);
  }
}

static dbus_bool_t AddWatchFunction(DBusWatch* watch,
				    void*      data) {
  struct ConnectionData* c = (struct ConnectionData*) data;
  struct WatchData*      w;

  kprintf("AddWatchFunction\n");

  w = CreateWatchData(watch);

  if (w == NULL) {
    return FALSE;
  }

  AddTail((struct List*) &c->watches, (struct Node*) w);
  dbus_watch_set_data(watch, w, NULL);

  return TRUE;
}

static void WatchToggledFunction(DBusWatch* watch,
				 void*      data) {
  struct ConnectionData* c = (struct ConnectionData*) data;
  struct WatchData*      w = (struct WatchData*) dbus_watch_get_data(watch);

  kprintf("WatchToggledFunction %lx\n", w);

  w->enabled = dbus_watch_get_enabled(watch);
}

static void RemoveWatchFunction(DBusWatch* watch,
				void*      data) {
  struct ConnectionData* c = (struct ConnectionData*) data;
  struct WatchData*      w = (struct WatchData*) dbus_watch_get_data(watch);

  kprintf("RemoveWatchFunction %lx\n", w);

  dbus_watch_set_data(watch, NULL, NULL);
  DeleteWatchData(w);
}

static dbus_bool_t AddTimeoutFunction(DBusTimeout* timeout,
				      void*        data) {
  struct ConnectionData* c = (struct ConnectionData*) data;

  kprintf("AddTimeoutFunction\n");
  return TRUE;
}

static void TimeoutToggledFunction(DBusTimeout* timeout,
				   void* data) {
  struct ConnectionData* c = (struct ConnectionData*) data;

  kprintf("TimeoutToggledFunction\n");
}

static void RemoveTimeoutFunction(DBusTimeout* timeout,
				  void*        data) {
  struct ConnectionData* c = (struct ConnectionData*) data;

  kprintf("RemoveTimeoutFunction\n");
}


static void DispatchStatusFunction(DBusConnection*    connection,
				   DBusDispatchStatus new_status,
				   void*              data) {
  struct ConnectionData* c = (struct ConnectionData*) data;

  kprintf("DispatchStatusFunction %d\n", new_status);
  
  if (new_status == DBUS_DISPATCH_DATA_REMAINS) {
    Signal(c->main, 1UL << c->signal);
  }
}

static void WakeupMainFunction(void* data) {
  struct ConnectionData* c = (struct ConnectionData*) data;

  kprintf("WakeupMainFunction\n");
  Signal(c->main, 1UL << c->signal);
}

dbus_bool_t dbus_a_setup_connection(DBusConnection* connection) {
  struct ConnectionData* c;

  c = CreateConnectionData(connection);

  if (c != NULL) {
    dbus_connection_set_watch_functions(connection,
					AddWatchFunction,
					RemoveWatchFunction,
					WatchToggledFunction,
					c, NULL);
  
    dbus_connection_set_timeout_functions(connection,
					  AddTimeoutFunction,
					  RemoveTimeoutFunction,
					  TimeoutToggledFunction,
					  c, NULL);

    dbus_connection_set_dispatch_status_function(connection, DispatchStatusFunction,
						 c, NULL);

    dbus_connection_set_wakeup_main_function(connection, WakeupMainFunction,
					     c, NULL);

    kprintf("Slot A: %ld\n", Slot);
    if (dbus_connection_allocate_data_slot(&Slot)) {
      kprintf("Slot B: %ld\n", Slot);
      if (dbus_connection_set_data(connection, Slot, c, DeleteConnectionData)) {
	kprintf("Slot C: %ld\n", Slot);
	return TRUE;
      }
    }
  }

  DeleteConnectionData(c);
  return FALSE;
}

void dbus_a_cleanup_connection(DBusConnection* connection) {
  struct ConnectionData* c;

  kprintf("CleanupAmigaConnection\n");

  kprintf("Slot X: %ld\n", Slot);
  c = dbus_connection_get_data(connection, Slot);

  if (c != NULL) {
    dbus_connection_set_data(connection, Slot, NULL, NULL);
    dbus_connection_free_data_slot(&Slot);
    kprintf("Slot Y: %ld\n", Slot);
  }

  dbus_connection_set_dispatch_status_function(connection, NULL, NULL, NULL);
  dbus_connection_set_wakeup_main_function(connection, NULL, NULL, NULL);
  dbus_connection_set_watch_functions(connection, NULL, NULL, NULL, NULL, NULL);
  dbus_connection_set_timeout_functions(connection, NULL, NULL, NULL, NULL, NULL);

  kprintf("Slot X: %ld\n", Slot);
}
