/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus-connection.h DBusConnection object
 *
 * Copyright (C) 2002, 2003  Red Hat Inc.
 *
 * Licensed under the Academic Free License version 2.1
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#if !defined (DBUS_INSIDE_DBUS_H) && !defined (DBUS_COMPILATION)
#error "Only <dbus/dbus.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef DBUS_CONNECTION_H
#define DBUS_CONNECTION_H

#include <dbus/dbus-errors.h>
#include <dbus/dbus-memory.h>
#include <dbus/dbus-message.h>
#include <dbus/dbus-shared.h>

DBUS_BEGIN_DECLS;

typedef struct DBusWatch DBusWatch;
typedef struct DBusTimeout DBusTimeout;
typedef struct DBusPreallocatedSend DBusPreallocatedSend;
typedef struct DBusPendingCall DBusPendingCall;
typedef struct DBusConnection DBusConnection;
typedef struct DBusObjectPathVTable DBusObjectPathVTable;

typedef enum
{
  DBUS_WATCH_READABLE = 1 << 0, /**< As in POLLIN */
  DBUS_WATCH_WRITABLE = 1 << 1, /**< As in POLLOUT */
  DBUS_WATCH_ERROR    = 1 << 2, /**< As in POLLERR (can't watch for this, but
                                 *   the flag can be passed to dbus_watch_handle()).
                                 */
  DBUS_WATCH_HANGUP   = 1 << 3  /**< As in POLLHUP (can't watch for it, but
                                 *   can be present in current state). */
} DBusWatchFlags;

typedef enum
{
  DBUS_DISPATCH_DATA_REMAINS,  /**< There is more data to potentially convert to messages. */
  DBUS_DISPATCH_COMPLETE,      /**< All currently available data has been processed. */
  DBUS_DISPATCH_NEED_MEMORY    /**< More memory is needed to continue. */
} DBusDispatchStatus;

typedef dbus_bool_t (* DBusAddWatchFunction)       (DBusWatch      *watch,
                                                    void           *data);
typedef void        (* DBusWatchToggledFunction)   (DBusWatch      *watch,
                                                    void           *data);
typedef void        (* DBusRemoveWatchFunction)    (DBusWatch      *watch,
                                                    void           *data);
typedef dbus_bool_t (* DBusAddTimeoutFunction)     (DBusTimeout    *timeout,
                                                    void           *data);
typedef void        (* DBusTimeoutToggledFunction) (DBusTimeout    *timeout,
                                                    void           *data);
typedef void        (* DBusRemoveTimeoutFunction)  (DBusTimeout    *timeout,
                                                    void           *data);
typedef void        (* DBusDispatchStatusFunction) (DBusConnection *connection,
                                                    DBusDispatchStatus new_status,
                                                    void           *data);
typedef void        (* DBusWakeupMainFunction)     (void           *data);
typedef dbus_bool_t (* DBusAllowUnixUserFunction)  (DBusConnection *connection,
                                                    unsigned long   uid,
                                                    void           *data);

typedef void (* DBusPendingCallNotifyFunction) (DBusPendingCall *pending,
                                                void            *user_data);


typedef DBusHandlerResult (* DBusHandleMessageFunction) (DBusConnection     *connection,
                                                         DBusMessage        *message,
                                                         void               *user_data);

DBusConnection*    dbus_connection_open                         (const char                 *address,
                                                                 DBusError                  *error);
DBusConnection*    dbus_connection_ref                          (DBusConnection             *connection);
void               dbus_connection_unref                        (DBusConnection             *connection);
void               dbus_connection_disconnect                   (DBusConnection             *connection);
dbus_bool_t        dbus_connection_get_is_connected             (DBusConnection             *connection);
dbus_bool_t        dbus_connection_get_is_authenticated         (DBusConnection             *connection);
void               dbus_connection_set_exit_on_disconnect       (DBusConnection             *connection,
                                                                 dbus_bool_t                 exit_on_disconnect);
void               dbus_connection_flush                        (DBusConnection             *connection);
DBusMessage*       dbus_connection_borrow_message               (DBusConnection             *connection);
void               dbus_connection_return_message               (DBusConnection             *connection,
                                                                 DBusMessage                *message);
void               dbus_connection_steal_borrowed_message       (DBusConnection             *connection,
                                                                 DBusMessage                *message);
DBusMessage*       dbus_connection_pop_message                  (DBusConnection             *connection);
DBusDispatchStatus dbus_connection_get_dispatch_status          (DBusConnection             *connection);
DBusDispatchStatus dbus_connection_dispatch                     (DBusConnection             *connection);
dbus_bool_t        dbus_connection_send                         (DBusConnection             *connection,
                                                                 DBusMessage                *message,
                                                                 dbus_uint32_t              *client_serial);
dbus_bool_t        dbus_connection_send_with_reply              (DBusConnection             *connection,
                                                                 DBusMessage                *message,
                                                                 DBusPendingCall           **pending_return,
                                                                 int                         timeout_milliseconds);
DBusMessage *      dbus_connection_send_with_reply_and_block    (DBusConnection             *connection,
                                                                 DBusMessage                *message,
                                                                 int                         timeout_milliseconds,
                                                                 DBusError                  *error);
dbus_bool_t        dbus_connection_set_watch_functions          (DBusConnection             *connection,
                                                                 DBusAddWatchFunction        add_function,
                                                                 DBusRemoveWatchFunction     remove_function,
                                                                 DBusWatchToggledFunction    toggled_function,
                                                                 void                       *data,
                                                                 DBusFreeFunction            free_data_function);
dbus_bool_t        dbus_connection_set_timeout_functions        (DBusConnection             *connection,
                                                                 DBusAddTimeoutFunction      add_function,
                                                                 DBusRemoveTimeoutFunction   remove_function,
                                                                 DBusTimeoutToggledFunction  toggled_function,
                                                                 void                       *data,
                                                                 DBusFreeFunction            free_data_function);
void               dbus_connection_set_wakeup_main_function     (DBusConnection             *connection,
                                                                 DBusWakeupMainFunction      wakeup_main_function,
                                                                 void                       *data,
                                                                 DBusFreeFunction            free_data_function);
void               dbus_connection_set_dispatch_status_function (DBusConnection             *connection,
                                                                 DBusDispatchStatusFunction  function,
                                                                 void                       *data,
                                                                 DBusFreeFunction            free_data_function);
dbus_bool_t        dbus_connection_get_unix_user                (DBusConnection             *connection,
                                                                 unsigned long              *uid);
dbus_bool_t        dbus_connection_get_unix_process_id          (DBusConnection             *connection,
                                                                 unsigned long              *pid);
void               dbus_connection_set_unix_user_function       (DBusConnection             *connection,
                                                                 DBusAllowUnixUserFunction   function,
                                                                 void                       *data,
                                                                 DBusFreeFunction            free_data_function);


int          dbus_watch_get_fd      (DBusWatch        *watch);
unsigned int dbus_watch_get_flags   (DBusWatch        *watch);
void*        dbus_watch_get_data    (DBusWatch        *watch);
void         dbus_watch_set_data    (DBusWatch        *watch,
                                     void             *data,
                                     DBusFreeFunction  free_data_function);
dbus_bool_t  dbus_watch_handle      (DBusWatch        *watch,
                                     unsigned int      flags);
dbus_bool_t  dbus_watch_get_enabled (DBusWatch        *watch);

int         dbus_timeout_get_interval (DBusTimeout      *timeout);
void*       dbus_timeout_get_data     (DBusTimeout      *timeout);
void        dbus_timeout_set_data     (DBusTimeout      *timeout,
                                       void             *data,
                                       DBusFreeFunction  free_data_function);
dbus_bool_t dbus_timeout_handle       (DBusTimeout      *timeout);
dbus_bool_t dbus_timeout_get_enabled  (DBusTimeout      *timeout);

/* Filters */

dbus_bool_t dbus_connection_add_filter    (DBusConnection            *connection,
                                           DBusHandleMessageFunction  function,
                                           void                      *user_data,
                                           DBusFreeFunction           free_data_function);
void        dbus_connection_remove_filter (DBusConnection            *connection,
                                           DBusHandleMessageFunction  function,
                                           void                      *user_data);


/* Other */
dbus_bool_t dbus_connection_allocate_data_slot (dbus_int32_t     *slot_p);
void        dbus_connection_free_data_slot     (dbus_int32_t     *slot_p);
dbus_bool_t dbus_connection_set_data           (DBusConnection   *connection,
                                                dbus_int32_t      slot,
                                                void             *data,
                                                DBusFreeFunction  free_data_func);
void*       dbus_connection_get_data           (DBusConnection   *connection,
                                                dbus_int32_t      slot);

void        dbus_connection_set_change_sigpipe (dbus_bool_t       will_modify_sigpipe); 

void dbus_connection_set_max_message_size  (DBusConnection *connection,
                                            long            size);
long dbus_connection_get_max_message_size  (DBusConnection *connection);
void dbus_connection_set_max_received_size (DBusConnection *connection,
                                            long            size);
long dbus_connection_get_max_received_size (DBusConnection *connection);
long dbus_connection_get_outgoing_size     (DBusConnection *connection);

DBusPreallocatedSend* dbus_connection_preallocate_send       (DBusConnection       *connection);
void                  dbus_connection_free_preallocated_send (DBusConnection       *connection,
                                                              DBusPreallocatedSend *preallocated);
void                  dbus_connection_send_preallocated      (DBusConnection       *connection,
                                                              DBusPreallocatedSend *preallocated,
                                                              DBusMessage          *message,
                                                              dbus_uint32_t        *client_serial);


/* Object tree functionality */

typedef void              (* DBusObjectPathUnregisterFunction) (DBusConnection  *connection,
                                                                void            *user_data);
typedef DBusHandlerResult (* DBusObjectPathMessageFunction)    (DBusConnection  *connection,
                                                                DBusMessage     *message,
                                                                void            *user_data);

/**
 * Virtual table that must be implemented to handle a portion of the
 * object path hierarchy.
 */
struct DBusObjectPathVTable
{
  DBusObjectPathUnregisterFunction   unregister_function; /**< Function to unregister this handler */
  DBusObjectPathMessageFunction      message_function; /**< Function to handle messages */
  
  void (* dbus_internal_pad1) (void *); /**< Reserved for future expansion */
  void (* dbus_internal_pad2) (void *); /**< Reserved for future expansion */
  void (* dbus_internal_pad3) (void *); /**< Reserved for future expansion */
  void (* dbus_internal_pad4) (void *); /**< Reserved for future expansion */
};

dbus_bool_t dbus_connection_register_object_path   (DBusConnection              *connection,
                                                    const char                  *path,
                                                    const DBusObjectPathVTable  *vtable,
                                                    void                        *user_data);
dbus_bool_t dbus_connection_register_fallback      (DBusConnection              *connection,
                                                    const char                  *path,
                                                    const DBusObjectPathVTable  *vtable,
                                                    void                        *user_data);
dbus_bool_t dbus_connection_unregister_object_path (DBusConnection              *connection,
                                                    const char                  *path);

dbus_bool_t dbus_connection_list_registered        (DBusConnection              *connection,
                                                    const char                  *parent_path,
                                                    char                      ***child_entries);

dbus_bool_t dbus_connection_get_unix_fd            (DBusConnection              *connection,
                                                    int                         *fd);

DBUS_END_DECLS;

#endif /* DBUS_CONNECTION_H */
