#ifndef DBUS_INTERN_H
#define DBUS_INTERN_H

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

#include <exec/libraries.h>
#include <exec/execbase.h>
#include <dos/dos.h>

struct DBUSBase {
    struct Library   libnode;
    BPTR             seglist;
};

void InitThreads(struct DBUSBase* DBUSBase);

#endif /* DBUS_INTERN_H */
