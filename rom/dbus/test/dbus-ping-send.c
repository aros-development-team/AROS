#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

static gboolean send_ping (DBusConnection *bus);

int
main (int argc, char **argv)
{
  GMainLoop *loop;
  DBusConnection *bus;
  DBusError error;

  /* Create a new event loop to run in */
  loop = g_main_loop_new (NULL, FALSE);

  /* Get a connection to the session bus */
  dbus_error_init (&error);
  bus = dbus_bus_get (DBUS_BUS_SESSION, &error);
  if (!bus) {
    g_warning ("Failed to connect to the D-BUS daemon: %s", error.message);
    dbus_error_free (&error);
    return 1;
  }

  /* Set up this connection to work in a GLib event loop */
  dbus_connection_setup_with_g_main (bus, NULL);
  /* Every second call send_ping() with the bus as an argument*/
  g_timeout_add (1000, (GSourceFunc)send_ping, bus);

  /* Start the event loop */
  g_main_loop_run (loop);
  return 0;
}

static gboolean
send_ping (DBusConnection *bus)
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
  g_print("Ping!\n");
  /* Return TRUE to tell the event loop we want to be called again */
  return TRUE;
}
