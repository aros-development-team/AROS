/*
 * Based on examples from "Connect desktop apps using D-BUS -- Helping
 * applications talk to one another", developerWorks 27 Jul 2004,
 */


#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <stdio.h>

#ifdef __AMIGA__
#include <proto/dbus.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "dbus-amiga.h"

struct Library* DBUSBase;
#else
# include <dbus/dbus-glib.h> 
# include <dbus/dbus-glib-lowlevel.h>
# define RETURN_OK     0
# define RETURN_WARN   5
# define RETURN_ERROR 10
# define RETURN_FAIL  20
#endif

static int send_command(DBusConnection *bus, int num_args, const char** args);

int main(int argc, const char** argv) {
  int             rc = RETURN_OK;
  DBusError       error;
  DBusConnection* bus;
    
#ifdef __AMIGA__
  DBUSBase = OpenLibrary("dbus.library", 1);

  if (DBUSBase == NULL) {
    fprintf (stderr, "Unable to open dbus.library version 1\n");
    return RETURN_FAIL;
  }
#endif
  
  /* Get a connection to the session bus */
  dbus_error_init (&error);
  bus = dbus_bus_get (DBUS_BUS_SESSION, &error);

  if (bus) {
#ifdef __AMIGA__
    /* Set up this connection to work as an Amiga thread */
    if (dbus_a_setup_connection(bus)) {
      rc = send_command(bus, argc - 1, argv + 1);
      dbus_a_cleanup_connection(bus);
    }
    else {
      fprintf(stderr, "Failed to set up the Amiga connection\n");
      rc = RETURN_FAIL;
    }
#else
    /* Set up this connection to work in a GLib event loop */
    dbus_connection_setup_with_g_main (bus, NULL);
    rc = send_command(bus, argc - 1, argv + 1);
#endif
  }
  else {
    fprintf(stderr, "Failed to connect to the D-BUS daemon: %s\n", error.message);
    rc = RETURN_FAIL;
  }
   
  dbus_error_free(&error);

#ifdef __AMIGA__
  CloseLibrary(DBUSBase);
#endif

  return rc;
}

static int send_command(DBusConnection *bus, int num_args, const char** args)
{
  int rc = RETURN_OK;
  int i;
  DBusMessage* message = NULL;
  DBusMessage* reply = NULL;
  DBusError error;

  dbus_error_init (&error);
  
  /* Create a new message "Exec" on the "org.aros.dbus.DCOP" interface,
   * from the object "/org/aros/dbus/dbus_dcop". */
  message = dbus_message_new_method_call ("org.aros.dbus.DCOP",
					  "/org/aros/dbus/dbus_dcop",
					  "org.aros.dbus.DCOP",
					  "Exec");

  if (message != NULL) {
    /* Append the argumens as strings to the message */
    for (i = 0; i < num_args; ++i) {
      dbus_message_append_args (message,
				DBUS_TYPE_STRING, args[i],
				DBUS_TYPE_INVALID);
    }
  
    /* Send the message */
    reply = dbus_connection_send_with_reply_and_block (bus, message, -1, &error);

    /* Free the message now we have finished with it */
    dbus_message_unref (message);

    if (reply != NULL) {
      char* res_str = NULL;
      
      if (dbus_message_get_args(reply, &error,
				DBUS_TYPE_STRING, &res_str,
				DBUS_TYPE_INVALID)) {
	if (res_str != NULL) {
	  /* Print out the result */
	  printf("%s", res_str);

	  dbus_free(res_str);
	  rc = RETURN_OK;
	}
	else {
	  rc = RETURN_WARN;
	}
      }
    
      /* Free the reply now we have finished with it */
      dbus_message_unref (reply);
    }
    else {
      fprintf(stderr, "Failed to get a reply from the D-BUS/DCOP bridge: %s\n", error.message);
      rc = RETURN_ERROR;
    }
  }
  else {
    fprintf(stderr, "Failed to send message to the D-BUS/DCOP bridge: %s\n", error.message);
    rc = RETURN_ERROR;
    
  }
  
  dbus_error_free(&error);
  return rc;
}
