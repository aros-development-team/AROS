/*
 * Based on examples from "Connect desktop apps using D-BUS -- Helping
 * applications talk to one another", developerWorks 27 Jul 2004,
 */

#define DBUS_API_SUBJECT_TO_CHANGE
#include <proto/dbus.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>

#include "dbus-amiga.h"

struct Library* DBUSBase;

static int run();
static void send_ping(DBusConnection *bus);

int main() {
  int rc = 0;
  
  DBUSBase = OpenLibrary("dbus.library", 1);

  if (DBUSBase != NULL) {
    rc = run();
    CloseLibrary(DBUSBase);
  }
  else {
    rc = 20;
    fprintf (stderr, "Unable to open dbus.library version 1\n");
  }

  return rc;
}

static int run() {
  void*           ac;
  DBusError       error;
  DBusConnection* bus;

  /* Get a connection to the session bus */
  dbus_error_init (&error);
  bus = dbus_bus_get (DBUS_BUS_SESSION, &error);

  if (!bus) {
    fprintf(stderr, "Failed to connect to the D-BUS daemon: %s", error.message);
    dbus_error_free(&error);
    return 10;
  }
 
  /* Set up this connection to work in an Amiga thread */
  if (dbus_a_setup_connection(bus)) {
    while ((SetSignal(0, 0) & SIGBREAKF_CTRL_C) == 0) {
      send_ping(bus);
      Delay(50);
    }
    dbus_a_cleanup_connection(bus);
  }

  dbus_error_free(&error);
  return 0;
}

static void send_ping(DBusConnection *bus)
{
  DBusMessage *message;

  /* Create a new signal "Ping" on the "com.burtonini.dbus.Signal" interface,
   * from the object "/com/burtonini/dbus/ping". */
  message = dbus_message_new_signal ("/com/burtonini/dbus/ping",
                                     "com.burtonini.dbus.Signal", "Ping");
  /* Append the string "Ping!" to the signal */
  dbus_message_append_args (message,
                            DBUS_TYPE_STRING, "Ping!",
                            DBUS_TYPE_INVALID);
  /* Send the signal */
  dbus_connection_send (bus, message, NULL);
  /* Free the signal now we have finished with it */
  dbus_message_unref (message);
  /* Tell the user we send a signal */
  printf("Ping!\n");
}
