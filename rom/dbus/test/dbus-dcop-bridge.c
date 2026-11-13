
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib.h>
#include <dbus/dbus-glib.h> 
#include <dbus/dbus-glib-lowlevel.h>

#define RETURN_OK     0
#define RETURN_ERROR 10
#define RETURN_FAIL  20

static const char* exec_cmd(char* const argv[]) {
  int s;
  int fd[2];
  char* res = NULL;

  if (pipe(fd) < 0) {
    perror("pipe");
    return NULL;
  }

  s = fork();

  if (s < 0) {
    perror("fork");
    close(fd[0]);
    close(fd[1]);
    return NULL;
  }
  else if (s == 0) {
    if (dup2(fd[1], STDOUT_FILENO) < 0 ||
	dup2(fd[1], STDERR_FILENO) < 0) {
      perror("dup2");
      exit(RETURN_ERROR);
    }

    close(fd[0]);
    close(fd[1]);

    if (execvp(argv[0], argv) < 0) {
      perror("execvp");
      exit(RETURN_FAIL);
    }
  }
  else {
    close(fd[1]);

    res = malloc(65536);

    if (res != NULL) {
      int rd = read(fd[0], res, 65535);

      if (rd >= 0) {
	res[rd] = 0;
      }
    }
    else {
      perror("malloc");
    }
    
    close(fd[0]);
  }

  return res;
}


static DBusHandlerResult filter_disconnect(DBusConnection* connection,
					   DBusMessage*    message,
					   void*           user_data) {
  GMainLoop* loop = (GMainLoop*) user_data;
  
  if (!dbus_message_is_signal(message, DBUS_INTERFACE_ORG_FREEDESKTOP_LOCAL,
			      "Disconnected")) {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  dbus_connection_disconnect(connection);
  g_main_loop_quit(loop);
  
  return DBUS_HANDLER_RESULT_HANDLED;
}

static void path_unregistered(DBusConnection* connection,
			      void*           user_data) {
  printf("path_unregistered\n");
  // Connection was finalized 
}

static DBusHandlerResult path_message(DBusConnection* connection,
				      DBusMessage*    message,
				      void*           user_data) {
  if (dbus_message_is_method_call(message,
				  "org.aros.dbus.DCOP",
				  "Exec")) {
    DBusMessageIter iter;
    DBusMessage* reply;
    const char* res = NULL;
    int argc = 1;
    char** argv;

    dbus_message_iter_init(message, &iter);

    while (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING) {
      ++argc;

      if (!dbus_message_iter_next(&iter)) {
	break;
      }
    }

    argv = malloc(sizeof (char*) * (argc + 1));

    if (argv != NULL) {
      int i;
      argv[0] = "dcop";

      dbus_message_iter_init(message, &iter);
      
      for (i = 1; i < argc; ++i) {
	argv[i] = dbus_message_iter_get_string(&iter);
	dbus_message_iter_next(&iter);
      }

      argv[i] = NULL;

      res = exec_cmd(argv);
    }
    else {
      fprintf(stderr, "Unable to create dcop argument vector\n");
    }
    

    reply = dbus_message_new_method_return(message);

    if (reply != NULL) {
      if (dbus_message_append_args(reply,
				   DBUS_TYPE_STRING, res,
				   DBUS_TYPE_INVALID)) {
	if (dbus_connection_send(connection, reply, NULL)) {
	  // Alles gut
	}
	else {
	  fprintf(stderr, "Unable to send reply\n");
	}
      }
      else {
	fprintf(stderr, "Unable to build reply\n");
      }
      
      dbus_message_unref (reply);
    }
    else {
      fprintf(stderr, "Unable to create reply\n");
    }

    free(res);

    return DBUS_HANDLER_RESULT_HANDLED;
  }
  else {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
}


static DBusObjectPathVTable dcop_vtable = {
  path_unregistered,
  path_message,
  NULL,
  0, 0, 0
};

int main(int argc, char **argv)
{
  GMainLoop *loop;
  DBusConnection *connection;
  DBusError error;
  int rc = RETURN_OK;

  if (argc != 1) {
    fprintf(stderr, "%s: No arguments allowed.\n", argv[0]);
    return RETURN_FAIL;
  }
  
  dbus_error_init(&error);
  
  connection = dbus_bus_get(DBUS_BUS_ACTIVATION, &error);

  if (connection != NULL) {
    loop = g_main_loop_new(NULL, FALSE);

    if (loop != NULL) {
      dbus_connection_setup_with_g_main(connection, NULL);
      
      if (dbus_connection_add_filter(connection, filter_disconnect, loop, NULL)) {
	if (dbus_connection_register_object_path(connection,
						 "/org/aros/dbus/dbus_dcop",
						 &dcop_vtable,
						 NULL)) {
	  int result = dbus_bus_acquire_service(connection, "org.aros.dbus.DCOP",
						0, &error);
	  
	  if (!dbus_error_is_set(&error)) {
	    g_main_loop_run(loop);
	  }
	  else {
	    fprintf(stderr, "ailed to acquire service: %s\n", error.message);
	    rc = RETURN_FAIL;
	  }
	}
	else {
	  fprintf(stderr, "Failed to register object path\n");
	  rc = RETURN_FAIL;
	}

	dbus_connection_remove_filter(connection, filter_disconnect, NULL);
      }
      else {
	fprintf(stderr, "Failed to create connection filter\n");
	rc = RETURN_FAIL;
      }

    }
    else {
      fprintf(stderr, "Failed to create mail loop\n");
      rc = RETURN_FAIL;
    }

    dbus_connection_unref(connection);
  }
  else {
    fprintf(stderr, "Failed to open connection to activating message bus: %s\n", error.message);
    rc = RETURN_FAIL;
  }
  
  dbus_error_free(&error);
  dbus_shutdown();
  return rc;
}
