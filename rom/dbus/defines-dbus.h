/* Automatically generated header! Do not edit! */

#ifndef _INLINE_DBUS_H
#define _INLINE_DBUS_H

#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#ifndef DBUS_BASE_NAME
#define DBUS_BASE_NAME DBUSBase
#endif /* !DBUS_BASE_NAME */

#define dbus_address_entries_free(___entries) \
	AROS_LC1(void, dbus_address_entries_free, \
	AROS_LCA(DBusAddressEntry **, (___entries), A0), \
	struct Library *, DBUS_BASE_NAME, 16, Dbus)

#define dbus_address_entry_get_method(___entry) \
	AROS_LC1(const char *, dbus_address_entry_get_method, \
	AROS_LCA(DBusAddressEntry *, (___entry), A0), \
	struct Library *, DBUS_BASE_NAME, 17, Dbus)

#define dbus_address_entry_get_value(___entry, ___key) \
	AROS_LC2(const char *, dbus_address_entry_get_value, \
	AROS_LCA(DBusAddressEntry *, (___entry), A0), \
	AROS_LCA(const char *, (___key), A1), \
	struct Library *, DBUS_BASE_NAME, 18, Dbus)

#define dbus_parse_address(___address, ___entry, ___array_len, ___error) \
	AROS_LC4(dbus_bool_t, dbus_parse_address, \
	AROS_LCA(const char *, (___address), A0), \
	AROS_LCA(DBusAddressEntry ***, (___entry), A1), \
	AROS_LCA(int *, (___array_len), A2), \
	AROS_LCA(DBusError *, (___error), A3), \
	struct Library *, DBUS_BASE_NAME, 19, Dbus)

#define dbus_bus_get(___type, ___error) \
	AROS_LC2(DBusConnection *, dbus_bus_get, \
	AROS_LCA(DBusBusType, (___type), D0), \
	AROS_LCA(DBusError *, (___error), A0), \
	struct Library *, DBUS_BASE_NAME, 20, Dbus)

#define dbus_bus_register(___connection, ___error) \
	AROS_LC2(dbus_bool_t, dbus_bus_register, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(DBusError *, (___error), A1), \
	struct Library *, DBUS_BASE_NAME, 21, Dbus)

#define dbus_bus_set_base_service(___connection, ___base_service) \
	AROS_LC2(dbus_bool_t, dbus_bus_set_base_service, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(const char *, (___base_service), A1), \
	struct Library *, DBUS_BASE_NAME, 22, Dbus)

#define dbus_bus_get_base_service(___connection) \
	AROS_LC1(const char *, dbus_bus_get_base_service, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	struct Library *, DBUS_BASE_NAME, 23, Dbus)

#define dbus_bus_get_unix_user(___connection, ___service, ___error) \
	AROS_LC3(long unsigned int, dbus_bus_get_unix_user, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(const char *, (___service), A1), \
	AROS_LCA(DBusError *, (___error), A2), \
	struct Library *, DBUS_BASE_NAME, 24, Dbus)

#define dbus_bus_acquire_service(___connection, ___service_name, ___flags, ___error) \
	AROS_LC4(int, dbus_bus_acquire_service, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(const char *, (___service_name), A1), \
	AROS_LCA(unsigned int, (___flags), D0), \
	AROS_LCA(DBusError *, (___error), A2), \
	struct Library *, DBUS_BASE_NAME, 25, Dbus)

#define dbus_bus_service_exists(___connection, ___service_name, ___error) \
	AROS_LC3(dbus_bool_t, dbus_bus_service_exists, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(const char *, (___service_name), A1), \
	AROS_LCA(DBusError *, (___error), A2), \
	struct Library *, DBUS_BASE_NAME, 26, Dbus)

#define dbus_bus_activate_service(___connection, ___service_name, ___flags, ___result, ___error) \
	AROS_LC5(dbus_bool_t, dbus_bus_activate_service, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(const char *, (___service_name), A1), \
	AROS_LCA(dbus_uint32_t, (___flags), D0), \
	AROS_LCA(dbus_uint32_t *, (___result), A2), \
	AROS_LCA(DBusError *, (___error), A3), \
	struct Library *, DBUS_BASE_NAME, 27, Dbus)

#define dbus_bus_add_match(___connection, ___rule, ___error) \
	AROS_LC3(void, dbus_bus_add_match, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(const char *, (___rule), A1), \
	AROS_LCA(DBusError *, (___error), A2), \
	struct Library *, DBUS_BASE_NAME, 28, Dbus)

#define dbus_bus_remove_match(___connection, ___rule, ___error) \
	AROS_LC3(void, dbus_bus_remove_match, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(const char *, (___rule), A1), \
	AROS_LCA(DBusError *, (___error), A2), \
	struct Library *, DBUS_BASE_NAME, 29, Dbus)

#define dbus_connection_open(___address, ___error) \
	AROS_LC2(DBusConnection *, dbus_connection_open, \
	AROS_LCA(const char *, (___address), A0), \
	AROS_LCA(DBusError *, (___error), A1), \
	struct Library *, DBUS_BASE_NAME, 30, Dbus)

#define dbus_connection_ref(___connection) \
	AROS_LC1(DBusConnection *, dbus_connection_ref, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	struct Library *, DBUS_BASE_NAME, 31, Dbus)

#define dbus_connection_unref(___connection) \
	AROS_LC1(void, dbus_connection_unref, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	struct Library *, DBUS_BASE_NAME, 32, Dbus)

#define dbus_connection_disconnect(___connection) \
	AROS_LC1(void, dbus_connection_disconnect, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	struct Library *, DBUS_BASE_NAME, 33, Dbus)

#define dbus_connection_get_is_connected(___connection) \
	AROS_LC1(dbus_bool_t, dbus_connection_get_is_connected, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	struct Library *, DBUS_BASE_NAME, 34, Dbus)

#define dbus_connection_get_is_authenticated(___connection) \
	AROS_LC1(dbus_bool_t, dbus_connection_get_is_authenticated, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	struct Library *, DBUS_BASE_NAME, 35, Dbus)

#define dbus_connection_set_exit_on_disconnect(___connection, ___exit_on_disconnect) \
	AROS_LC2(void, dbus_connection_set_exit_on_disconnect, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(dbus_bool_t, (___exit_on_disconnect), D0), \
	struct Library *, DBUS_BASE_NAME, 36, Dbus)

#define dbus_connection_preallocate_send(___connection) \
	AROS_LC1(DBusPreallocatedSend *, dbus_connection_preallocate_send, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	struct Library *, DBUS_BASE_NAME, 37, Dbus)

#define dbus_connection_free_preallocated_send(___connection, ___preallocated) \
	AROS_LC2(void, dbus_connection_free_preallocated_send, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(DBusPreallocatedSend *, (___preallocated), A1), \
	struct Library *, DBUS_BASE_NAME, 38, Dbus)

#define dbus_connection_send_preallocated(___connection, ___preallocated, ___message, ___client_serial) \
	AROS_LC4(void, dbus_connection_send_preallocated, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(DBusPreallocatedSend *, (___preallocated), A1), \
	AROS_LCA(DBusMessage *, (___message), A2), \
	AROS_LCA(dbus_uint32_t *, (___client_serial), A3), \
	struct Library *, DBUS_BASE_NAME, 39, Dbus)

#define dbus_connection_send(___connection, ___message, ___client_serial) \
	AROS_LC3(dbus_bool_t, dbus_connection_send, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(DBusMessage *, (___message), A1), \
	AROS_LCA(dbus_uint32_t *, (___client_serial), A2), \
	struct Library *, DBUS_BASE_NAME, 40, Dbus)

#define dbus_connection_send_with_reply(___connection, ___message, ___pending_return, ___timeout_milliseconds) \
	AROS_LC4(dbus_bool_t, dbus_connection_send_with_reply, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(DBusMessage *, (___message), A1), \
	AROS_LCA(DBusPendingCall **, (___pending_return), A2), \
	AROS_LCA(int, (___timeout_milliseconds), D0), \
	struct Library *, DBUS_BASE_NAME, 41, Dbus)

#define dbus_connection_send_with_reply_and_block(___connection, ___message, ___timeout_milliseconds, ___error) \
	AROS_LC4(DBusMessage *, dbus_connection_send_with_reply_and_block, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(DBusMessage *, (___message), A1), \
	AROS_LCA(int, (___timeout_milliseconds), D0), \
	AROS_LCA(DBusError *, (___error), A2), \
	struct Library *, DBUS_BASE_NAME, 42, Dbus)

#define dbus_connection_flush(___connection) \
	AROS_LC1(void, dbus_connection_flush, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	struct Library *, DBUS_BASE_NAME, 43, Dbus)

#define dbus_connection_borrow_message(___connection) \
	AROS_LC1(DBusMessage *, dbus_connection_borrow_message, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	struct Library *, DBUS_BASE_NAME, 44, Dbus)

#define dbus_connection_return_message(___connection, ___message) \
	AROS_LC2(void, dbus_connection_return_message, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(DBusMessage *, (___message), A1), \
	struct Library *, DBUS_BASE_NAME, 45, Dbus)

#define dbus_connection_steal_borrowed_message(___connection, ___message) \
	AROS_LC2(void, dbus_connection_steal_borrowed_message, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(DBusMessage *, (___message), A1), \
	struct Library *, DBUS_BASE_NAME, 46, Dbus)

#define dbus_connection_pop_message(___connection) \
	AROS_LC1(DBusMessage *, dbus_connection_pop_message, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	struct Library *, DBUS_BASE_NAME, 47, Dbus)

#define dbus_connection_get_dispatch_status(___connection) \
	AROS_LC1(DBusDispatchStatus, dbus_connection_get_dispatch_status, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	struct Library *, DBUS_BASE_NAME, 48, Dbus)

#define dbus_connection_dispatch(___connection) \
	AROS_LC1(DBusDispatchStatus, dbus_connection_dispatch, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	struct Library *, DBUS_BASE_NAME, 49, Dbus)

#define dbus_connection_set_watch_functions(___connection, ___add_function, ___remove_function, ___toggled_function, ___data, ___free_data_function) \
	AROS_LC6(dbus_bool_t, dbus_connection_set_watch_functions, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(DBusAddWatchFunction, (___add_function), D0), \
	AROS_LCA(DBusRemoveWatchFunction, (___remove_function), D1), \
	AROS_LCA(DBusWatchToggledFunction, (___toggled_function), D2), \
	AROS_LCA(void *, (___data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_data_function), D3), \
	struct Library *, DBUS_BASE_NAME, 50, Dbus)

#define dbus_connection_set_timeout_functions(___connection, ___add_function, ___remove_function, ___toggled_function, ___data, ___free_data_function) \
	AROS_LC6(dbus_bool_t, dbus_connection_set_timeout_functions, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(DBusAddTimeoutFunction, (___add_function), D0), \
	AROS_LCA(DBusRemoveTimeoutFunction, (___remove_function), D1), \
	AROS_LCA(DBusTimeoutToggledFunction, (___toggled_function), D2), \
	AROS_LCA(void *, (___data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_data_function), D3), \
	struct Library *, DBUS_BASE_NAME, 51, Dbus)

#define dbus_connection_set_wakeup_main_function(___connection, ___wakeup_main_function, ___data, ___free_data_function) \
	AROS_LC4(void, dbus_connection_set_wakeup_main_function, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(DBusWakeupMainFunction, (___wakeup_main_function), D0), \
	AROS_LCA(void *, (___data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_data_function), D1), \
	struct Library *, DBUS_BASE_NAME, 52, Dbus)

#define dbus_connection_set_dispatch_status_function(___connection, ___function, ___data, ___free_data_function) \
	AROS_LC4(void, dbus_connection_set_dispatch_status_function, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(DBusDispatchStatusFunction, (___function), D0), \
	AROS_LCA(void *, (___data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_data_function), D1), \
	struct Library *, DBUS_BASE_NAME, 53, Dbus)

#define dbus_connection_get_unix_fd(___connection, ___fd) \
	AROS_LC2(dbus_bool_t, dbus_connection_get_unix_fd, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(int *, (___fd), A1), \
	struct Library *, DBUS_BASE_NAME, 54, Dbus)

#define dbus_connection_get_unix_user(___connection, ___uid) \
	AROS_LC2(dbus_bool_t, dbus_connection_get_unix_user, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(long unsigned int *, (___uid), A1), \
	struct Library *, DBUS_BASE_NAME, 55, Dbus)

#define dbus_connection_get_unix_process_id(___connection, ___pid) \
	AROS_LC2(dbus_bool_t, dbus_connection_get_unix_process_id, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(long unsigned int *, (___pid), A1), \
	struct Library *, DBUS_BASE_NAME, 56, Dbus)

#define dbus_connection_set_unix_user_function(___connection, ___function, ___data, ___free_data_function) \
	AROS_LC4(void, dbus_connection_set_unix_user_function, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(DBusAllowUnixUserFunction, (___function), D0), \
	AROS_LCA(void *, (___data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_data_function), D1), \
	struct Library *, DBUS_BASE_NAME, 57, Dbus)

#define dbus_connection_add_filter(___connection, ___function, ___user_data, ___free_data_function) \
	AROS_LC4(dbus_bool_t, dbus_connection_add_filter, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(DBusHandleMessageFunction, (___function), D0), \
	AROS_LCA(void *, (___user_data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_data_function), D1), \
	struct Library *, DBUS_BASE_NAME, 58, Dbus)

#define dbus_connection_remove_filter(___connection, ___function, ___user_data) \
	AROS_LC3(void, dbus_connection_remove_filter, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(DBusHandleMessageFunction, (___function), D0), \
	AROS_LCA(void *, (___user_data), A1), \
	struct Library *, DBUS_BASE_NAME, 59, Dbus)

#define dbus_connection_register_object_path(___connection, ___path, ___vtable, ___user_data) \
	AROS_LC4(dbus_bool_t, dbus_connection_register_object_path, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(const char *, (___path), A1), \
	AROS_LCA(const DBusObjectPathVTable *, (___vtable), A2), \
	AROS_LCA(void *, (___user_data), A3), \
	struct Library *, DBUS_BASE_NAME, 60, Dbus)

#define dbus_connection_register_fallback(___connection, ___path, ___vtable, ___user_data) \
	AROS_LC4(dbus_bool_t, dbus_connection_register_fallback, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(const char *, (___path), A1), \
	AROS_LCA(const DBusObjectPathVTable *, (___vtable), A2), \
	AROS_LCA(void *, (___user_data), A3), \
	struct Library *, DBUS_BASE_NAME, 61, Dbus)

#define dbus_connection_unregister_object_path(___connection, ___path) \
	AROS_LC2(dbus_bool_t, dbus_connection_unregister_object_path, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(const char *, (___path), A1), \
	struct Library *, DBUS_BASE_NAME, 62, Dbus)

#define dbus_connection_list_registered(___connection, ___parent_path, ___child_entries) \
	AROS_LC3(dbus_bool_t, dbus_connection_list_registered, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(const char *, (___parent_path), A1), \
	AROS_LCA(char ***, (___child_entries), A2), \
	struct Library *, DBUS_BASE_NAME, 63, Dbus)

#define dbus_connection_allocate_data_slot(___slot_p) \
	AROS_LC1(dbus_bool_t, dbus_connection_allocate_data_slot, \
	AROS_LCA(dbus_int32_t *, (___slot_p), A0), \
	struct Library *, DBUS_BASE_NAME, 64, Dbus)

#define dbus_connection_free_data_slot(___slot_p) \
	AROS_LC1(void, dbus_connection_free_data_slot, \
	AROS_LCA(dbus_int32_t *, (___slot_p), A0), \
	struct Library *, DBUS_BASE_NAME, 65, Dbus)

#define dbus_connection_set_data(___connection, ___slot, ___data, ___free_data_func) \
	AROS_LC4(dbus_bool_t, dbus_connection_set_data, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(dbus_int32_t, (___slot), D0), \
	AROS_LCA(void *, (___data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_data_func), D1), \
	struct Library *, DBUS_BASE_NAME, 66, Dbus)

#define dbus_connection_get_data(___connection, ___slot) \
	AROS_LC2(void *, dbus_connection_get_data, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(dbus_int32_t, (___slot), D0), \
	struct Library *, DBUS_BASE_NAME, 67, Dbus)

#define dbus_connection_set_change_sigpipe(___will_modify_sigpipe) \
	AROS_LC1(void, dbus_connection_set_change_sigpipe, \
	AROS_LCA(dbus_bool_t, (___will_modify_sigpipe), D0), \
	struct Library *, DBUS_BASE_NAME, 68, Dbus)

#define dbus_connection_set_max_message_size(___connection, ___size) \
	AROS_LC2(void, dbus_connection_set_max_message_size, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(long int, (___size), D0), \
	struct Library *, DBUS_BASE_NAME, 69, Dbus)

#define dbus_connection_get_max_message_size(___connection) \
	AROS_LC1(long int, dbus_connection_get_max_message_size, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	struct Library *, DBUS_BASE_NAME, 70, Dbus)

#define dbus_connection_set_max_received_size(___connection, ___size) \
	AROS_LC2(void, dbus_connection_set_max_received_size, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	AROS_LCA(long int, (___size), D0), \
	struct Library *, DBUS_BASE_NAME, 71, Dbus)

#define dbus_connection_get_max_received_size(___connection) \
	AROS_LC1(long int, dbus_connection_get_max_received_size, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	struct Library *, DBUS_BASE_NAME, 72, Dbus)

#define dbus_connection_get_outgoing_size(___connection) \
	AROS_LC1(long int, dbus_connection_get_outgoing_size, \
	AROS_LCA(DBusConnection *, (___connection), A0), \
	struct Library *, DBUS_BASE_NAME, 73, Dbus)

#define dbus_error_init(___error) \
	AROS_LC1(void, dbus_error_init, \
	AROS_LCA(DBusError *, (___error), A0), \
	struct Library *, DBUS_BASE_NAME, 74, Dbus)

#define dbus_error_free(___error) \
	AROS_LC1(void, dbus_error_free, \
	AROS_LCA(DBusError *, (___error), A0), \
	struct Library *, DBUS_BASE_NAME, 75, Dbus)

#define dbus_set_error_const(___error, ___name, ___message) \
	AROS_LC3(void, dbus_set_error_const, \
	AROS_LCA(DBusError *, (___error), A0), \
	AROS_LCA(const char *, (___name), A1), \
	AROS_LCA(const char *, (___message), A2), \
	struct Library *, DBUS_BASE_NAME, 76, Dbus)

#define dbus_move_error(___src, ___dest) \
	AROS_LC2(void, dbus_move_error, \
	AROS_LCA(DBusError *, (___src), A0), \
	AROS_LCA(DBusError *, (___dest), A1), \
	struct Library *, DBUS_BASE_NAME, 77, Dbus)

#define dbus_error_has_name(___error, ___name) \
	AROS_LC2(dbus_bool_t, dbus_error_has_name, \
	AROS_LCA(const DBusError *, (___error), A0), \
	AROS_LCA(const char *, (___name), A1), \
	struct Library *, DBUS_BASE_NAME, 78, Dbus)

#define dbus_error_is_set(___error) \
	AROS_LC1(dbus_bool_t, dbus_error_is_set, \
	AROS_LCA(const DBusError *, (___error), A0), \
	struct Library *, DBUS_BASE_NAME, 79, Dbus)

#define dbus_set_error(___error, ___name, ___format, ...) \
	({void (*_func) (DBusError *, const char *, const char *, ...) = \
	    (void (*) (DBusError *, const char *, const char *, ...))\
	    __AROS_GETVECADDR(DBUS_BASE_NAME, 80);\
	  (*_func)((___error), (___name), (___format), __VA_ARGS__); })

#define dbus_malloc(___bytes) \
	AROS_LC1(void *, dbus_malloc, \
	AROS_LCA(size_t, (___bytes), D0), \
	struct Library *, DBUS_BASE_NAME, 81, Dbus)

#define dbus_malloc0(___bytes) \
	AROS_LC1(void *, dbus_malloc0, \
	AROS_LCA(size_t, (___bytes), D0), \
	struct Library *, DBUS_BASE_NAME, 82, Dbus)

#define dbus_realloc(___memory, ___bytes) \
	AROS_LC2(void *, dbus_realloc, \
	AROS_LCA(void *, (___memory), A0), \
	AROS_LCA(size_t, (___bytes), D0), \
	struct Library *, DBUS_BASE_NAME, 83, Dbus)

#define dbus_free(___memory) \
	AROS_LC1(void, dbus_free, \
	AROS_LCA(void *, (___memory), A0), \
	struct Library *, DBUS_BASE_NAME, 84, Dbus)

#define dbus_free_string_array(___str_array) \
	AROS_LC1(void, dbus_free_string_array, \
	AROS_LCA(char **, (___str_array), A0), \
	struct Library *, DBUS_BASE_NAME, 85, Dbus)

#define dbus_shutdown() \
	AROS_LC0(void, dbus_shutdown, \
	struct Library *, DBUS_BASE_NAME, 86, Dbus)

#define dbus_message_set_reply_serial(___message, ___reply_serial) \
	AROS_LC2(dbus_bool_t, dbus_message_set_reply_serial, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(dbus_uint32_t, (___reply_serial), D0), \
	struct Library *, DBUS_BASE_NAME, 87, Dbus)

#define dbus_message_get_serial(___message) \
	AROS_LC1(dbus_uint32_t, dbus_message_get_serial, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	struct Library *, DBUS_BASE_NAME, 88, Dbus)

#define dbus_message_get_reply_serial(___message) \
	AROS_LC1(dbus_uint32_t, dbus_message_get_reply_serial, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	struct Library *, DBUS_BASE_NAME, 89, Dbus)

#define dbus_message_new(___message_type) \
	AROS_LC1(DBusMessage *, dbus_message_new, \
	AROS_LCA(int, (___message_type), D0), \
	struct Library *, DBUS_BASE_NAME, 90, Dbus)

#define dbus_message_new_method_call(___service, ___path, ___interface, ___method) \
	AROS_LC4(DBusMessage *, dbus_message_new_method_call, \
	AROS_LCA(const char *, (___service), A0), \
	AROS_LCA(const char *, (___path), A1), \
	AROS_LCA(const char *, (___interface), A2), \
	AROS_LCA(const char *, (___method), A3), \
	struct Library *, DBUS_BASE_NAME, 91, Dbus)

#define dbus_message_new_method_return(___method_call) \
	AROS_LC1(DBusMessage *, dbus_message_new_method_return, \
	AROS_LCA(DBusMessage *, (___method_call), A0), \
	struct Library *, DBUS_BASE_NAME, 92, Dbus)

#define dbus_message_new_signal(___path, ___interface, ___name) \
	AROS_LC3(DBusMessage *, dbus_message_new_signal, \
	AROS_LCA(const char *, (___path), A0), \
	AROS_LCA(const char *, (___interface), A1), \
	AROS_LCA(const char *, (___name), A2), \
	struct Library *, DBUS_BASE_NAME, 93, Dbus)

#define dbus_message_new_error(___reply_to, ___error_name, ___error_message) \
	AROS_LC3(DBusMessage *, dbus_message_new_error, \
	AROS_LCA(DBusMessage *, (___reply_to), A0), \
	AROS_LCA(const char *, (___error_name), A1), \
	AROS_LCA(const char *, (___error_message), A2), \
	struct Library *, DBUS_BASE_NAME, 94, Dbus)

#define dbus_message_new_error_printf(___reply_to, ___error_name, ___error_format, ...) \
	({DBusMessage * (*_func) (DBusMessage *, const char *, const char *, ...) = \
	    (DBusMessage * (*) (DBusMessage *, const char *, const char *, ...))\
	    __AROS_GETVECADDR(DBUS_BASE_NAME, 95);\
	  (*_func)((___reply_to), (___error_name), (___error_format), __VA_ARGS__); })

#define dbus_message_copy(___message) \
	AROS_LC1(DBusMessage *, dbus_message_copy, \
	AROS_LCA(const DBusMessage *, (___message), A0), \
	struct Library *, DBUS_BASE_NAME, 96, Dbus)

#define dbus_message_ref(___message) \
	AROS_LC1(DBusMessage *, dbus_message_ref, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	struct Library *, DBUS_BASE_NAME, 97, Dbus)

#define dbus_message_unref(___message) \
	AROS_LC1(void, dbus_message_unref, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	struct Library *, DBUS_BASE_NAME, 98, Dbus)

#define dbus_message_get_type(___message) \
	AROS_LC1(int, dbus_message_get_type, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	struct Library *, DBUS_BASE_NAME, 99, Dbus)

#define dbus_message_set_path(___message, ___object_path) \
	AROS_LC2(dbus_bool_t, dbus_message_set_path, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(const char *, (___object_path), A1), \
	struct Library *, DBUS_BASE_NAME, 100, Dbus)

#define dbus_message_get_path(___message) \
	AROS_LC1(const char *, dbus_message_get_path, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	struct Library *, DBUS_BASE_NAME, 101, Dbus)

#define dbus_message_get_path_decomposed(___message, ___path) \
	AROS_LC2(dbus_bool_t, dbus_message_get_path_decomposed, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(char ***, (___path), A1), \
	struct Library *, DBUS_BASE_NAME, 102, Dbus)

#define dbus_message_set_interface(___message, ___interface) \
	AROS_LC2(dbus_bool_t, dbus_message_set_interface, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(const char *, (___interface), A1), \
	struct Library *, DBUS_BASE_NAME, 103, Dbus)

#define dbus_message_get_interface(___message) \
	AROS_LC1(const char *, dbus_message_get_interface, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	struct Library *, DBUS_BASE_NAME, 104, Dbus)

#define dbus_message_set_member(___message, ___member) \
	AROS_LC2(dbus_bool_t, dbus_message_set_member, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(const char *, (___member), A1), \
	struct Library *, DBUS_BASE_NAME, 105, Dbus)

#define dbus_message_get_member(___message) \
	AROS_LC1(const char *, dbus_message_get_member, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	struct Library *, DBUS_BASE_NAME, 106, Dbus)

#define dbus_message_set_error_name(___message, ___error_name) \
	AROS_LC2(dbus_bool_t, dbus_message_set_error_name, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(const char *, (___error_name), A1), \
	struct Library *, DBUS_BASE_NAME, 107, Dbus)

#define dbus_message_get_error_name(___message) \
	AROS_LC1(const char *, dbus_message_get_error_name, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	struct Library *, DBUS_BASE_NAME, 108, Dbus)

#define dbus_message_set_destination(___message, ___destination) \
	AROS_LC2(dbus_bool_t, dbus_message_set_destination, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(const char *, (___destination), A1), \
	struct Library *, DBUS_BASE_NAME, 109, Dbus)

#define dbus_message_get_destination(___message) \
	AROS_LC1(const char *, dbus_message_get_destination, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	struct Library *, DBUS_BASE_NAME, 110, Dbus)

#define dbus_message_append_args(___message, ___first_arg_type, ...) \
	({dbus_bool_t (*_func) (DBusMessage *, int, ...) = \
	    (dbus_bool_t (*) (DBusMessage *, int, ...))\
	    __AROS_GETVECADDR(DBUS_BASE_NAME, 111);\
	  (*_func)((___message), (___first_arg_type), __VA_ARGS__); })

#define dbus_message_get_args(___message, ___error, ___first_arg_type, ...) \
	({dbus_bool_t (*_func) (DBusMessage *, DBusError *, int, ...) = \
	    (dbus_bool_t (*) (DBusMessage *, DBusError *, int, ...))\
	    __AROS_GETVECADDR(DBUS_BASE_NAME, 112);\
	  (*_func)((___message), (___error), (___first_arg_type), __VA_ARGS__); })

#define dbus_message_get_args_valist(___message, ___error, ___first_arg_type, ___var_args) \
	AROS_LC4(dbus_bool_t, dbus_message_get_args_valist, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(DBusError *, (___error), A1), \
	AROS_LCA(int, (___first_arg_type), D0), \
	AROS_LCA(va_list, (___var_args), D1), \
	struct Library *, DBUS_BASE_NAME, 113, Dbus)

#define dbus_message_iter_get_args(___iter, ___error, ___first_arg_type, ...) \
	({dbus_bool_t (*_func) (DBusMessageIter *, DBusError *, int, ...) = \
	    (dbus_bool_t (*) (DBusMessageIter *, DBusError *, int, ...))\
	    __AROS_GETVECADDR(DBUS_BASE_NAME, 114);\
	  (*_func)((___iter), (___error), (___first_arg_type), __VA_ARGS__); })

#define dbus_message_iter_init(___message, ___iter) \
	AROS_LC2(dbus_bool_t, dbus_message_iter_init, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(DBusMessageIter *, (___iter), A1), \
	struct Library *, DBUS_BASE_NAME, 115, Dbus)

#define dbus_message_iter_has_next(___iter) \
	AROS_LC1(dbus_bool_t, dbus_message_iter_has_next, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	struct Library *, DBUS_BASE_NAME, 116, Dbus)

#define dbus_message_iter_next(___iter) \
	AROS_LC1(dbus_bool_t, dbus_message_iter_next, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	struct Library *, DBUS_BASE_NAME, 117, Dbus)

#define dbus_message_iter_get_arg_type(___iter) \
	AROS_LC1(int, dbus_message_iter_get_arg_type, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	struct Library *, DBUS_BASE_NAME, 118, Dbus)

#define dbus_message_iter_get_array_type(___iter) \
	AROS_LC1(int, dbus_message_iter_get_array_type, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	struct Library *, DBUS_BASE_NAME, 119, Dbus)

#define dbus_message_iter_get_string(___iter) \
	AROS_LC1(char *, dbus_message_iter_get_string, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	struct Library *, DBUS_BASE_NAME, 120, Dbus)

#define dbus_message_iter_get_object_path(___iter) \
	AROS_LC1(char *, dbus_message_iter_get_object_path, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	struct Library *, DBUS_BASE_NAME, 121, Dbus)

#define dbus_message_iter_get_custom(___iter, ___name, ___value, ___len) \
	AROS_LC4(dbus_bool_t, dbus_message_iter_get_custom, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(char **, (___name), A1), \
	AROS_LCA(unsigned char **, (___value), A2), \
	AROS_LCA(int *, (___len), A3), \
	struct Library *, DBUS_BASE_NAME, 122, Dbus)

#define dbus_message_iter_get_args_valist(___iter, ___error, ___first_arg_type, ___var_args) \
	AROS_LC4(dbus_bool_t, dbus_message_iter_get_args_valist, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(DBusError *, (___error), A1), \
	AROS_LCA(int, (___first_arg_type), D0), \
	AROS_LCA(va_list, (___var_args), D1), \
	struct Library *, DBUS_BASE_NAME, 123, Dbus)

#define dbus_message_iter_get_byte(___iter) \
	AROS_LC1(unsigned char, dbus_message_iter_get_byte, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	struct Library *, DBUS_BASE_NAME, 124, Dbus)

#define dbus_message_iter_get_boolean(___iter) \
	AROS_LC1(dbus_bool_t, dbus_message_iter_get_boolean, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	struct Library *, DBUS_BASE_NAME, 125, Dbus)

#define dbus_message_iter_get_int32(___iter) \
	AROS_LC1(dbus_int32_t, dbus_message_iter_get_int32, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	struct Library *, DBUS_BASE_NAME, 126, Dbus)

#define dbus_message_iter_get_uint32(___iter) \
	AROS_LC1(dbus_uint32_t, dbus_message_iter_get_uint32, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	struct Library *, DBUS_BASE_NAME, 127, Dbus)

#define dbus_message_iter_get_int64(___iter) \
	AROS_LC1(dbus_int64_t, dbus_message_iter_get_int64, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	struct Library *, DBUS_BASE_NAME, 128, Dbus)

#define dbus_message_iter_get_uint64(___iter) \
	AROS_LC1(dbus_uint64_t, dbus_message_iter_get_uint64, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	struct Library *, DBUS_BASE_NAME, 129, Dbus)

#define dbus_message_iter_get_double(___iter) \
	AROS_LC1(double, dbus_message_iter_get_double, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	struct Library *, DBUS_BASE_NAME, 130, Dbus)

#define dbus_message_iter_init_array_iterator(___iter, ___array_iter, ___array_type) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_init_array_iterator, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(DBusMessageIter *, (___array_iter), A1), \
	AROS_LCA(int *, (___array_type), A2), \
	struct Library *, DBUS_BASE_NAME, 131, Dbus)

#define dbus_message_iter_init_dict_iterator(___iter, ___dict_iter) \
	AROS_LC2(dbus_bool_t, dbus_message_iter_init_dict_iterator, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(DBusMessageIter *, (___dict_iter), A1), \
	struct Library *, DBUS_BASE_NAME, 132, Dbus)

#define dbus_message_iter_get_byte_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_get_byte_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(unsigned char **, (___value), A1), \
	AROS_LCA(int *, (___len), A2), \
	struct Library *, DBUS_BASE_NAME, 133, Dbus)

#define dbus_message_iter_get_boolean_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_get_boolean_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(unsigned char **, (___value), A1), \
	AROS_LCA(int *, (___len), A2), \
	struct Library *, DBUS_BASE_NAME, 134, Dbus)

#define dbus_message_iter_get_int32_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_get_int32_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(dbus_int32_t **, (___value), A1), \
	AROS_LCA(int *, (___len), A2), \
	struct Library *, DBUS_BASE_NAME, 135, Dbus)

#define dbus_message_iter_get_uint32_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_get_uint32_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(dbus_uint32_t **, (___value), A1), \
	AROS_LCA(int *, (___len), A2), \
	struct Library *, DBUS_BASE_NAME, 136, Dbus)

#define dbus_message_iter_get_int64_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_get_int64_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(dbus_int64_t **, (___value), A1), \
	AROS_LCA(int *, (___len), A2), \
	struct Library *, DBUS_BASE_NAME, 137, Dbus)

#define dbus_message_iter_get_uint64_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_get_uint64_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(dbus_uint64_t **, (___value), A1), \
	AROS_LCA(int *, (___len), A2), \
	struct Library *, DBUS_BASE_NAME, 138, Dbus)

#define dbus_message_iter_get_double_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_get_double_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(double **, (___value), A1), \
	AROS_LCA(int *, (___len), A2), \
	struct Library *, DBUS_BASE_NAME, 139, Dbus)

#define dbus_message_iter_get_string_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_get_string_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(char ***, (___value), A1), \
	AROS_LCA(int *, (___len), A2), \
	struct Library *, DBUS_BASE_NAME, 140, Dbus)

#define dbus_message_iter_get_object_path_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_get_object_path_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(char ***, (___value), A1), \
	AROS_LCA(int *, (___len), A2), \
	struct Library *, DBUS_BASE_NAME, 141, Dbus)

#define dbus_message_iter_get_dict_key(___iter) \
	AROS_LC1(char *, dbus_message_iter_get_dict_key, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	struct Library *, DBUS_BASE_NAME, 142, Dbus)

#define dbus_message_append_iter_init(___message, ___iter) \
	AROS_LC2(void, dbus_message_append_iter_init, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(DBusMessageIter *, (___iter), A1), \
	struct Library *, DBUS_BASE_NAME, 143, Dbus)

#define dbus_message_iter_append_nil(___iter) \
	AROS_LC1(dbus_bool_t, dbus_message_iter_append_nil, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	struct Library *, DBUS_BASE_NAME, 144, Dbus)

#define dbus_message_iter_append_boolean(___iter, ___value) \
	AROS_LC2(dbus_bool_t, dbus_message_iter_append_boolean, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(dbus_bool_t, (___value), D0), \
	struct Library *, DBUS_BASE_NAME, 145, Dbus)

#define dbus_message_iter_append_byte(___iter, ___value) \
	AROS_LC2(dbus_bool_t, dbus_message_iter_append_byte, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(unsigned char, (___value), D0), \
	struct Library *, DBUS_BASE_NAME, 146, Dbus)

#define dbus_message_iter_append_int32(___iter, ___value) \
	AROS_LC2(dbus_bool_t, dbus_message_iter_append_int32, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(dbus_int32_t, (___value), D0), \
	struct Library *, DBUS_BASE_NAME, 147, Dbus)

#define dbus_message_iter_append_uint32(___iter, ___value) \
	AROS_LC2(dbus_bool_t, dbus_message_iter_append_uint32, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(dbus_uint32_t, (___value), D0), \
	struct Library *, DBUS_BASE_NAME, 148, Dbus)

#define dbus_message_iter_append_int64(___iter, ___value) \
	AROS_LC2(dbus_bool_t, dbus_message_iter_append_int64, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(dbus_int64_t, (___value), D0), \
	struct Library *, DBUS_BASE_NAME, 149, Dbus)

#define dbus_message_iter_append_uint64(___iter, ___value) \
	AROS_LC2(dbus_bool_t, dbus_message_iter_append_uint64, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(dbus_uint64_t, (___value), D0), \
	struct Library *, DBUS_BASE_NAME, 150, Dbus)

#define dbus_message_iter_append_double(___iter, ___value) \
	AROS_LC2(dbus_bool_t, dbus_message_iter_append_double, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(double, (___value), D0), \
	struct Library *, DBUS_BASE_NAME, 151, Dbus)

#define dbus_message_iter_append_string(___iter, ___value) \
	AROS_LC2(dbus_bool_t, dbus_message_iter_append_string, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(const char *, (___value), A1), \
	struct Library *, DBUS_BASE_NAME, 152, Dbus)

#define dbus_message_iter_append_object_path(___iter, ___value) \
	AROS_LC2(dbus_bool_t, dbus_message_iter_append_object_path, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(const char *, (___value), A1), \
	struct Library *, DBUS_BASE_NAME, 153, Dbus)

#define dbus_message_iter_append_custom(___iter, ___name, ___data, ___len) \
	AROS_LC4(dbus_bool_t, dbus_message_iter_append_custom, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(const char *, (___name), A1), \
	AROS_LCA(const unsigned char *, (___data), A2), \
	AROS_LCA(int, (___len), D0), \
	struct Library *, DBUS_BASE_NAME, 154, Dbus)

#define dbus_message_iter_append_dict_key(___iter, ___value) \
	AROS_LC2(dbus_bool_t, dbus_message_iter_append_dict_key, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(const char *, (___value), A1), \
	struct Library *, DBUS_BASE_NAME, 155, Dbus)

#define dbus_message_iter_append_array(___iter, ___array_iter, ___element_type) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_append_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(DBusMessageIter *, (___array_iter), A1), \
	AROS_LCA(int, (___element_type), D0), \
	struct Library *, DBUS_BASE_NAME, 156, Dbus)

#define dbus_message_iter_append_dict(___iter, ___dict_iter) \
	AROS_LC2(dbus_bool_t, dbus_message_iter_append_dict, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(DBusMessageIter *, (___dict_iter), A1), \
	struct Library *, DBUS_BASE_NAME, 157, Dbus)

#define dbus_message_append_args_valist(___message, ___first_arg_type, ___var_args) \
	AROS_LC3(dbus_bool_t, dbus_message_append_args_valist, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(int, (___first_arg_type), D0), \
	AROS_LCA(va_list, (___var_args), D1), \
	struct Library *, DBUS_BASE_NAME, 158, Dbus)

#define dbus_message_iter_append_boolean_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_append_boolean_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(const unsigned char *, (___value), A1), \
	AROS_LCA(int, (___len), D0), \
	struct Library *, DBUS_BASE_NAME, 159, Dbus)

#define dbus_message_iter_append_int32_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_append_int32_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(const dbus_int32_t *, (___value), A1), \
	AROS_LCA(int, (___len), D0), \
	struct Library *, DBUS_BASE_NAME, 160, Dbus)

#define dbus_message_iter_append_uint32_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_append_uint32_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(const dbus_uint32_t *, (___value), A1), \
	AROS_LCA(int, (___len), D0), \
	struct Library *, DBUS_BASE_NAME, 161, Dbus)

#define dbus_message_iter_append_int64_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_append_int64_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(const dbus_int64_t *, (___value), A1), \
	AROS_LCA(int, (___len), D0), \
	struct Library *, DBUS_BASE_NAME, 162, Dbus)

#define dbus_message_iter_append_uint64_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_append_uint64_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(const dbus_uint64_t *, (___value), A1), \
	AROS_LCA(int, (___len), D0), \
	struct Library *, DBUS_BASE_NAME, 163, Dbus)

#define dbus_message_iter_append_double_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_append_double_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(const double *, (___value), A1), \
	AROS_LCA(int, (___len), D0), \
	struct Library *, DBUS_BASE_NAME, 164, Dbus)

#define dbus_message_iter_append_byte_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_append_byte_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(const unsigned char *, (___value), A1), \
	AROS_LCA(int, (___len), D0), \
	struct Library *, DBUS_BASE_NAME, 165, Dbus)

#define dbus_message_iter_append_string_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_append_string_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(const char **, (___value), A1), \
	AROS_LCA(int, (___len), D0), \
	struct Library *, DBUS_BASE_NAME, 166, Dbus)

#define dbus_message_iter_append_object_path_array(___iter, ___value, ___len) \
	AROS_LC3(dbus_bool_t, dbus_message_iter_append_object_path_array, \
	AROS_LCA(DBusMessageIter *, (___iter), A0), \
	AROS_LCA(const char **, (___value), A1), \
	AROS_LCA(int, (___len), D0), \
	struct Library *, DBUS_BASE_NAME, 167, Dbus)

#define dbus_message_set_sender(___message, ___sender) \
	AROS_LC2(dbus_bool_t, dbus_message_set_sender, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(const char *, (___sender), A1), \
	struct Library *, DBUS_BASE_NAME, 168, Dbus)

#define dbus_message_set_no_reply(___message, ___no_reply) \
	AROS_LC2(void, dbus_message_set_no_reply, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(dbus_bool_t, (___no_reply), D0), \
	struct Library *, DBUS_BASE_NAME, 169, Dbus)

#define dbus_message_get_no_reply(___message) \
	AROS_LC1(dbus_bool_t, dbus_message_get_no_reply, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	struct Library *, DBUS_BASE_NAME, 170, Dbus)

#define dbus_message_set_auto_activation(___message, ___auto_activation) \
	AROS_LC2(void, dbus_message_set_auto_activation, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(dbus_bool_t, (___auto_activation), D0), \
	struct Library *, DBUS_BASE_NAME, 171, Dbus)

#define dbus_message_get_auto_activation(___message) \
	AROS_LC1(dbus_bool_t, dbus_message_get_auto_activation, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	struct Library *, DBUS_BASE_NAME, 172, Dbus)

#define dbus_message_get_sender(___message) \
	AROS_LC1(const char *, dbus_message_get_sender, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	struct Library *, DBUS_BASE_NAME, 173, Dbus)

#define dbus_message_get_signature(___message) \
	AROS_LC1(const char *, dbus_message_get_signature, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	struct Library *, DBUS_BASE_NAME, 174, Dbus)

#define dbus_message_is_method_call(___message, ___interface, ___method) \
	AROS_LC3(dbus_bool_t, dbus_message_is_method_call, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(const char *, (___interface), A1), \
	AROS_LCA(const char *, (___method), A2), \
	struct Library *, DBUS_BASE_NAME, 175, Dbus)

#define dbus_message_is_signal(___message, ___interface, ___signal_name) \
	AROS_LC3(dbus_bool_t, dbus_message_is_signal, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(const char *, (___interface), A1), \
	AROS_LCA(const char *, (___signal_name), A2), \
	struct Library *, DBUS_BASE_NAME, 176, Dbus)

#define dbus_message_is_error(___message, ___error_name) \
	AROS_LC2(dbus_bool_t, dbus_message_is_error, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(const char *, (___error_name), A1), \
	struct Library *, DBUS_BASE_NAME, 177, Dbus)

#define dbus_message_has_destination(___message, ___service) \
	AROS_LC2(dbus_bool_t, dbus_message_has_destination, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(const char *, (___service), A1), \
	struct Library *, DBUS_BASE_NAME, 178, Dbus)

#define dbus_message_has_sender(___message, ___service) \
	AROS_LC2(dbus_bool_t, dbus_message_has_sender, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(const char *, (___service), A1), \
	struct Library *, DBUS_BASE_NAME, 179, Dbus)

#define dbus_message_has_signature(___message, ___signature) \
	AROS_LC2(dbus_bool_t, dbus_message_has_signature, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(const char *, (___signature), A1), \
	struct Library *, DBUS_BASE_NAME, 180, Dbus)

#define dbus_set_error_from_message(___error, ___message) \
	AROS_LC2(dbus_bool_t, dbus_set_error_from_message, \
	AROS_LCA(DBusError *, (___error), A0), \
	AROS_LCA(DBusMessage *, (___message), A1), \
	struct Library *, DBUS_BASE_NAME, 181, Dbus)

#define dbus_message_allocate_data_slot(___slot_p) \
	AROS_LC1(dbus_bool_t, dbus_message_allocate_data_slot, \
	AROS_LCA(dbus_int32_t *, (___slot_p), A0), \
	struct Library *, DBUS_BASE_NAME, 182, Dbus)

#define dbus_message_free_data_slot(___slot_p) \
	AROS_LC1(void, dbus_message_free_data_slot, \
	AROS_LCA(dbus_int32_t *, (___slot_p), A0), \
	struct Library *, DBUS_BASE_NAME, 183, Dbus)

#define dbus_message_set_data(___message, ___slot, ___data, ___free_data_func) \
	AROS_LC4(dbus_bool_t, dbus_message_set_data, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(dbus_int32_t, (___slot), D0), \
	AROS_LCA(void *, (___data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_data_func), D1), \
	struct Library *, DBUS_BASE_NAME, 184, Dbus)

#define dbus_message_get_data(___message, ___slot) \
	AROS_LC2(void *, dbus_message_get_data, \
	AROS_LCA(DBusMessage *, (___message), A0), \
	AROS_LCA(dbus_int32_t, (___slot), D0), \
	struct Library *, DBUS_BASE_NAME, 185, Dbus)

#define dbus_message_type_from_string(___type_str) \
	AROS_LC1(int, dbus_message_type_from_string, \
	AROS_LCA(const char *, (___type_str), A0), \
	struct Library *, DBUS_BASE_NAME, 186, Dbus)

#define dbus_pending_call_ref(___pending) \
	AROS_LC1(DBusPendingCall *, dbus_pending_call_ref, \
	AROS_LCA(DBusPendingCall *, (___pending), A0), \
	struct Library *, DBUS_BASE_NAME, 187, Dbus)

#define dbus_pending_call_unref(___pending) \
	AROS_LC1(void, dbus_pending_call_unref, \
	AROS_LCA(DBusPendingCall *, (___pending), A0), \
	struct Library *, DBUS_BASE_NAME, 188, Dbus)

#define dbus_pending_call_set_notify(___pending, ___function, ___user_data, ___free_user_data) \
	AROS_LC4(dbus_bool_t, dbus_pending_call_set_notify, \
	AROS_LCA(DBusPendingCall *, (___pending), A0), \
	AROS_LCA(DBusPendingCallNotifyFunction, (___function), D0), \
	AROS_LCA(void *, (___user_data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_user_data), D1), \
	struct Library *, DBUS_BASE_NAME, 189, Dbus)

#define dbus_pending_call_cancel(___pending) \
	AROS_LC1(void, dbus_pending_call_cancel, \
	AROS_LCA(DBusPendingCall *, (___pending), A0), \
	struct Library *, DBUS_BASE_NAME, 190, Dbus)

#define dbus_pending_call_get_completed(___pending) \
	AROS_LC1(dbus_bool_t, dbus_pending_call_get_completed, \
	AROS_LCA(DBusPendingCall *, (___pending), A0), \
	struct Library *, DBUS_BASE_NAME, 191, Dbus)

#define dbus_pending_call_get_reply(___pending) \
	AROS_LC1(DBusMessage *, dbus_pending_call_get_reply, \
	AROS_LCA(DBusPendingCall *, (___pending), A0), \
	struct Library *, DBUS_BASE_NAME, 192, Dbus)

#define dbus_pending_call_block(___pending) \
	AROS_LC1(void, dbus_pending_call_block, \
	AROS_LCA(DBusPendingCall *, (___pending), A0), \
	struct Library *, DBUS_BASE_NAME, 193, Dbus)

#define dbus_pending_call_allocate_data_slot(___slot_p) \
	AROS_LC1(dbus_bool_t, dbus_pending_call_allocate_data_slot, \
	AROS_LCA(dbus_int32_t *, (___slot_p), A0), \
	struct Library *, DBUS_BASE_NAME, 194, Dbus)

#define dbus_pending_call_free_data_slot(___slot_p) \
	AROS_LC1(void, dbus_pending_call_free_data_slot, \
	AROS_LCA(dbus_int32_t *, (___slot_p), A0), \
	struct Library *, DBUS_BASE_NAME, 195, Dbus)

#define dbus_pending_call_set_data(___pending, ___slot, ___data, ___free_data_func) \
	AROS_LC4(dbus_bool_t, dbus_pending_call_set_data, \
	AROS_LCA(DBusPendingCall *, (___pending), A0), \
	AROS_LCA(dbus_int32_t, (___slot), D0), \
	AROS_LCA(void *, (___data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_data_func), D1), \
	struct Library *, DBUS_BASE_NAME, 196, Dbus)

#define dbus_pending_call_get_data(___pending, ___slot) \
	AROS_LC2(void *, dbus_pending_call_get_data, \
	AROS_LCA(DBusPendingCall *, (___pending), A0), \
	AROS_LCA(dbus_int32_t, (___slot), D0), \
	struct Library *, DBUS_BASE_NAME, 197, Dbus)

#define dbus_server_listen(___address, ___error) \
	AROS_LC2(DBusServer *, dbus_server_listen, \
	AROS_LCA(const char *, (___address), A0), \
	AROS_LCA(DBusError *, (___error), A1), \
	struct Library *, DBUS_BASE_NAME, 198, Dbus)

#define dbus_server_ref(___server) \
	AROS_LC1(DBusServer *, dbus_server_ref, \
	AROS_LCA(DBusServer *, (___server), A0), \
	struct Library *, DBUS_BASE_NAME, 199, Dbus)

#define dbus_server_unref(___server) \
	AROS_LC1(void, dbus_server_unref, \
	AROS_LCA(DBusServer *, (___server), A0), \
	struct Library *, DBUS_BASE_NAME, 200, Dbus)

#define dbus_server_disconnect(___server) \
	AROS_LC1(void, dbus_server_disconnect, \
	AROS_LCA(DBusServer *, (___server), A0), \
	struct Library *, DBUS_BASE_NAME, 201, Dbus)

#define dbus_server_get_is_connected(___server) \
	AROS_LC1(dbus_bool_t, dbus_server_get_is_connected, \
	AROS_LCA(DBusServer *, (___server), A0), \
	struct Library *, DBUS_BASE_NAME, 202, Dbus)

#define dbus_server_get_address(___server) \
	AROS_LC1(char *, dbus_server_get_address, \
	AROS_LCA(DBusServer *, (___server), A0), \
	struct Library *, DBUS_BASE_NAME, 203, Dbus)

#define dbus_server_set_new_connection_function(___server, ___function, ___data, ___free_data_function) \
	AROS_LC4(void, dbus_server_set_new_connection_function, \
	AROS_LCA(DBusServer *, (___server), A0), \
	AROS_LCA(DBusNewConnectionFunction, (___function), D0), \
	AROS_LCA(void *, (___data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_data_function), D1), \
	struct Library *, DBUS_BASE_NAME, 204, Dbus)

#define dbus_server_set_watch_functions(___server, ___add_function, ___remove_function, ___toggled_function, ___data, ___free_data_function) \
	AROS_LC6(dbus_bool_t, dbus_server_set_watch_functions, \
	AROS_LCA(DBusServer *, (___server), A0), \
	AROS_LCA(DBusAddWatchFunction, (___add_function), D0), \
	AROS_LCA(DBusRemoveWatchFunction, (___remove_function), D1), \
	AROS_LCA(DBusWatchToggledFunction, (___toggled_function), D2), \
	AROS_LCA(void *, (___data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_data_function), D3), \
	struct Library *, DBUS_BASE_NAME, 205, Dbus)

#define dbus_server_set_timeout_functions(___server, ___add_function, ___remove_function, ___toggled_function, ___data, ___free_data_function) \
	AROS_LC6(dbus_bool_t, dbus_server_set_timeout_functions, \
	AROS_LCA(DBusServer *, (___server), A0), \
	AROS_LCA(DBusAddTimeoutFunction, (___add_function), D0), \
	AROS_LCA(DBusRemoveTimeoutFunction, (___remove_function), D1), \
	AROS_LCA(DBusTimeoutToggledFunction, (___toggled_function), D2), \
	AROS_LCA(void *, (___data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_data_function), D3), \
	struct Library *, DBUS_BASE_NAME, 206, Dbus)

#define dbus_server_set_auth_mechanisms(___server, ___mechanisms) \
	AROS_LC2(dbus_bool_t, dbus_server_set_auth_mechanisms, \
	AROS_LCA(DBusServer *, (___server), A0), \
	AROS_LCA(const char **, (___mechanisms), A1), \
	struct Library *, DBUS_BASE_NAME, 207, Dbus)

#define dbus_server_allocate_data_slot(___slot_p) \
	AROS_LC1(dbus_bool_t, dbus_server_allocate_data_slot, \
	AROS_LCA(dbus_int32_t *, (___slot_p), A0), \
	struct Library *, DBUS_BASE_NAME, 208, Dbus)

#define dbus_server_free_data_slot(___slot_p) \
	AROS_LC1(void, dbus_server_free_data_slot, \
	AROS_LCA(dbus_int32_t *, (___slot_p), A0), \
	struct Library *, DBUS_BASE_NAME, 209, Dbus)

#define dbus_server_set_data(___server, ___slot, ___data, ___free_data_func) \
	AROS_LC4(dbus_bool_t, dbus_server_set_data, \
	AROS_LCA(DBusServer *, (___server), A0), \
	AROS_LCA(int, (___slot), D0), \
	AROS_LCA(void *, (___data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_data_func), D1), \
	struct Library *, DBUS_BASE_NAME, 210, Dbus)

#define dbus_server_get_data(___server, ___slot) \
	AROS_LC2(void *, dbus_server_get_data, \
	AROS_LCA(DBusServer *, (___server), A0), \
	AROS_LCA(int, (___slot), D0), \
	struct Library *, DBUS_BASE_NAME, 211, Dbus)

#define dbus_internal_do_not_use_run_tests(___test_data_dir) \
	AROS_LC1(void, dbus_internal_do_not_use_run_tests, \
	AROS_LCA(const char *, (___test_data_dir), A0), \
	struct Library *, DBUS_BASE_NAME, 212, Dbus)

#define dbus_mutex_new() \
	AROS_LC0(DBusMutex *, dbus_mutex_new, \
	struct Library *, DBUS_BASE_NAME, 213, Dbus)

#define dbus_mutex_free(___mutex) \
	AROS_LC1(void, dbus_mutex_free, \
	AROS_LCA(DBusMutex *, (___mutex), A0), \
	struct Library *, DBUS_BASE_NAME, 214, Dbus)

#define dbus_mutex_lock(___mutex) \
	AROS_LC1(dbus_bool_t, dbus_mutex_lock, \
	AROS_LCA(DBusMutex *, (___mutex), A0), \
	struct Library *, DBUS_BASE_NAME, 215, Dbus)

#define dbus_mutex_unlock(___mutex) \
	AROS_LC1(dbus_bool_t, dbus_mutex_unlock, \
	AROS_LCA(DBusMutex *, (___mutex), A0), \
	struct Library *, DBUS_BASE_NAME, 216, Dbus)

#define dbus_condvar_new() \
	AROS_LC0(DBusCondVar *, dbus_condvar_new, \
	struct Library *, DBUS_BASE_NAME, 217, Dbus)

#define dbus_condvar_free(___cond) \
	AROS_LC1(void, dbus_condvar_free, \
	AROS_LCA(DBusCondVar *, (___cond), A0), \
	struct Library *, DBUS_BASE_NAME, 218, Dbus)

#define dbus_condvar_wait(___cond, ___mutex) \
	AROS_LC2(void, dbus_condvar_wait, \
	AROS_LCA(DBusCondVar *, (___cond), A0), \
	AROS_LCA(DBusMutex *, (___mutex), A1), \
	struct Library *, DBUS_BASE_NAME, 219, Dbus)

#define dbus_condvar_wait_timeout(___cond, ___mutex, ___timeout_milliseconds) \
	AROS_LC3(dbus_bool_t, dbus_condvar_wait_timeout, \
	AROS_LCA(DBusCondVar *, (___cond), A0), \
	AROS_LCA(DBusMutex *, (___mutex), A1), \
	AROS_LCA(int, (___timeout_milliseconds), D0), \
	struct Library *, DBUS_BASE_NAME, 220, Dbus)

#define dbus_condvar_wake_one(___cond) \
	AROS_LC1(void, dbus_condvar_wake_one, \
	AROS_LCA(DBusCondVar *, (___cond), A0), \
	struct Library *, DBUS_BASE_NAME, 221, Dbus)

#define dbus_condvar_wake_all(___cond) \
	AROS_LC1(void, dbus_condvar_wake_all, \
	AROS_LCA(DBusCondVar *, (___cond), A0), \
	struct Library *, DBUS_BASE_NAME, 222, Dbus)

#define dbus_threads_init(___functions) \
	AROS_LC1(dbus_bool_t, dbus_threads_init, \
	AROS_LCA(const DBusThreadFunctions *, (___functions), A0), \
	struct Library *, DBUS_BASE_NAME, 223, Dbus)

#define dbus_timeout_get_interval(___timeout) \
	AROS_LC1(int, dbus_timeout_get_interval, \
	AROS_LCA(DBusTimeout *, (___timeout), A0), \
	struct Library *, DBUS_BASE_NAME, 224, Dbus)

#define dbus_timeout_get_data(___timeout) \
	AROS_LC1(void *, dbus_timeout_get_data, \
	AROS_LCA(DBusTimeout *, (___timeout), A0), \
	struct Library *, DBUS_BASE_NAME, 225, Dbus)

#define dbus_timeout_set_data(___timeout, ___data, ___free_data_function) \
	AROS_LC3(void, dbus_timeout_set_data, \
	AROS_LCA(DBusTimeout *, (___timeout), A0), \
	AROS_LCA(void *, (___data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_data_function), D0), \
	struct Library *, DBUS_BASE_NAME, 226, Dbus)

#define dbus_timeout_handle(___timeout) \
	AROS_LC1(dbus_bool_t, dbus_timeout_handle, \
	AROS_LCA(DBusTimeout *, (___timeout), A0), \
	struct Library *, DBUS_BASE_NAME, 227, Dbus)

#define dbus_timeout_get_enabled(___timeout) \
	AROS_LC1(dbus_bool_t, dbus_timeout_get_enabled, \
	AROS_LCA(DBusTimeout *, (___timeout), A0), \
	struct Library *, DBUS_BASE_NAME, 228, Dbus)

#define dbus_watch_get_fd(___watch) \
	AROS_LC1(int, dbus_watch_get_fd, \
	AROS_LCA(DBusWatch *, (___watch), A0), \
	struct Library *, DBUS_BASE_NAME, 229, Dbus)

#define dbus_watch_get_flags(___watch) \
	AROS_LC1(unsigned int, dbus_watch_get_flags, \
	AROS_LCA(DBusWatch *, (___watch), A0), \
	struct Library *, DBUS_BASE_NAME, 230, Dbus)

#define dbus_watch_get_data(___watch) \
	AROS_LC1(void *, dbus_watch_get_data, \
	AROS_LCA(DBusWatch *, (___watch), A0), \
	struct Library *, DBUS_BASE_NAME, 231, Dbus)

#define dbus_watch_set_data(___watch, ___data, ___free_data_function) \
	AROS_LC3(void, dbus_watch_set_data, \
	AROS_LCA(DBusWatch *, (___watch), A0), \
	AROS_LCA(void *, (___data), A1), \
	AROS_LCA(DBusFreeFunction, (___free_data_function), D0), \
	struct Library *, DBUS_BASE_NAME, 232, Dbus)

#define dbus_watch_get_enabled(___watch) \
	AROS_LC1(dbus_bool_t, dbus_watch_get_enabled, \
	AROS_LCA(DBusWatch *, (___watch), A0), \
	struct Library *, DBUS_BASE_NAME, 233, Dbus)

#define dbus_watch_handle(___watch, ___flags) \
	AROS_LC2(dbus_bool_t, dbus_watch_handle, \
	AROS_LCA(DBusWatch *, (___watch), A0), \
	AROS_LCA(unsigned int, (___flags), D0), \
	struct Library *, DBUS_BASE_NAME, 234, Dbus)

#endif /* !_INLINE_DBUS_H */
