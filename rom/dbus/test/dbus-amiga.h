#ifndef DBUS_AMIGA_H
#define DBUS_AMIGA_H

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

dbus_bool_t dbus_a_setup_connection(DBusConnection* connection);
void dbus_a_cleanup_connection(DBusConnection* connection);

#endif /* DBUS_AMIGA_H */
