/* Automatically generated gatestubs! Do not edit! */

#include <dbus/dbus.h>

#define _sfdc_strarg(a) _sfdc_strarg2(a)
#define _sfdc_strarg2(a) #a

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <aros/libcall.h>

void
dbus_address_entries_free(DBusAddressEntry ** ___entries);

AROS_LH1I(void, __dbus_address_entries_free,
	AROS_LHA(DBusAddressEntry **, ___entries, A0),
	struct Library *, _base, 16, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_address_entries_free(___entries);
  AROS_LIBFUNC_EXIT
}

const char *
dbus_address_entry_get_method(DBusAddressEntry * ___entry);

AROS_LH1I(const char *, __dbus_address_entry_get_method,
	AROS_LHA(DBusAddressEntry *, ___entry, A0),
	struct Library *, _base, 17, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_address_entry_get_method(___entry);
  AROS_LIBFUNC_EXIT
}

const char *
dbus_address_entry_get_value(DBusAddressEntry * ___entry, const char * ___key);

AROS_LH2I(const char *, __dbus_address_entry_get_value,
	AROS_LHA(DBusAddressEntry *, ___entry, A0),
	AROS_LHA(const char *, ___key, A1),
	struct Library *, _base, 18, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_address_entry_get_value(___entry, ___key);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_parse_address(const char * ___address, DBusAddressEntry *** ___entry, int * ___array_len, DBusError * ___error);

AROS_LH4I(dbus_bool_t, __dbus_parse_address,
	AROS_LHA(const char *, ___address, A0),
	AROS_LHA(DBusAddressEntry ***, ___entry, A1),
	AROS_LHA(int *, ___array_len, A2),
	AROS_LHA(DBusError *, ___error, A3),
	struct Library *, _base, 19, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_parse_address(___address, ___entry, ___array_len, ___error);
  AROS_LIBFUNC_EXIT
}

DBusConnection *
dbus_bus_get(DBusBusType ___type, DBusError * ___error);

AROS_LH2I(DBusConnection *, __dbus_bus_get,
	AROS_LHA(DBusBusType, ___type, D0),
	AROS_LHA(DBusError *, ___error, A0),
	struct Library *, _base, 20, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_bus_get(___type, ___error);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_bus_register(DBusConnection * ___connection, DBusError * ___error);

AROS_LH2I(dbus_bool_t, __dbus_bus_register,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(DBusError *, ___error, A1),
	struct Library *, _base, 21, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_bus_register(___connection, ___error);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_bus_set_base_service(DBusConnection * ___connection, const char * ___base_service);

AROS_LH2I(dbus_bool_t, __dbus_bus_set_base_service,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(const char *, ___base_service, A1),
	struct Library *, _base, 22, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_bus_set_base_service(___connection, ___base_service);
  AROS_LIBFUNC_EXIT
}

const char *
dbus_bus_get_base_service(DBusConnection * ___connection);

AROS_LH1I(const char *, __dbus_bus_get_base_service,
	AROS_LHA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 23, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_bus_get_base_service(___connection);
  AROS_LIBFUNC_EXIT
}

long unsigned int
dbus_bus_get_unix_user(DBusConnection * ___connection, const char * ___service, DBusError * ___error);

AROS_LH3I(long unsigned int, __dbus_bus_get_unix_user,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(const char *, ___service, A1),
	AROS_LHA(DBusError *, ___error, A2),
	struct Library *, _base, 24, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_bus_get_unix_user(___connection, ___service, ___error);
  AROS_LIBFUNC_EXIT
}

int
dbus_bus_acquire_service(DBusConnection * ___connection, const char * ___service_name, unsigned int ___flags, DBusError * ___error);

AROS_LH4I(int, __dbus_bus_acquire_service,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(const char *, ___service_name, A1),
	AROS_LHA(unsigned int, ___flags, D0),
	AROS_LHA(DBusError *, ___error, A2),
	struct Library *, _base, 25, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_bus_acquire_service(___connection, ___service_name, ___flags, ___error);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_bus_service_exists(DBusConnection * ___connection, const char * ___service_name, DBusError * ___error);

AROS_LH3I(dbus_bool_t, __dbus_bus_service_exists,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(const char *, ___service_name, A1),
	AROS_LHA(DBusError *, ___error, A2),
	struct Library *, _base, 26, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_bus_service_exists(___connection, ___service_name, ___error);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_bus_activate_service(DBusConnection * ___connection, const char * ___service_name, dbus_uint32_t ___flags, dbus_uint32_t * ___result, DBusError * ___error);

AROS_LH5I(dbus_bool_t, __dbus_bus_activate_service,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(const char *, ___service_name, A1),
	AROS_LHA(dbus_uint32_t, ___flags, D0),
	AROS_LHA(dbus_uint32_t *, ___result, A2),
	AROS_LHA(DBusError *, ___error, A3),
	struct Library *, _base, 27, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_bus_activate_service(___connection, ___service_name, ___flags, ___result, ___error);
  AROS_LIBFUNC_EXIT
}

void
dbus_bus_add_match(DBusConnection * ___connection, const char * ___rule, DBusError * ___error);

AROS_LH3I(void, __dbus_bus_add_match,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(const char *, ___rule, A1),
	AROS_LHA(DBusError *, ___error, A2),
	struct Library *, _base, 28, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_bus_add_match(___connection, ___rule, ___error);
  AROS_LIBFUNC_EXIT
}

void
dbus_bus_remove_match(DBusConnection * ___connection, const char * ___rule, DBusError * ___error);

AROS_LH3I(void, __dbus_bus_remove_match,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(const char *, ___rule, A1),
	AROS_LHA(DBusError *, ___error, A2),
	struct Library *, _base, 29, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_bus_remove_match(___connection, ___rule, ___error);
  AROS_LIBFUNC_EXIT
}

DBusConnection *
dbus_connection_open(const char * ___address, DBusError * ___error);

AROS_LH2I(DBusConnection *, __dbus_connection_open,
	AROS_LHA(const char *, ___address, A0),
	AROS_LHA(DBusError *, ___error, A1),
	struct Library *, _base, 30, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_open(___address, ___error);
  AROS_LIBFUNC_EXIT
}

DBusConnection *
dbus_connection_ref(DBusConnection * ___connection);

AROS_LH1I(DBusConnection *, __dbus_connection_ref,
	AROS_LHA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 31, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_ref(___connection);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_unref(DBusConnection * ___connection);

AROS_LH1I(void, __dbus_connection_unref,
	AROS_LHA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 32, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_unref(___connection);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_disconnect(DBusConnection * ___connection);

AROS_LH1I(void, __dbus_connection_disconnect,
	AROS_LHA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 33, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_disconnect(___connection);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_get_is_connected(DBusConnection * ___connection);

AROS_LH1I(dbus_bool_t, __dbus_connection_get_is_connected,
	AROS_LHA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 34, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_get_is_connected(___connection);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_get_is_authenticated(DBusConnection * ___connection);

AROS_LH1I(dbus_bool_t, __dbus_connection_get_is_authenticated,
	AROS_LHA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 35, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_get_is_authenticated(___connection);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_set_exit_on_disconnect(DBusConnection * ___connection, dbus_bool_t ___exit_on_disconnect);

AROS_LH2I(void, __dbus_connection_set_exit_on_disconnect,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(dbus_bool_t, ___exit_on_disconnect, D0),
	struct Library *, _base, 36, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_set_exit_on_disconnect(___connection, ___exit_on_disconnect);
  AROS_LIBFUNC_EXIT
}

DBusPreallocatedSend *
dbus_connection_preallocate_send(DBusConnection * ___connection);

AROS_LH1I(DBusPreallocatedSend *, __dbus_connection_preallocate_send,
	AROS_LHA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 37, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_preallocate_send(___connection);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_free_preallocated_send(DBusConnection * ___connection, DBusPreallocatedSend * ___preallocated);

AROS_LH2I(void, __dbus_connection_free_preallocated_send,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(DBusPreallocatedSend *, ___preallocated, A1),
	struct Library *, _base, 38, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_free_preallocated_send(___connection, ___preallocated);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_send_preallocated(DBusConnection * ___connection, DBusPreallocatedSend * ___preallocated, DBusMessage * ___message, dbus_uint32_t * ___client_serial);

AROS_LH4I(void, __dbus_connection_send_preallocated,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(DBusPreallocatedSend *, ___preallocated, A1),
	AROS_LHA(DBusMessage *, ___message, A2),
	AROS_LHA(dbus_uint32_t *, ___client_serial, A3),
	struct Library *, _base, 39, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_send_preallocated(___connection, ___preallocated, ___message, ___client_serial);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_send(DBusConnection * ___connection, DBusMessage * ___message, dbus_uint32_t * ___client_serial);

AROS_LH3I(dbus_bool_t, __dbus_connection_send,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(DBusMessage *, ___message, A1),
	AROS_LHA(dbus_uint32_t *, ___client_serial, A2),
	struct Library *, _base, 40, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_send(___connection, ___message, ___client_serial);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_send_with_reply(DBusConnection * ___connection, DBusMessage * ___message, DBusPendingCall ** ___pending_return, int ___timeout_milliseconds);

AROS_LH4I(dbus_bool_t, __dbus_connection_send_with_reply,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(DBusMessage *, ___message, A1),
	AROS_LHA(DBusPendingCall **, ___pending_return, A2),
	AROS_LHA(int, ___timeout_milliseconds, D0),
	struct Library *, _base, 41, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_send_with_reply(___connection, ___message, ___pending_return, ___timeout_milliseconds);
  AROS_LIBFUNC_EXIT
}

DBusMessage *
dbus_connection_send_with_reply_and_block(DBusConnection * ___connection, DBusMessage * ___message, int ___timeout_milliseconds, DBusError * ___error);

AROS_LH4I(DBusMessage *, __dbus_connection_send_with_reply_and_block,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(DBusMessage *, ___message, A1),
	AROS_LHA(int, ___timeout_milliseconds, D0),
	AROS_LHA(DBusError *, ___error, A2),
	struct Library *, _base, 42, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_send_with_reply_and_block(___connection, ___message, ___timeout_milliseconds, ___error);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_flush(DBusConnection * ___connection);

AROS_LH1I(void, __dbus_connection_flush,
	AROS_LHA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 43, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_flush(___connection);
  AROS_LIBFUNC_EXIT
}

DBusMessage *
dbus_connection_borrow_message(DBusConnection * ___connection);

AROS_LH1I(DBusMessage *, __dbus_connection_borrow_message,
	AROS_LHA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 44, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_borrow_message(___connection);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_return_message(DBusConnection * ___connection, DBusMessage * ___message);

AROS_LH2I(void, __dbus_connection_return_message,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(DBusMessage *, ___message, A1),
	struct Library *, _base, 45, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_return_message(___connection, ___message);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_steal_borrowed_message(DBusConnection * ___connection, DBusMessage * ___message);

AROS_LH2I(void, __dbus_connection_steal_borrowed_message,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(DBusMessage *, ___message, A1),
	struct Library *, _base, 46, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_steal_borrowed_message(___connection, ___message);
  AROS_LIBFUNC_EXIT
}

DBusMessage *
dbus_connection_pop_message(DBusConnection * ___connection);

AROS_LH1I(DBusMessage *, __dbus_connection_pop_message,
	AROS_LHA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 47, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_pop_message(___connection);
  AROS_LIBFUNC_EXIT
}

DBusDispatchStatus
dbus_connection_get_dispatch_status(DBusConnection * ___connection);

AROS_LH1I(DBusDispatchStatus, __dbus_connection_get_dispatch_status,
	AROS_LHA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 48, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_get_dispatch_status(___connection);
  AROS_LIBFUNC_EXIT
}

DBusDispatchStatus
dbus_connection_dispatch(DBusConnection * ___connection);

AROS_LH1I(DBusDispatchStatus, __dbus_connection_dispatch,
	AROS_LHA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 49, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_dispatch(___connection);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_set_watch_functions(DBusConnection * ___connection, DBusAddWatchFunction ___add_function, DBusRemoveWatchFunction ___remove_function, DBusWatchToggledFunction ___toggled_function, void * ___data, DBusFreeFunction ___free_data_function);

AROS_LH6I(dbus_bool_t, __dbus_connection_set_watch_functions,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(DBusAddWatchFunction, ___add_function, D0),
	AROS_LHA(DBusRemoveWatchFunction, ___remove_function, D1),
	AROS_LHA(DBusWatchToggledFunction, ___toggled_function, D2),
	AROS_LHA(void *, ___data, A1),
	AROS_LHA(DBusFreeFunction, ___free_data_function, D3),
	struct Library *, _base, 50, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_set_watch_functions(___connection, ___add_function, ___remove_function, ___toggled_function, ___data, ___free_data_function);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_set_timeout_functions(DBusConnection * ___connection, DBusAddTimeoutFunction ___add_function, DBusRemoveTimeoutFunction ___remove_function, DBusTimeoutToggledFunction ___toggled_function, void * ___data, DBusFreeFunction ___free_data_function);

AROS_LH6I(dbus_bool_t, __dbus_connection_set_timeout_functions,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(DBusAddTimeoutFunction, ___add_function, D0),
	AROS_LHA(DBusRemoveTimeoutFunction, ___remove_function, D1),
	AROS_LHA(DBusTimeoutToggledFunction, ___toggled_function, D2),
	AROS_LHA(void *, ___data, A1),
	AROS_LHA(DBusFreeFunction, ___free_data_function, D3),
	struct Library *, _base, 51, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_set_timeout_functions(___connection, ___add_function, ___remove_function, ___toggled_function, ___data, ___free_data_function);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_set_wakeup_main_function(DBusConnection * ___connection, DBusWakeupMainFunction ___wakeup_main_function, void * ___data, DBusFreeFunction ___free_data_function);

AROS_LH4I(void, __dbus_connection_set_wakeup_main_function,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(DBusWakeupMainFunction, ___wakeup_main_function, D0),
	AROS_LHA(void *, ___data, A1),
	AROS_LHA(DBusFreeFunction, ___free_data_function, D1),
	struct Library *, _base, 52, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_set_wakeup_main_function(___connection, ___wakeup_main_function, ___data, ___free_data_function);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_set_dispatch_status_function(DBusConnection * ___connection, DBusDispatchStatusFunction ___function, void * ___data, DBusFreeFunction ___free_data_function);

AROS_LH4I(void, __dbus_connection_set_dispatch_status_function,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(DBusDispatchStatusFunction, ___function, D0),
	AROS_LHA(void *, ___data, A1),
	AROS_LHA(DBusFreeFunction, ___free_data_function, D1),
	struct Library *, _base, 53, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_set_dispatch_status_function(___connection, ___function, ___data, ___free_data_function);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_get_unix_fd(DBusConnection * ___connection, int * ___fd);

AROS_LH2I(dbus_bool_t, __dbus_connection_get_unix_fd,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(int *, ___fd, A1),
	struct Library *, _base, 54, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_get_unix_fd(___connection, ___fd);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_get_unix_user(DBusConnection * ___connection, long unsigned int * ___uid);

AROS_LH2I(dbus_bool_t, __dbus_connection_get_unix_user,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(long unsigned int *, ___uid, A1),
	struct Library *, _base, 55, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_get_unix_user(___connection, ___uid);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_get_unix_process_id(DBusConnection * ___connection, long unsigned int * ___pid);

AROS_LH2I(dbus_bool_t, __dbus_connection_get_unix_process_id,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(long unsigned int *, ___pid, A1),
	struct Library *, _base, 56, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_get_unix_process_id(___connection, ___pid);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_set_unix_user_function(DBusConnection * ___connection, DBusAllowUnixUserFunction ___function, void * ___data, DBusFreeFunction ___free_data_function);

AROS_LH4I(void, __dbus_connection_set_unix_user_function,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(DBusAllowUnixUserFunction, ___function, D0),
	AROS_LHA(void *, ___data, A1),
	AROS_LHA(DBusFreeFunction, ___free_data_function, D1),
	struct Library *, _base, 57, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_set_unix_user_function(___connection, ___function, ___data, ___free_data_function);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_add_filter(DBusConnection * ___connection, DBusHandleMessageFunction ___function, void * ___user_data, DBusFreeFunction ___free_data_function);

AROS_LH4I(dbus_bool_t, __dbus_connection_add_filter,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(DBusHandleMessageFunction, ___function, D0),
	AROS_LHA(void *, ___user_data, A1),
	AROS_LHA(DBusFreeFunction, ___free_data_function, D1),
	struct Library *, _base, 58, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_add_filter(___connection, ___function, ___user_data, ___free_data_function);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_remove_filter(DBusConnection * ___connection, DBusHandleMessageFunction ___function, void * ___user_data);

AROS_LH3I(void, __dbus_connection_remove_filter,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(DBusHandleMessageFunction, ___function, D0),
	AROS_LHA(void *, ___user_data, A1),
	struct Library *, _base, 59, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_remove_filter(___connection, ___function, ___user_data);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_register_object_path(DBusConnection * ___connection, const char * ___path, const DBusObjectPathVTable * ___vtable, void * ___user_data);

AROS_LH4I(dbus_bool_t, __dbus_connection_register_object_path,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(const char *, ___path, A1),
	AROS_LHA(const DBusObjectPathVTable *, ___vtable, A2),
	AROS_LHA(void *, ___user_data, A3),
	struct Library *, _base, 60, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_register_object_path(___connection, ___path, ___vtable, ___user_data);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_register_fallback(DBusConnection * ___connection, const char * ___path, const DBusObjectPathVTable * ___vtable, void * ___user_data);

AROS_LH4I(dbus_bool_t, __dbus_connection_register_fallback,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(const char *, ___path, A1),
	AROS_LHA(const DBusObjectPathVTable *, ___vtable, A2),
	AROS_LHA(void *, ___user_data, A3),
	struct Library *, _base, 61, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_register_fallback(___connection, ___path, ___vtable, ___user_data);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_unregister_object_path(DBusConnection * ___connection, const char * ___path);

AROS_LH2I(dbus_bool_t, __dbus_connection_unregister_object_path,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(const char *, ___path, A1),
	struct Library *, _base, 62, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_unregister_object_path(___connection, ___path);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_list_registered(DBusConnection * ___connection, const char * ___parent_path, char *** ___child_entries);

AROS_LH3I(dbus_bool_t, __dbus_connection_list_registered,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(const char *, ___parent_path, A1),
	AROS_LHA(char ***, ___child_entries, A2),
	struct Library *, _base, 63, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_list_registered(___connection, ___parent_path, ___child_entries);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_allocate_data_slot(dbus_int32_t * ___slot_p);

AROS_LH1I(dbus_bool_t, __dbus_connection_allocate_data_slot,
	AROS_LHA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 64, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_allocate_data_slot(___slot_p);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_free_data_slot(dbus_int32_t * ___slot_p);

AROS_LH1I(void, __dbus_connection_free_data_slot,
	AROS_LHA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 65, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_free_data_slot(___slot_p);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_connection_set_data(DBusConnection * ___connection, dbus_int32_t ___slot, void * ___data, DBusFreeFunction ___free_data_func);

AROS_LH4I(dbus_bool_t, __dbus_connection_set_data,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(dbus_int32_t, ___slot, D0),
	AROS_LHA(void *, ___data, A1),
	AROS_LHA(DBusFreeFunction, ___free_data_func, D1),
	struct Library *, _base, 66, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_set_data(___connection, ___slot, ___data, ___free_data_func);
  AROS_LIBFUNC_EXIT
}

void *
dbus_connection_get_data(DBusConnection * ___connection, dbus_int32_t ___slot);

AROS_LH2I(void *, __dbus_connection_get_data,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(dbus_int32_t, ___slot, D0),
	struct Library *, _base, 67, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_get_data(___connection, ___slot);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_set_change_sigpipe(dbus_bool_t ___will_modify_sigpipe);

AROS_LH1I(void, __dbus_connection_set_change_sigpipe,
	AROS_LHA(dbus_bool_t, ___will_modify_sigpipe, D0),
	struct Library *, _base, 68, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_set_change_sigpipe(___will_modify_sigpipe);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_set_max_message_size(DBusConnection * ___connection, long int ___size);

AROS_LH2I(void, __dbus_connection_set_max_message_size,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(long int, ___size, D0),
	struct Library *, _base, 69, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_set_max_message_size(___connection, ___size);
  AROS_LIBFUNC_EXIT
}

long int
dbus_connection_get_max_message_size(DBusConnection * ___connection);

AROS_LH1I(long int, __dbus_connection_get_max_message_size,
	AROS_LHA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 70, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_get_max_message_size(___connection);
  AROS_LIBFUNC_EXIT
}

void
dbus_connection_set_max_received_size(DBusConnection * ___connection, long int ___size);

AROS_LH2I(void, __dbus_connection_set_max_received_size,
	AROS_LHA(DBusConnection *, ___connection, A0),
	AROS_LHA(long int, ___size, D0),
	struct Library *, _base, 71, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_set_max_received_size(___connection, ___size);
  AROS_LIBFUNC_EXIT
}

long int
dbus_connection_get_max_received_size(DBusConnection * ___connection);

AROS_LH1I(long int, __dbus_connection_get_max_received_size,
	AROS_LHA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 72, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_get_max_received_size(___connection);
  AROS_LIBFUNC_EXIT
}

long int
dbus_connection_get_outgoing_size(DBusConnection * ___connection);

AROS_LH1I(long int, __dbus_connection_get_outgoing_size,
	AROS_LHA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 73, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_connection_get_outgoing_size(___connection);
  AROS_LIBFUNC_EXIT
}

void
dbus_error_init(DBusError * ___error);

AROS_LH1I(void, __dbus_error_init,
	AROS_LHA(DBusError *, ___error, A0),
	struct Library *, _base, 74, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_error_init(___error);
  AROS_LIBFUNC_EXIT
}

void
dbus_error_free(DBusError * ___error);

AROS_LH1I(void, __dbus_error_free,
	AROS_LHA(DBusError *, ___error, A0),
	struct Library *, _base, 75, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_error_free(___error);
  AROS_LIBFUNC_EXIT
}

void
dbus_set_error_const(DBusError * ___error, const char * ___name, const char * ___message);

AROS_LH3I(void, __dbus_set_error_const,
	AROS_LHA(DBusError *, ___error, A0),
	AROS_LHA(const char *, ___name, A1),
	AROS_LHA(const char *, ___message, A2),
	struct Library *, _base, 76, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_set_error_const(___error, ___name, ___message);
  AROS_LIBFUNC_EXIT
}

void
dbus_move_error(DBusError * ___src, DBusError * ___dest);

AROS_LH2I(void, __dbus_move_error,
	AROS_LHA(DBusError *, ___src, A0),
	AROS_LHA(DBusError *, ___dest, A1),
	struct Library *, _base, 77, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_move_error(___src, ___dest);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_error_has_name(const DBusError * ___error, const char * ___name);

AROS_LH2I(dbus_bool_t, __dbus_error_has_name,
	AROS_LHA(const DBusError *, ___error, A0),
	AROS_LHA(const char *, ___name, A1),
	struct Library *, _base, 78, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_error_has_name(___error, ___name);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_error_is_set(const DBusError * ___error);

AROS_LH1I(dbus_bool_t, __dbus_error_is_set,
	AROS_LHA(const DBusError *, ___error, A0),
	struct Library *, _base, 79, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_error_is_set(___error);
  AROS_LIBFUNC_EXIT
}

#define __dbus_set_error AROS_SLIB_ENTRY(__dbus_set_error,Dbus)

void
dbus_set_error(DBusError * ___error, const char * ___name, const char * ___format, ...);

void
__dbus_set_error(DBusError * ___error, const char * ___name, const char * ___format, ...);

__asm(".globl " _sfdc_strarg(__dbus_set_error) );
__asm(".type  " _sfdc_strarg(__dbus_set_error) ", @function");
__asm(_sfdc_strarg(__dbus_set_error) ":");
#if defined(__mc68000__) || defined(__i386__)
__asm("jmp dbus_set_error");
#else
# error "Unknown CPU"
#endif

void *
dbus_malloc(size_t ___bytes);

AROS_LH1I(void *, __dbus_malloc,
	AROS_LHA(size_t, ___bytes, D0),
	struct Library *, _base, 81, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_malloc(___bytes);
  AROS_LIBFUNC_EXIT
}

void *
dbus_malloc0(size_t ___bytes);

AROS_LH1I(void *, __dbus_malloc0,
	AROS_LHA(size_t, ___bytes, D0),
	struct Library *, _base, 82, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_malloc0(___bytes);
  AROS_LIBFUNC_EXIT
}

void *
dbus_realloc(void * ___memory, size_t ___bytes);

AROS_LH2I(void *, __dbus_realloc,
	AROS_LHA(void *, ___memory, A0),
	AROS_LHA(size_t, ___bytes, D0),
	struct Library *, _base, 83, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_realloc(___memory, ___bytes);
  AROS_LIBFUNC_EXIT
}

void
dbus_free(void * ___memory);

AROS_LH1I(void, __dbus_free,
	AROS_LHA(void *, ___memory, A0),
	struct Library *, _base, 84, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_free(___memory);
  AROS_LIBFUNC_EXIT
}

void
dbus_free_string_array(char ** ___str_array);

AROS_LH1I(void, __dbus_free_string_array,
	AROS_LHA(char **, ___str_array, A0),
	struct Library *, _base, 85, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_free_string_array(___str_array);
  AROS_LIBFUNC_EXIT
}

void
dbus_shutdown(void);

AROS_LH0I(void, __dbus_shutdown,
	struct Library *, _base, 86, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_shutdown();
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_set_reply_serial(DBusMessage * ___message, dbus_uint32_t ___reply_serial);

AROS_LH2I(dbus_bool_t, __dbus_message_set_reply_serial,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(dbus_uint32_t, ___reply_serial, D0),
	struct Library *, _base, 87, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_set_reply_serial(___message, ___reply_serial);
  AROS_LIBFUNC_EXIT
}

dbus_uint32_t
dbus_message_get_serial(DBusMessage * ___message);

AROS_LH1I(dbus_uint32_t, __dbus_message_get_serial,
	AROS_LHA(DBusMessage *, ___message, A0),
	struct Library *, _base, 88, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_get_serial(___message);
  AROS_LIBFUNC_EXIT
}

dbus_uint32_t
dbus_message_get_reply_serial(DBusMessage * ___message);

AROS_LH1I(dbus_uint32_t, __dbus_message_get_reply_serial,
	AROS_LHA(DBusMessage *, ___message, A0),
	struct Library *, _base, 89, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_get_reply_serial(___message);
  AROS_LIBFUNC_EXIT
}

DBusMessage *
dbus_message_new(int ___message_type);

AROS_LH1I(DBusMessage *, __dbus_message_new,
	AROS_LHA(int, ___message_type, D0),
	struct Library *, _base, 90, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_new(___message_type);
  AROS_LIBFUNC_EXIT
}

DBusMessage *
dbus_message_new_method_call(const char * ___service, const char * ___path, const char * ___interface, const char * ___method);

AROS_LH4I(DBusMessage *, __dbus_message_new_method_call,
	AROS_LHA(const char *, ___service, A0),
	AROS_LHA(const char *, ___path, A1),
	AROS_LHA(const char *, ___interface, A2),
	AROS_LHA(const char *, ___method, A3),
	struct Library *, _base, 91, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_new_method_call(___service, ___path, ___interface, ___method);
  AROS_LIBFUNC_EXIT
}

DBusMessage *
dbus_message_new_method_return(DBusMessage * ___method_call);

AROS_LH1I(DBusMessage *, __dbus_message_new_method_return,
	AROS_LHA(DBusMessage *, ___method_call, A0),
	struct Library *, _base, 92, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_new_method_return(___method_call);
  AROS_LIBFUNC_EXIT
}

DBusMessage *
dbus_message_new_signal(const char * ___path, const char * ___interface, const char * ___name);

AROS_LH3I(DBusMessage *, __dbus_message_new_signal,
	AROS_LHA(const char *, ___path, A0),
	AROS_LHA(const char *, ___interface, A1),
	AROS_LHA(const char *, ___name, A2),
	struct Library *, _base, 93, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_new_signal(___path, ___interface, ___name);
  AROS_LIBFUNC_EXIT
}

DBusMessage *
dbus_message_new_error(DBusMessage * ___reply_to, const char * ___error_name, const char * ___error_message);

AROS_LH3I(DBusMessage *, __dbus_message_new_error,
	AROS_LHA(DBusMessage *, ___reply_to, A0),
	AROS_LHA(const char *, ___error_name, A1),
	AROS_LHA(const char *, ___error_message, A2),
	struct Library *, _base, 94, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_new_error(___reply_to, ___error_name, ___error_message);
  AROS_LIBFUNC_EXIT
}

#define __dbus_message_new_error_printf AROS_SLIB_ENTRY(__dbus_message_new_error_printf,Dbus)

DBusMessage *
dbus_message_new_error_printf(DBusMessage * ___reply_to, const char * ___error_name, const char * ___error_format, ...);

DBusMessage *
__dbus_message_new_error_printf(DBusMessage * ___reply_to, const char * ___error_name, const char * ___error_format, ...);

__asm(".globl " _sfdc_strarg(__dbus_message_new_error_printf) );
__asm(".type  " _sfdc_strarg(__dbus_message_new_error_printf) ", @function");
__asm(_sfdc_strarg(__dbus_message_new_error_printf) ":");
#if defined(__mc68000__) || defined(__i386__)
__asm("jmp dbus_message_new_error_printf");
#else
# error "Unknown CPU"
#endif

DBusMessage *
dbus_message_copy(const DBusMessage * ___message);

AROS_LH1I(DBusMessage *, __dbus_message_copy,
	AROS_LHA(const DBusMessage *, ___message, A0),
	struct Library *, _base, 96, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_copy(___message);
  AROS_LIBFUNC_EXIT
}

DBusMessage *
dbus_message_ref(DBusMessage * ___message);

AROS_LH1I(DBusMessage *, __dbus_message_ref,
	AROS_LHA(DBusMessage *, ___message, A0),
	struct Library *, _base, 97, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_ref(___message);
  AROS_LIBFUNC_EXIT
}

void
dbus_message_unref(DBusMessage * ___message);

AROS_LH1I(void, __dbus_message_unref,
	AROS_LHA(DBusMessage *, ___message, A0),
	struct Library *, _base, 98, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_unref(___message);
  AROS_LIBFUNC_EXIT
}

int
dbus_message_get_type(DBusMessage * ___message);

AROS_LH1I(int, __dbus_message_get_type,
	AROS_LHA(DBusMessage *, ___message, A0),
	struct Library *, _base, 99, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_get_type(___message);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_set_path(DBusMessage * ___message, const char * ___object_path);

AROS_LH2I(dbus_bool_t, __dbus_message_set_path,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(const char *, ___object_path, A1),
	struct Library *, _base, 100, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_set_path(___message, ___object_path);
  AROS_LIBFUNC_EXIT
}

const char *
dbus_message_get_path(DBusMessage * ___message);

AROS_LH1I(const char *, __dbus_message_get_path,
	AROS_LHA(DBusMessage *, ___message, A0),
	struct Library *, _base, 101, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_get_path(___message);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_get_path_decomposed(DBusMessage * ___message, char *** ___path);

AROS_LH2I(dbus_bool_t, __dbus_message_get_path_decomposed,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(char ***, ___path, A1),
	struct Library *, _base, 102, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_get_path_decomposed(___message, ___path);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_set_interface(DBusMessage * ___message, const char * ___interface);

AROS_LH2I(dbus_bool_t, __dbus_message_set_interface,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(const char *, ___interface, A1),
	struct Library *, _base, 103, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_set_interface(___message, ___interface);
  AROS_LIBFUNC_EXIT
}

const char *
dbus_message_get_interface(DBusMessage * ___message);

AROS_LH1I(const char *, __dbus_message_get_interface,
	AROS_LHA(DBusMessage *, ___message, A0),
	struct Library *, _base, 104, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_get_interface(___message);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_set_member(DBusMessage * ___message, const char * ___member);

AROS_LH2I(dbus_bool_t, __dbus_message_set_member,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(const char *, ___member, A1),
	struct Library *, _base, 105, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_set_member(___message, ___member);
  AROS_LIBFUNC_EXIT
}

const char *
dbus_message_get_member(DBusMessage * ___message);

AROS_LH1I(const char *, __dbus_message_get_member,
	AROS_LHA(DBusMessage *, ___message, A0),
	struct Library *, _base, 106, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_get_member(___message);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_set_error_name(DBusMessage * ___message, const char * ___error_name);

AROS_LH2I(dbus_bool_t, __dbus_message_set_error_name,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(const char *, ___error_name, A1),
	struct Library *, _base, 107, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_set_error_name(___message, ___error_name);
  AROS_LIBFUNC_EXIT
}

const char *
dbus_message_get_error_name(DBusMessage * ___message);

AROS_LH1I(const char *, __dbus_message_get_error_name,
	AROS_LHA(DBusMessage *, ___message, A0),
	struct Library *, _base, 108, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_get_error_name(___message);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_set_destination(DBusMessage * ___message, const char * ___destination);

AROS_LH2I(dbus_bool_t, __dbus_message_set_destination,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(const char *, ___destination, A1),
	struct Library *, _base, 109, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_set_destination(___message, ___destination);
  AROS_LIBFUNC_EXIT
}

const char *
dbus_message_get_destination(DBusMessage * ___message);

AROS_LH1I(const char *, __dbus_message_get_destination,
	AROS_LHA(DBusMessage *, ___message, A0),
	struct Library *, _base, 110, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_get_destination(___message);
  AROS_LIBFUNC_EXIT
}

#define __dbus_message_append_args AROS_SLIB_ENTRY(__dbus_message_append_args,Dbus)

dbus_bool_t
dbus_message_append_args(DBusMessage * ___message, int ___first_arg_type, ...);

dbus_bool_t
__dbus_message_append_args(DBusMessage * ___message, int ___first_arg_type, ...);

__asm(".globl " _sfdc_strarg(__dbus_message_append_args) );
__asm(".type  " _sfdc_strarg(__dbus_message_append_args) ", @function");
__asm(_sfdc_strarg(__dbus_message_append_args) ":");
#if defined(__mc68000__) || defined(__i386__)
__asm("jmp dbus_message_append_args");
#else
# error "Unknown CPU"
#endif

#define __dbus_message_get_args AROS_SLIB_ENTRY(__dbus_message_get_args,Dbus)

dbus_bool_t
dbus_message_get_args(DBusMessage * ___message, DBusError * ___error, int ___first_arg_type, ...);

dbus_bool_t
__dbus_message_get_args(DBusMessage * ___message, DBusError * ___error, int ___first_arg_type, ...);

__asm(".globl " _sfdc_strarg(__dbus_message_get_args) );
__asm(".type  " _sfdc_strarg(__dbus_message_get_args) ", @function");
__asm(_sfdc_strarg(__dbus_message_get_args) ":");
#if defined(__mc68000__) || defined(__i386__)
__asm("jmp dbus_message_get_args");
#else
# error "Unknown CPU"
#endif

dbus_bool_t
dbus_message_get_args_valist(DBusMessage * ___message, DBusError * ___error, int ___first_arg_type, va_list ___var_args);

AROS_LH4I(dbus_bool_t, __dbus_message_get_args_valist,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(DBusError *, ___error, A1),
	AROS_LHA(int, ___first_arg_type, D0),
	AROS_LHA(va_list, ___var_args, D1),
	struct Library *, _base, 113, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_get_args_valist(___message, ___error, ___first_arg_type, ___var_args);
  AROS_LIBFUNC_EXIT
}

#define __dbus_message_iter_get_args AROS_SLIB_ENTRY(__dbus_message_iter_get_args,Dbus)

dbus_bool_t
dbus_message_iter_get_args(DBusMessageIter * ___iter, DBusError * ___error, int ___first_arg_type, ...);

dbus_bool_t
__dbus_message_iter_get_args(DBusMessageIter * ___iter, DBusError * ___error, int ___first_arg_type, ...);

__asm(".globl " _sfdc_strarg(__dbus_message_iter_get_args) );
__asm(".type  " _sfdc_strarg(__dbus_message_iter_get_args) ", @function");
__asm(_sfdc_strarg(__dbus_message_iter_get_args) ":");
#if defined(__mc68000__) || defined(__i386__)
__asm("jmp dbus_message_iter_get_args");
#else
# error "Unknown CPU"
#endif

dbus_bool_t
dbus_message_iter_init(DBusMessage * ___message, DBusMessageIter * ___iter);

AROS_LH2I(dbus_bool_t, __dbus_message_iter_init,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(DBusMessageIter *, ___iter, A1),
	struct Library *, _base, 115, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_init(___message, ___iter);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_has_next(DBusMessageIter * ___iter);

AROS_LH1I(dbus_bool_t, __dbus_message_iter_has_next,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 116, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_has_next(___iter);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_next(DBusMessageIter * ___iter);

AROS_LH1I(dbus_bool_t, __dbus_message_iter_next,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 117, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_next(___iter);
  AROS_LIBFUNC_EXIT
}

int
dbus_message_iter_get_arg_type(DBusMessageIter * ___iter);

AROS_LH1I(int, __dbus_message_iter_get_arg_type,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 118, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_arg_type(___iter);
  AROS_LIBFUNC_EXIT
}

int
dbus_message_iter_get_array_type(DBusMessageIter * ___iter);

AROS_LH1I(int, __dbus_message_iter_get_array_type,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 119, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_array_type(___iter);
  AROS_LIBFUNC_EXIT
}

char *
dbus_message_iter_get_string(DBusMessageIter * ___iter);

AROS_LH1I(char *, __dbus_message_iter_get_string,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 120, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_string(___iter);
  AROS_LIBFUNC_EXIT
}

char *
dbus_message_iter_get_object_path(DBusMessageIter * ___iter);

AROS_LH1I(char *, __dbus_message_iter_get_object_path,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 121, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_object_path(___iter);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_get_custom(DBusMessageIter * ___iter, char ** ___name, unsigned char ** ___value, int * ___len);

AROS_LH4I(dbus_bool_t, __dbus_message_iter_get_custom,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(char **, ___name, A1),
	AROS_LHA(unsigned char **, ___value, A2),
	AROS_LHA(int *, ___len, A3),
	struct Library *, _base, 122, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_custom(___iter, ___name, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_get_args_valist(DBusMessageIter * ___iter, DBusError * ___error, int ___first_arg_type, va_list ___var_args);

AROS_LH4I(dbus_bool_t, __dbus_message_iter_get_args_valist,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(DBusError *, ___error, A1),
	AROS_LHA(int, ___first_arg_type, D0),
	AROS_LHA(va_list, ___var_args, D1),
	struct Library *, _base, 123, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_args_valist(___iter, ___error, ___first_arg_type, ___var_args);
  AROS_LIBFUNC_EXIT
}

unsigned char
dbus_message_iter_get_byte(DBusMessageIter * ___iter);

AROS_LH1I(unsigned char, __dbus_message_iter_get_byte,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 124, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_byte(___iter);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_get_boolean(DBusMessageIter * ___iter);

AROS_LH1I(dbus_bool_t, __dbus_message_iter_get_boolean,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 125, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_boolean(___iter);
  AROS_LIBFUNC_EXIT
}

dbus_int32_t
dbus_message_iter_get_int32(DBusMessageIter * ___iter);

AROS_LH1I(dbus_int32_t, __dbus_message_iter_get_int32,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 126, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_int32(___iter);
  AROS_LIBFUNC_EXIT
}

dbus_uint32_t
dbus_message_iter_get_uint32(DBusMessageIter * ___iter);

AROS_LH1I(dbus_uint32_t, __dbus_message_iter_get_uint32,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 127, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_uint32(___iter);
  AROS_LIBFUNC_EXIT
}

dbus_int64_t
dbus_message_iter_get_int64(DBusMessageIter * ___iter);

AROS_LH1I(dbus_int64_t, __dbus_message_iter_get_int64,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 128, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_int64(___iter);
  AROS_LIBFUNC_EXIT
}

dbus_uint64_t
dbus_message_iter_get_uint64(DBusMessageIter * ___iter);

AROS_LH1I(dbus_uint64_t, __dbus_message_iter_get_uint64,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 129, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_uint64(___iter);
  AROS_LIBFUNC_EXIT
}

double
dbus_message_iter_get_double(DBusMessageIter * ___iter);

AROS_LH1I(double, __dbus_message_iter_get_double,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 130, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_double(___iter);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_init_array_iterator(DBusMessageIter * ___iter, DBusMessageIter * ___array_iter, int * ___array_type);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_init_array_iterator,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(DBusMessageIter *, ___array_iter, A1),
	AROS_LHA(int *, ___array_type, A2),
	struct Library *, _base, 131, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_init_array_iterator(___iter, ___array_iter, ___array_type);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_init_dict_iterator(DBusMessageIter * ___iter, DBusMessageIter * ___dict_iter);

AROS_LH2I(dbus_bool_t, __dbus_message_iter_init_dict_iterator,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(DBusMessageIter *, ___dict_iter, A1),
	struct Library *, _base, 132, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_init_dict_iterator(___iter, ___dict_iter);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_get_byte_array(DBusMessageIter * ___iter, unsigned char ** ___value, int * ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_get_byte_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(unsigned char **, ___value, A1),
	AROS_LHA(int *, ___len, A2),
	struct Library *, _base, 133, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_byte_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_get_boolean_array(DBusMessageIter * ___iter, unsigned char ** ___value, int * ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_get_boolean_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(unsigned char **, ___value, A1),
	AROS_LHA(int *, ___len, A2),
	struct Library *, _base, 134, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_boolean_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_get_int32_array(DBusMessageIter * ___iter, dbus_int32_t ** ___value, int * ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_get_int32_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(dbus_int32_t **, ___value, A1),
	AROS_LHA(int *, ___len, A2),
	struct Library *, _base, 135, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_int32_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_get_uint32_array(DBusMessageIter * ___iter, dbus_uint32_t ** ___value, int * ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_get_uint32_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(dbus_uint32_t **, ___value, A1),
	AROS_LHA(int *, ___len, A2),
	struct Library *, _base, 136, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_uint32_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_get_int64_array(DBusMessageIter * ___iter, dbus_int64_t ** ___value, int * ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_get_int64_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(dbus_int64_t **, ___value, A1),
	AROS_LHA(int *, ___len, A2),
	struct Library *, _base, 137, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_int64_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_get_uint64_array(DBusMessageIter * ___iter, dbus_uint64_t ** ___value, int * ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_get_uint64_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(dbus_uint64_t **, ___value, A1),
	AROS_LHA(int *, ___len, A2),
	struct Library *, _base, 138, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_uint64_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_get_double_array(DBusMessageIter * ___iter, double ** ___value, int * ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_get_double_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(double **, ___value, A1),
	AROS_LHA(int *, ___len, A2),
	struct Library *, _base, 139, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_double_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_get_string_array(DBusMessageIter * ___iter, char *** ___value, int * ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_get_string_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(char ***, ___value, A1),
	AROS_LHA(int *, ___len, A2),
	struct Library *, _base, 140, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_string_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_get_object_path_array(DBusMessageIter * ___iter, char *** ___value, int * ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_get_object_path_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(char ***, ___value, A1),
	AROS_LHA(int *, ___len, A2),
	struct Library *, _base, 141, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_object_path_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

char *
dbus_message_iter_get_dict_key(DBusMessageIter * ___iter);

AROS_LH1I(char *, __dbus_message_iter_get_dict_key,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 142, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_get_dict_key(___iter);
  AROS_LIBFUNC_EXIT
}

void
dbus_message_append_iter_init(DBusMessage * ___message, DBusMessageIter * ___iter);

AROS_LH2I(void, __dbus_message_append_iter_init,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(DBusMessageIter *, ___iter, A1),
	struct Library *, _base, 143, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_append_iter_init(___message, ___iter);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_nil(DBusMessageIter * ___iter);

AROS_LH1I(dbus_bool_t, __dbus_message_iter_append_nil,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 144, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_nil(___iter);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_boolean(DBusMessageIter * ___iter, dbus_bool_t ___value);

AROS_LH2I(dbus_bool_t, __dbus_message_iter_append_boolean,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(dbus_bool_t, ___value, D0),
	struct Library *, _base, 145, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_boolean(___iter, ___value);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_byte(DBusMessageIter * ___iter, unsigned char ___value);

AROS_LH2I(dbus_bool_t, __dbus_message_iter_append_byte,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(unsigned char, ___value, D0),
	struct Library *, _base, 146, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_byte(___iter, ___value);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_int32(DBusMessageIter * ___iter, dbus_int32_t ___value);

AROS_LH2I(dbus_bool_t, __dbus_message_iter_append_int32,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(dbus_int32_t, ___value, D0),
	struct Library *, _base, 147, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_int32(___iter, ___value);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_uint32(DBusMessageIter * ___iter, dbus_uint32_t ___value);

AROS_LH2I(dbus_bool_t, __dbus_message_iter_append_uint32,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(dbus_uint32_t, ___value, D0),
	struct Library *, _base, 148, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_uint32(___iter, ___value);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_int64(DBusMessageIter * ___iter, dbus_int64_t ___value);

AROS_LH2I(dbus_bool_t, __dbus_message_iter_append_int64,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(dbus_int64_t, ___value, D0),
	struct Library *, _base, 149, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_int64(___iter, ___value);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_uint64(DBusMessageIter * ___iter, dbus_uint64_t ___value);

AROS_LH2I(dbus_bool_t, __dbus_message_iter_append_uint64,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(dbus_uint64_t, ___value, D0),
	struct Library *, _base, 150, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_uint64(___iter, ___value);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_double(DBusMessageIter * ___iter, double ___value);

AROS_LH2I(dbus_bool_t, __dbus_message_iter_append_double,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(double, ___value, D0),
	struct Library *, _base, 151, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_double(___iter, ___value);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_string(DBusMessageIter * ___iter, const char * ___value);

AROS_LH2I(dbus_bool_t, __dbus_message_iter_append_string,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(const char *, ___value, A1),
	struct Library *, _base, 152, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_string(___iter, ___value);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_object_path(DBusMessageIter * ___iter, const char * ___value);

AROS_LH2I(dbus_bool_t, __dbus_message_iter_append_object_path,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(const char *, ___value, A1),
	struct Library *, _base, 153, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_object_path(___iter, ___value);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_custom(DBusMessageIter * ___iter, const char * ___name, const unsigned char * ___data, int ___len);

AROS_LH4I(dbus_bool_t, __dbus_message_iter_append_custom,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(const char *, ___name, A1),
	AROS_LHA(const unsigned char *, ___data, A2),
	AROS_LHA(int, ___len, D0),
	struct Library *, _base, 154, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_custom(___iter, ___name, ___data, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_dict_key(DBusMessageIter * ___iter, const char * ___value);

AROS_LH2I(dbus_bool_t, __dbus_message_iter_append_dict_key,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(const char *, ___value, A1),
	struct Library *, _base, 155, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_dict_key(___iter, ___value);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_array(DBusMessageIter * ___iter, DBusMessageIter * ___array_iter, int ___element_type);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_append_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(DBusMessageIter *, ___array_iter, A1),
	AROS_LHA(int, ___element_type, D0),
	struct Library *, _base, 156, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_array(___iter, ___array_iter, ___element_type);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_dict(DBusMessageIter * ___iter, DBusMessageIter * ___dict_iter);

AROS_LH2I(dbus_bool_t, __dbus_message_iter_append_dict,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(DBusMessageIter *, ___dict_iter, A1),
	struct Library *, _base, 157, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_dict(___iter, ___dict_iter);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_append_args_valist(DBusMessage * ___message, int ___first_arg_type, va_list ___var_args);

AROS_LH3I(dbus_bool_t, __dbus_message_append_args_valist,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(int, ___first_arg_type, D0),
	AROS_LHA(va_list, ___var_args, D1),
	struct Library *, _base, 158, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_append_args_valist(___message, ___first_arg_type, ___var_args);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_boolean_array(DBusMessageIter * ___iter, const unsigned char * ___value, int ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_append_boolean_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(const unsigned char *, ___value, A1),
	AROS_LHA(int, ___len, D0),
	struct Library *, _base, 159, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_boolean_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_int32_array(DBusMessageIter * ___iter, const dbus_int32_t * ___value, int ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_append_int32_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(const dbus_int32_t *, ___value, A1),
	AROS_LHA(int, ___len, D0),
	struct Library *, _base, 160, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_int32_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_uint32_array(DBusMessageIter * ___iter, const dbus_uint32_t * ___value, int ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_append_uint32_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(const dbus_uint32_t *, ___value, A1),
	AROS_LHA(int, ___len, D0),
	struct Library *, _base, 161, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_uint32_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_int64_array(DBusMessageIter * ___iter, const dbus_int64_t * ___value, int ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_append_int64_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(const dbus_int64_t *, ___value, A1),
	AROS_LHA(int, ___len, D0),
	struct Library *, _base, 162, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_int64_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_uint64_array(DBusMessageIter * ___iter, const dbus_uint64_t * ___value, int ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_append_uint64_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(const dbus_uint64_t *, ___value, A1),
	AROS_LHA(int, ___len, D0),
	struct Library *, _base, 163, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_uint64_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_double_array(DBusMessageIter * ___iter, const double * ___value, int ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_append_double_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(const double *, ___value, A1),
	AROS_LHA(int, ___len, D0),
	struct Library *, _base, 164, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_double_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_byte_array(DBusMessageIter * ___iter, const unsigned char * ___value, int ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_append_byte_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(const unsigned char *, ___value, A1),
	AROS_LHA(int, ___len, D0),
	struct Library *, _base, 165, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_byte_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_string_array(DBusMessageIter * ___iter, const char ** ___value, int ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_append_string_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(const char **, ___value, A1),
	AROS_LHA(int, ___len, D0),
	struct Library *, _base, 166, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_string_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_iter_append_object_path_array(DBusMessageIter * ___iter, const char ** ___value, int ___len);

AROS_LH3I(dbus_bool_t, __dbus_message_iter_append_object_path_array,
	AROS_LHA(DBusMessageIter *, ___iter, A0),
	AROS_LHA(const char **, ___value, A1),
	AROS_LHA(int, ___len, D0),
	struct Library *, _base, 167, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_iter_append_object_path_array(___iter, ___value, ___len);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_set_sender(DBusMessage * ___message, const char * ___sender);

AROS_LH2I(dbus_bool_t, __dbus_message_set_sender,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(const char *, ___sender, A1),
	struct Library *, _base, 168, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_set_sender(___message, ___sender);
  AROS_LIBFUNC_EXIT
}

void
dbus_message_set_no_reply(DBusMessage * ___message, dbus_bool_t ___no_reply);

AROS_LH2I(void, __dbus_message_set_no_reply,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(dbus_bool_t, ___no_reply, D0),
	struct Library *, _base, 169, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_set_no_reply(___message, ___no_reply);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_get_no_reply(DBusMessage * ___message);

AROS_LH1I(dbus_bool_t, __dbus_message_get_no_reply,
	AROS_LHA(DBusMessage *, ___message, A0),
	struct Library *, _base, 170, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_get_no_reply(___message);
  AROS_LIBFUNC_EXIT
}

void
dbus_message_set_auto_activation(DBusMessage * ___message, dbus_bool_t ___auto_activation);

AROS_LH2I(void, __dbus_message_set_auto_activation,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(dbus_bool_t, ___auto_activation, D0),
	struct Library *, _base, 171, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_set_auto_activation(___message, ___auto_activation);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_get_auto_activation(DBusMessage * ___message);

AROS_LH1I(dbus_bool_t, __dbus_message_get_auto_activation,
	AROS_LHA(DBusMessage *, ___message, A0),
	struct Library *, _base, 172, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_get_auto_activation(___message);
  AROS_LIBFUNC_EXIT
}

const char *
dbus_message_get_sender(DBusMessage * ___message);

AROS_LH1I(const char *, __dbus_message_get_sender,
	AROS_LHA(DBusMessage *, ___message, A0),
	struct Library *, _base, 173, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_get_sender(___message);
  AROS_LIBFUNC_EXIT
}

const char *
dbus_message_get_signature(DBusMessage * ___message);

AROS_LH1I(const char *, __dbus_message_get_signature,
	AROS_LHA(DBusMessage *, ___message, A0),
	struct Library *, _base, 174, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_get_signature(___message);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_is_method_call(DBusMessage * ___message, const char * ___interface, const char * ___method);

AROS_LH3I(dbus_bool_t, __dbus_message_is_method_call,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(const char *, ___interface, A1),
	AROS_LHA(const char *, ___method, A2),
	struct Library *, _base, 175, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_is_method_call(___message, ___interface, ___method);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_is_signal(DBusMessage * ___message, const char * ___interface, const char * ___signal_name);

AROS_LH3I(dbus_bool_t, __dbus_message_is_signal,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(const char *, ___interface, A1),
	AROS_LHA(const char *, ___signal_name, A2),
	struct Library *, _base, 176, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_is_signal(___message, ___interface, ___signal_name);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_is_error(DBusMessage * ___message, const char * ___error_name);

AROS_LH2I(dbus_bool_t, __dbus_message_is_error,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(const char *, ___error_name, A1),
	struct Library *, _base, 177, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_is_error(___message, ___error_name);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_has_destination(DBusMessage * ___message, const char * ___service);

AROS_LH2I(dbus_bool_t, __dbus_message_has_destination,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(const char *, ___service, A1),
	struct Library *, _base, 178, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_has_destination(___message, ___service);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_has_sender(DBusMessage * ___message, const char * ___service);

AROS_LH2I(dbus_bool_t, __dbus_message_has_sender,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(const char *, ___service, A1),
	struct Library *, _base, 179, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_has_sender(___message, ___service);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_has_signature(DBusMessage * ___message, const char * ___signature);

AROS_LH2I(dbus_bool_t, __dbus_message_has_signature,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(const char *, ___signature, A1),
	struct Library *, _base, 180, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_has_signature(___message, ___signature);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_set_error_from_message(DBusError * ___error, DBusMessage * ___message);

AROS_LH2I(dbus_bool_t, __dbus_set_error_from_message,
	AROS_LHA(DBusError *, ___error, A0),
	AROS_LHA(DBusMessage *, ___message, A1),
	struct Library *, _base, 181, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_set_error_from_message(___error, ___message);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_allocate_data_slot(dbus_int32_t * ___slot_p);

AROS_LH1I(dbus_bool_t, __dbus_message_allocate_data_slot,
	AROS_LHA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 182, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_allocate_data_slot(___slot_p);
  AROS_LIBFUNC_EXIT
}

void
dbus_message_free_data_slot(dbus_int32_t * ___slot_p);

AROS_LH1I(void, __dbus_message_free_data_slot,
	AROS_LHA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 183, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_free_data_slot(___slot_p);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_message_set_data(DBusMessage * ___message, dbus_int32_t ___slot, void * ___data, DBusFreeFunction ___free_data_func);

AROS_LH4I(dbus_bool_t, __dbus_message_set_data,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(dbus_int32_t, ___slot, D0),
	AROS_LHA(void *, ___data, A1),
	AROS_LHA(DBusFreeFunction, ___free_data_func, D1),
	struct Library *, _base, 184, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_set_data(___message, ___slot, ___data, ___free_data_func);
  AROS_LIBFUNC_EXIT
}

void *
dbus_message_get_data(DBusMessage * ___message, dbus_int32_t ___slot);

AROS_LH2I(void *, __dbus_message_get_data,
	AROS_LHA(DBusMessage *, ___message, A0),
	AROS_LHA(dbus_int32_t, ___slot, D0),
	struct Library *, _base, 185, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_get_data(___message, ___slot);
  AROS_LIBFUNC_EXIT
}

int
dbus_message_type_from_string(const char * ___type_str);

AROS_LH1I(int, __dbus_message_type_from_string,
	AROS_LHA(const char *, ___type_str, A0),
	struct Library *, _base, 186, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_message_type_from_string(___type_str);
  AROS_LIBFUNC_EXIT
}

DBusPendingCall *
dbus_pending_call_ref(DBusPendingCall * ___pending);

AROS_LH1I(DBusPendingCall *, __dbus_pending_call_ref,
	AROS_LHA(DBusPendingCall *, ___pending, A0),
	struct Library *, _base, 187, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_pending_call_ref(___pending);
  AROS_LIBFUNC_EXIT
}

void
dbus_pending_call_unref(DBusPendingCall * ___pending);

AROS_LH1I(void, __dbus_pending_call_unref,
	AROS_LHA(DBusPendingCall *, ___pending, A0),
	struct Library *, _base, 188, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_pending_call_unref(___pending);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_pending_call_set_notify(DBusPendingCall * ___pending, DBusPendingCallNotifyFunction ___function, void * ___user_data, DBusFreeFunction ___free_user_data);

AROS_LH4I(dbus_bool_t, __dbus_pending_call_set_notify,
	AROS_LHA(DBusPendingCall *, ___pending, A0),
	AROS_LHA(DBusPendingCallNotifyFunction, ___function, D0),
	AROS_LHA(void *, ___user_data, A1),
	AROS_LHA(DBusFreeFunction, ___free_user_data, D1),
	struct Library *, _base, 189, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_pending_call_set_notify(___pending, ___function, ___user_data, ___free_user_data);
  AROS_LIBFUNC_EXIT
}

void
dbus_pending_call_cancel(DBusPendingCall * ___pending);

AROS_LH1I(void, __dbus_pending_call_cancel,
	AROS_LHA(DBusPendingCall *, ___pending, A0),
	struct Library *, _base, 190, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_pending_call_cancel(___pending);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_pending_call_get_completed(DBusPendingCall * ___pending);

AROS_LH1I(dbus_bool_t, __dbus_pending_call_get_completed,
	AROS_LHA(DBusPendingCall *, ___pending, A0),
	struct Library *, _base, 191, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_pending_call_get_completed(___pending);
  AROS_LIBFUNC_EXIT
}

DBusMessage *
dbus_pending_call_get_reply(DBusPendingCall * ___pending);

AROS_LH1I(DBusMessage *, __dbus_pending_call_get_reply,
	AROS_LHA(DBusPendingCall *, ___pending, A0),
	struct Library *, _base, 192, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_pending_call_get_reply(___pending);
  AROS_LIBFUNC_EXIT
}

void
dbus_pending_call_block(DBusPendingCall * ___pending);

AROS_LH1I(void, __dbus_pending_call_block,
	AROS_LHA(DBusPendingCall *, ___pending, A0),
	struct Library *, _base, 193, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_pending_call_block(___pending);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_pending_call_allocate_data_slot(dbus_int32_t * ___slot_p);

AROS_LH1I(dbus_bool_t, __dbus_pending_call_allocate_data_slot,
	AROS_LHA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 194, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_pending_call_allocate_data_slot(___slot_p);
  AROS_LIBFUNC_EXIT
}

void
dbus_pending_call_free_data_slot(dbus_int32_t * ___slot_p);

AROS_LH1I(void, __dbus_pending_call_free_data_slot,
	AROS_LHA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 195, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_pending_call_free_data_slot(___slot_p);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_pending_call_set_data(DBusPendingCall * ___pending, dbus_int32_t ___slot, void * ___data, DBusFreeFunction ___free_data_func);

AROS_LH4I(dbus_bool_t, __dbus_pending_call_set_data,
	AROS_LHA(DBusPendingCall *, ___pending, A0),
	AROS_LHA(dbus_int32_t, ___slot, D0),
	AROS_LHA(void *, ___data, A1),
	AROS_LHA(DBusFreeFunction, ___free_data_func, D1),
	struct Library *, _base, 196, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_pending_call_set_data(___pending, ___slot, ___data, ___free_data_func);
  AROS_LIBFUNC_EXIT
}

void *
dbus_pending_call_get_data(DBusPendingCall * ___pending, dbus_int32_t ___slot);

AROS_LH2I(void *, __dbus_pending_call_get_data,
	AROS_LHA(DBusPendingCall *, ___pending, A0),
	AROS_LHA(dbus_int32_t, ___slot, D0),
	struct Library *, _base, 197, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_pending_call_get_data(___pending, ___slot);
  AROS_LIBFUNC_EXIT
}

DBusServer *
dbus_server_listen(const char * ___address, DBusError * ___error);

AROS_LH2I(DBusServer *, __dbus_server_listen,
	AROS_LHA(const char *, ___address, A0),
	AROS_LHA(DBusError *, ___error, A1),
	struct Library *, _base, 198, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_server_listen(___address, ___error);
  AROS_LIBFUNC_EXIT
}

DBusServer *
dbus_server_ref(DBusServer * ___server);

AROS_LH1I(DBusServer *, __dbus_server_ref,
	AROS_LHA(DBusServer *, ___server, A0),
	struct Library *, _base, 199, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_server_ref(___server);
  AROS_LIBFUNC_EXIT
}

void
dbus_server_unref(DBusServer * ___server);

AROS_LH1I(void, __dbus_server_unref,
	AROS_LHA(DBusServer *, ___server, A0),
	struct Library *, _base, 200, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_server_unref(___server);
  AROS_LIBFUNC_EXIT
}

void
dbus_server_disconnect(DBusServer * ___server);

AROS_LH1I(void, __dbus_server_disconnect,
	AROS_LHA(DBusServer *, ___server, A0),
	struct Library *, _base, 201, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_server_disconnect(___server);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_server_get_is_connected(DBusServer * ___server);

AROS_LH1I(dbus_bool_t, __dbus_server_get_is_connected,
	AROS_LHA(DBusServer *, ___server, A0),
	struct Library *, _base, 202, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_server_get_is_connected(___server);
  AROS_LIBFUNC_EXIT
}

char *
dbus_server_get_address(DBusServer * ___server);

AROS_LH1I(char *, __dbus_server_get_address,
	AROS_LHA(DBusServer *, ___server, A0),
	struct Library *, _base, 203, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_server_get_address(___server);
  AROS_LIBFUNC_EXIT
}

void
dbus_server_set_new_connection_function(DBusServer * ___server, DBusNewConnectionFunction ___function, void * ___data, DBusFreeFunction ___free_data_function);

AROS_LH4I(void, __dbus_server_set_new_connection_function,
	AROS_LHA(DBusServer *, ___server, A0),
	AROS_LHA(DBusNewConnectionFunction, ___function, D0),
	AROS_LHA(void *, ___data, A1),
	AROS_LHA(DBusFreeFunction, ___free_data_function, D1),
	struct Library *, _base, 204, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_server_set_new_connection_function(___server, ___function, ___data, ___free_data_function);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_server_set_watch_functions(DBusServer * ___server, DBusAddWatchFunction ___add_function, DBusRemoveWatchFunction ___remove_function, DBusWatchToggledFunction ___toggled_function, void * ___data, DBusFreeFunction ___free_data_function);

AROS_LH6I(dbus_bool_t, __dbus_server_set_watch_functions,
	AROS_LHA(DBusServer *, ___server, A0),
	AROS_LHA(DBusAddWatchFunction, ___add_function, D0),
	AROS_LHA(DBusRemoveWatchFunction, ___remove_function, D1),
	AROS_LHA(DBusWatchToggledFunction, ___toggled_function, D2),
	AROS_LHA(void *, ___data, A1),
	AROS_LHA(DBusFreeFunction, ___free_data_function, D3),
	struct Library *, _base, 205, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_server_set_watch_functions(___server, ___add_function, ___remove_function, ___toggled_function, ___data, ___free_data_function);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_server_set_timeout_functions(DBusServer * ___server, DBusAddTimeoutFunction ___add_function, DBusRemoveTimeoutFunction ___remove_function, DBusTimeoutToggledFunction ___toggled_function, void * ___data, DBusFreeFunction ___free_data_function);

AROS_LH6I(dbus_bool_t, __dbus_server_set_timeout_functions,
	AROS_LHA(DBusServer *, ___server, A0),
	AROS_LHA(DBusAddTimeoutFunction, ___add_function, D0),
	AROS_LHA(DBusRemoveTimeoutFunction, ___remove_function, D1),
	AROS_LHA(DBusTimeoutToggledFunction, ___toggled_function, D2),
	AROS_LHA(void *, ___data, A1),
	AROS_LHA(DBusFreeFunction, ___free_data_function, D3),
	struct Library *, _base, 206, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_server_set_timeout_functions(___server, ___add_function, ___remove_function, ___toggled_function, ___data, ___free_data_function);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_server_set_auth_mechanisms(DBusServer * ___server, const char ** ___mechanisms);

AROS_LH2I(dbus_bool_t, __dbus_server_set_auth_mechanisms,
	AROS_LHA(DBusServer *, ___server, A0),
	AROS_LHA(const char **, ___mechanisms, A1),
	struct Library *, _base, 207, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_server_set_auth_mechanisms(___server, ___mechanisms);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_server_allocate_data_slot(dbus_int32_t * ___slot_p);

AROS_LH1I(dbus_bool_t, __dbus_server_allocate_data_slot,
	AROS_LHA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 208, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_server_allocate_data_slot(___slot_p);
  AROS_LIBFUNC_EXIT
}

void
dbus_server_free_data_slot(dbus_int32_t * ___slot_p);

AROS_LH1I(void, __dbus_server_free_data_slot,
	AROS_LHA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 209, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_server_free_data_slot(___slot_p);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_server_set_data(DBusServer * ___server, int ___slot, void * ___data, DBusFreeFunction ___free_data_func);

AROS_LH4I(dbus_bool_t, __dbus_server_set_data,
	AROS_LHA(DBusServer *, ___server, A0),
	AROS_LHA(int, ___slot, D0),
	AROS_LHA(void *, ___data, A1),
	AROS_LHA(DBusFreeFunction, ___free_data_func, D1),
	struct Library *, _base, 210, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_server_set_data(___server, ___slot, ___data, ___free_data_func);
  AROS_LIBFUNC_EXIT
}

void *
dbus_server_get_data(DBusServer * ___server, int ___slot);

AROS_LH2I(void *, __dbus_server_get_data,
	AROS_LHA(DBusServer *, ___server, A0),
	AROS_LHA(int, ___slot, D0),
	struct Library *, _base, 211, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_server_get_data(___server, ___slot);
  AROS_LIBFUNC_EXIT
}

void
dbus_internal_do_not_use_run_tests(const char * ___test_data_dir);

AROS_LH1I(void, __dbus_internal_do_not_use_run_tests,
	AROS_LHA(const char *, ___test_data_dir, A0),
	struct Library *, _base, 212, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_internal_do_not_use_run_tests(___test_data_dir);
  AROS_LIBFUNC_EXIT
}

DBusMutex *
dbus_mutex_new(void);

AROS_LH0I(DBusMutex *, __dbus_mutex_new,
	struct Library *, _base, 213, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_mutex_new();
  AROS_LIBFUNC_EXIT
}

void
dbus_mutex_free(DBusMutex * ___mutex);

AROS_LH1I(void, __dbus_mutex_free,
	AROS_LHA(DBusMutex *, ___mutex, A0),
	struct Library *, _base, 214, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_mutex_free(___mutex);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_mutex_lock(DBusMutex * ___mutex);

AROS_LH1I(dbus_bool_t, __dbus_mutex_lock,
	AROS_LHA(DBusMutex *, ___mutex, A0),
	struct Library *, _base, 215, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_mutex_lock(___mutex);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_mutex_unlock(DBusMutex * ___mutex);

AROS_LH1I(dbus_bool_t, __dbus_mutex_unlock,
	AROS_LHA(DBusMutex *, ___mutex, A0),
	struct Library *, _base, 216, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_mutex_unlock(___mutex);
  AROS_LIBFUNC_EXIT
}

DBusCondVar *
dbus_condvar_new(void);

AROS_LH0I(DBusCondVar *, __dbus_condvar_new,
	struct Library *, _base, 217, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_condvar_new();
  AROS_LIBFUNC_EXIT
}

void
dbus_condvar_free(DBusCondVar * ___cond);

AROS_LH1I(void, __dbus_condvar_free,
	AROS_LHA(DBusCondVar *, ___cond, A0),
	struct Library *, _base, 218, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_condvar_free(___cond);
  AROS_LIBFUNC_EXIT
}

void
dbus_condvar_wait(DBusCondVar * ___cond, DBusMutex * ___mutex);

AROS_LH2I(void, __dbus_condvar_wait,
	AROS_LHA(DBusCondVar *, ___cond, A0),
	AROS_LHA(DBusMutex *, ___mutex, A1),
	struct Library *, _base, 219, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_condvar_wait(___cond, ___mutex);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_condvar_wait_timeout(DBusCondVar * ___cond, DBusMutex * ___mutex, int ___timeout_milliseconds);

AROS_LH3I(dbus_bool_t, __dbus_condvar_wait_timeout,
	AROS_LHA(DBusCondVar *, ___cond, A0),
	AROS_LHA(DBusMutex *, ___mutex, A1),
	AROS_LHA(int, ___timeout_milliseconds, D0),
	struct Library *, _base, 220, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_condvar_wait_timeout(___cond, ___mutex, ___timeout_milliseconds);
  AROS_LIBFUNC_EXIT
}

void
dbus_condvar_wake_one(DBusCondVar * ___cond);

AROS_LH1I(void, __dbus_condvar_wake_one,
	AROS_LHA(DBusCondVar *, ___cond, A0),
	struct Library *, _base, 221, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_condvar_wake_one(___cond);
  AROS_LIBFUNC_EXIT
}

void
dbus_condvar_wake_all(DBusCondVar * ___cond);

AROS_LH1I(void, __dbus_condvar_wake_all,
	AROS_LHA(DBusCondVar *, ___cond, A0),
	struct Library *, _base, 222, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_condvar_wake_all(___cond);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_threads_init(const DBusThreadFunctions * ___functions);

AROS_LH1I(dbus_bool_t, __dbus_threads_init,
	AROS_LHA(const DBusThreadFunctions *, ___functions, A0),
	struct Library *, _base, 223, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_threads_init(___functions);
  AROS_LIBFUNC_EXIT
}

int
dbus_timeout_get_interval(DBusTimeout * ___timeout);

AROS_LH1I(int, __dbus_timeout_get_interval,
	AROS_LHA(DBusTimeout *, ___timeout, A0),
	struct Library *, _base, 224, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_timeout_get_interval(___timeout);
  AROS_LIBFUNC_EXIT
}

void *
dbus_timeout_get_data(DBusTimeout * ___timeout);

AROS_LH1I(void *, __dbus_timeout_get_data,
	AROS_LHA(DBusTimeout *, ___timeout, A0),
	struct Library *, _base, 225, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_timeout_get_data(___timeout);
  AROS_LIBFUNC_EXIT
}

void
dbus_timeout_set_data(DBusTimeout * ___timeout, void * ___data, DBusFreeFunction ___free_data_function);

AROS_LH3I(void, __dbus_timeout_set_data,
	AROS_LHA(DBusTimeout *, ___timeout, A0),
	AROS_LHA(void *, ___data, A1),
	AROS_LHA(DBusFreeFunction, ___free_data_function, D0),
	struct Library *, _base, 226, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_timeout_set_data(___timeout, ___data, ___free_data_function);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_timeout_handle(DBusTimeout * ___timeout);

AROS_LH1I(dbus_bool_t, __dbus_timeout_handle,
	AROS_LHA(DBusTimeout *, ___timeout, A0),
	struct Library *, _base, 227, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_timeout_handle(___timeout);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_timeout_get_enabled(DBusTimeout * ___timeout);

AROS_LH1I(dbus_bool_t, __dbus_timeout_get_enabled,
	AROS_LHA(DBusTimeout *, ___timeout, A0),
	struct Library *, _base, 228, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_timeout_get_enabled(___timeout);
  AROS_LIBFUNC_EXIT
}

int
dbus_watch_get_fd(DBusWatch * ___watch);

AROS_LH1I(int, __dbus_watch_get_fd,
	AROS_LHA(DBusWatch *, ___watch, A0),
	struct Library *, _base, 229, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_watch_get_fd(___watch);
  AROS_LIBFUNC_EXIT
}

unsigned int
dbus_watch_get_flags(DBusWatch * ___watch);

AROS_LH1I(unsigned int, __dbus_watch_get_flags,
	AROS_LHA(DBusWatch *, ___watch, A0),
	struct Library *, _base, 230, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_watch_get_flags(___watch);
  AROS_LIBFUNC_EXIT
}

void *
dbus_watch_get_data(DBusWatch * ___watch);

AROS_LH1I(void *, __dbus_watch_get_data,
	AROS_LHA(DBusWatch *, ___watch, A0),
	struct Library *, _base, 231, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_watch_get_data(___watch);
  AROS_LIBFUNC_EXIT
}

void
dbus_watch_set_data(DBusWatch * ___watch, void * ___data, DBusFreeFunction ___free_data_function);

AROS_LH3I(void, __dbus_watch_set_data,
	AROS_LHA(DBusWatch *, ___watch, A0),
	AROS_LHA(void *, ___data, A1),
	AROS_LHA(DBusFreeFunction, ___free_data_function, D0),
	struct Library *, _base, 232, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_watch_set_data(___watch, ___data, ___free_data_function);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_watch_get_enabled(DBusWatch * ___watch);

AROS_LH1I(dbus_bool_t, __dbus_watch_get_enabled,
	AROS_LHA(DBusWatch *, ___watch, A0),
	struct Library *, _base, 233, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_watch_get_enabled(___watch);
  AROS_LIBFUNC_EXIT
}

dbus_bool_t
dbus_watch_handle(DBusWatch * ___watch, unsigned int ___flags);

AROS_LH2I(dbus_bool_t, __dbus_watch_handle,
	AROS_LHA(DBusWatch *, ___watch, A0),
	AROS_LHA(unsigned int, ___flags, D0),
	struct Library *, _base, 234, Dbus)
{
  AROS_LIBFUNC_INIT
  return dbus_watch_handle(___watch, ___flags);
  AROS_LIBFUNC_EXIT
}


#ifdef __cplusplus
}
#endif /* __cplusplus */
