/* Automatically generated header! Do not edit! */

#ifndef _GATEPROTO_DBUS_H
#define _GATEPROTO_DBUS_H

#include <dbus/dbus.h>

#define _sfdc_strarg(a) _sfdc_strarg2(a)
#define _sfdc_strarg2(a) #a

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <aros/libcall.h>

AROS_LD1I(void, __dbus_address_entries_free,
	AROS_LDA(DBusAddressEntry **, ___entries, A0),
	struct Library *, _base, 16, Dbus);
#define __dbus_address_entries_free AROS_SLIB_ENTRY(__dbus_address_entries_free,Dbus)

AROS_LD1I(const char *, __dbus_address_entry_get_method,
	AROS_LDA(DBusAddressEntry *, ___entry, A0),
	struct Library *, _base, 17, Dbus);
#define __dbus_address_entry_get_method AROS_SLIB_ENTRY(__dbus_address_entry_get_method,Dbus)

AROS_LD2I(const char *, __dbus_address_entry_get_value,
	AROS_LDA(DBusAddressEntry *, ___entry, A0),
	AROS_LDA(const char *, ___key, A1),
	struct Library *, _base, 18, Dbus);
#define __dbus_address_entry_get_value AROS_SLIB_ENTRY(__dbus_address_entry_get_value,Dbus)

AROS_LD4I(dbus_bool_t, __dbus_parse_address,
	AROS_LDA(const char *, ___address, A0),
	AROS_LDA(DBusAddressEntry ***, ___entry, A1),
	AROS_LDA(int *, ___array_len, A2),
	AROS_LDA(DBusError *, ___error, A3),
	struct Library *, _base, 19, Dbus);
#define __dbus_parse_address AROS_SLIB_ENTRY(__dbus_parse_address,Dbus)

AROS_LD2I(DBusConnection *, __dbus_bus_get,
	AROS_LDA(DBusBusType, ___type, D0),
	AROS_LDA(DBusError *, ___error, A0),
	struct Library *, _base, 20, Dbus);
#define __dbus_bus_get AROS_SLIB_ENTRY(__dbus_bus_get,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_bus_register,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(DBusError *, ___error, A1),
	struct Library *, _base, 21, Dbus);
#define __dbus_bus_register AROS_SLIB_ENTRY(__dbus_bus_register,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_bus_set_base_service,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(const char *, ___base_service, A1),
	struct Library *, _base, 22, Dbus);
#define __dbus_bus_set_base_service AROS_SLIB_ENTRY(__dbus_bus_set_base_service,Dbus)

AROS_LD1I(const char *, __dbus_bus_get_base_service,
	AROS_LDA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 23, Dbus);
#define __dbus_bus_get_base_service AROS_SLIB_ENTRY(__dbus_bus_get_base_service,Dbus)

AROS_LD3I(long unsigned int, __dbus_bus_get_unix_user,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(const char *, ___service, A1),
	AROS_LDA(DBusError *, ___error, A2),
	struct Library *, _base, 24, Dbus);
#define __dbus_bus_get_unix_user AROS_SLIB_ENTRY(__dbus_bus_get_unix_user,Dbus)

AROS_LD4I(int, __dbus_bus_acquire_service,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(const char *, ___service_name, A1),
	AROS_LDA(unsigned int, ___flags, D0),
	AROS_LDA(DBusError *, ___error, A2),
	struct Library *, _base, 25, Dbus);
#define __dbus_bus_acquire_service AROS_SLIB_ENTRY(__dbus_bus_acquire_service,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_bus_service_exists,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(const char *, ___service_name, A1),
	AROS_LDA(DBusError *, ___error, A2),
	struct Library *, _base, 26, Dbus);
#define __dbus_bus_service_exists AROS_SLIB_ENTRY(__dbus_bus_service_exists,Dbus)

AROS_LD5I(dbus_bool_t, __dbus_bus_activate_service,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(const char *, ___service_name, A1),
	AROS_LDA(dbus_uint32_t, ___flags, D0),
	AROS_LDA(dbus_uint32_t *, ___result, A2),
	AROS_LDA(DBusError *, ___error, A3),
	struct Library *, _base, 27, Dbus);
#define __dbus_bus_activate_service AROS_SLIB_ENTRY(__dbus_bus_activate_service,Dbus)

AROS_LD3I(void, __dbus_bus_add_match,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(const char *, ___rule, A1),
	AROS_LDA(DBusError *, ___error, A2),
	struct Library *, _base, 28, Dbus);
#define __dbus_bus_add_match AROS_SLIB_ENTRY(__dbus_bus_add_match,Dbus)

AROS_LD3I(void, __dbus_bus_remove_match,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(const char *, ___rule, A1),
	AROS_LDA(DBusError *, ___error, A2),
	struct Library *, _base, 29, Dbus);
#define __dbus_bus_remove_match AROS_SLIB_ENTRY(__dbus_bus_remove_match,Dbus)

AROS_LD2I(DBusConnection *, __dbus_connection_open,
	AROS_LDA(const char *, ___address, A0),
	AROS_LDA(DBusError *, ___error, A1),
	struct Library *, _base, 30, Dbus);
#define __dbus_connection_open AROS_SLIB_ENTRY(__dbus_connection_open,Dbus)

AROS_LD1I(DBusConnection *, __dbus_connection_ref,
	AROS_LDA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 31, Dbus);
#define __dbus_connection_ref AROS_SLIB_ENTRY(__dbus_connection_ref,Dbus)

AROS_LD1I(void, __dbus_connection_unref,
	AROS_LDA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 32, Dbus);
#define __dbus_connection_unref AROS_SLIB_ENTRY(__dbus_connection_unref,Dbus)

AROS_LD1I(void, __dbus_connection_disconnect,
	AROS_LDA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 33, Dbus);
#define __dbus_connection_disconnect AROS_SLIB_ENTRY(__dbus_connection_disconnect,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_connection_get_is_connected,
	AROS_LDA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 34, Dbus);
#define __dbus_connection_get_is_connected AROS_SLIB_ENTRY(__dbus_connection_get_is_connected,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_connection_get_is_authenticated,
	AROS_LDA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 35, Dbus);
#define __dbus_connection_get_is_authenticated AROS_SLIB_ENTRY(__dbus_connection_get_is_authenticated,Dbus)

AROS_LD2I(void, __dbus_connection_set_exit_on_disconnect,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(dbus_bool_t, ___exit_on_disconnect, D0),
	struct Library *, _base, 36, Dbus);
#define __dbus_connection_set_exit_on_disconnect AROS_SLIB_ENTRY(__dbus_connection_set_exit_on_disconnect,Dbus)

AROS_LD1I(DBusPreallocatedSend *, __dbus_connection_preallocate_send,
	AROS_LDA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 37, Dbus);
#define __dbus_connection_preallocate_send AROS_SLIB_ENTRY(__dbus_connection_preallocate_send,Dbus)

AROS_LD2I(void, __dbus_connection_free_preallocated_send,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(DBusPreallocatedSend *, ___preallocated, A1),
	struct Library *, _base, 38, Dbus);
#define __dbus_connection_free_preallocated_send AROS_SLIB_ENTRY(__dbus_connection_free_preallocated_send,Dbus)

AROS_LD4I(void, __dbus_connection_send_preallocated,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(DBusPreallocatedSend *, ___preallocated, A1),
	AROS_LDA(DBusMessage *, ___message, A2),
	AROS_LDA(dbus_uint32_t *, ___client_serial, A3),
	struct Library *, _base, 39, Dbus);
#define __dbus_connection_send_preallocated AROS_SLIB_ENTRY(__dbus_connection_send_preallocated,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_connection_send,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(DBusMessage *, ___message, A1),
	AROS_LDA(dbus_uint32_t *, ___client_serial, A2),
	struct Library *, _base, 40, Dbus);
#define __dbus_connection_send AROS_SLIB_ENTRY(__dbus_connection_send,Dbus)

AROS_LD4I(dbus_bool_t, __dbus_connection_send_with_reply,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(DBusMessage *, ___message, A1),
	AROS_LDA(DBusPendingCall **, ___pending_return, A2),
	AROS_LDA(int, ___timeout_milliseconds, D0),
	struct Library *, _base, 41, Dbus);
#define __dbus_connection_send_with_reply AROS_SLIB_ENTRY(__dbus_connection_send_with_reply,Dbus)

AROS_LD4I(DBusMessage *, __dbus_connection_send_with_reply_and_block,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(DBusMessage *, ___message, A1),
	AROS_LDA(int, ___timeout_milliseconds, D0),
	AROS_LDA(DBusError *, ___error, A2),
	struct Library *, _base, 42, Dbus);
#define __dbus_connection_send_with_reply_and_block AROS_SLIB_ENTRY(__dbus_connection_send_with_reply_and_block,Dbus)

AROS_LD1I(void, __dbus_connection_flush,
	AROS_LDA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 43, Dbus);
#define __dbus_connection_flush AROS_SLIB_ENTRY(__dbus_connection_flush,Dbus)

AROS_LD1I(DBusMessage *, __dbus_connection_borrow_message,
	AROS_LDA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 44, Dbus);
#define __dbus_connection_borrow_message AROS_SLIB_ENTRY(__dbus_connection_borrow_message,Dbus)

AROS_LD2I(void, __dbus_connection_return_message,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(DBusMessage *, ___message, A1),
	struct Library *, _base, 45, Dbus);
#define __dbus_connection_return_message AROS_SLIB_ENTRY(__dbus_connection_return_message,Dbus)

AROS_LD2I(void, __dbus_connection_steal_borrowed_message,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(DBusMessage *, ___message, A1),
	struct Library *, _base, 46, Dbus);
#define __dbus_connection_steal_borrowed_message AROS_SLIB_ENTRY(__dbus_connection_steal_borrowed_message,Dbus)

AROS_LD1I(DBusMessage *, __dbus_connection_pop_message,
	AROS_LDA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 47, Dbus);
#define __dbus_connection_pop_message AROS_SLIB_ENTRY(__dbus_connection_pop_message,Dbus)

AROS_LD1I(DBusDispatchStatus, __dbus_connection_get_dispatch_status,
	AROS_LDA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 48, Dbus);
#define __dbus_connection_get_dispatch_status AROS_SLIB_ENTRY(__dbus_connection_get_dispatch_status,Dbus)

AROS_LD1I(DBusDispatchStatus, __dbus_connection_dispatch,
	AROS_LDA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 49, Dbus);
#define __dbus_connection_dispatch AROS_SLIB_ENTRY(__dbus_connection_dispatch,Dbus)

AROS_LD6I(dbus_bool_t, __dbus_connection_set_watch_functions,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(DBusAddWatchFunction, ___add_function, D0),
	AROS_LDA(DBusRemoveWatchFunction, ___remove_function, D1),
	AROS_LDA(DBusWatchToggledFunction, ___toggled_function, D2),
	AROS_LDA(void *, ___data, A1),
	AROS_LDA(DBusFreeFunction, ___free_data_function, D3),
	struct Library *, _base, 50, Dbus);
#define __dbus_connection_set_watch_functions AROS_SLIB_ENTRY(__dbus_connection_set_watch_functions,Dbus)

AROS_LD6I(dbus_bool_t, __dbus_connection_set_timeout_functions,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(DBusAddTimeoutFunction, ___add_function, D0),
	AROS_LDA(DBusRemoveTimeoutFunction, ___remove_function, D1),
	AROS_LDA(DBusTimeoutToggledFunction, ___toggled_function, D2),
	AROS_LDA(void *, ___data, A1),
	AROS_LDA(DBusFreeFunction, ___free_data_function, D3),
	struct Library *, _base, 51, Dbus);
#define __dbus_connection_set_timeout_functions AROS_SLIB_ENTRY(__dbus_connection_set_timeout_functions,Dbus)

AROS_LD4I(void, __dbus_connection_set_wakeup_main_function,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(DBusWakeupMainFunction, ___wakeup_main_function, D0),
	AROS_LDA(void *, ___data, A1),
	AROS_LDA(DBusFreeFunction, ___free_data_function, D1),
	struct Library *, _base, 52, Dbus);
#define __dbus_connection_set_wakeup_main_function AROS_SLIB_ENTRY(__dbus_connection_set_wakeup_main_function,Dbus)

AROS_LD4I(void, __dbus_connection_set_dispatch_status_function,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(DBusDispatchStatusFunction, ___function, D0),
	AROS_LDA(void *, ___data, A1),
	AROS_LDA(DBusFreeFunction, ___free_data_function, D1),
	struct Library *, _base, 53, Dbus);
#define __dbus_connection_set_dispatch_status_function AROS_SLIB_ENTRY(__dbus_connection_set_dispatch_status_function,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_connection_get_unix_fd,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(int *, ___fd, A1),
	struct Library *, _base, 54, Dbus);
#define __dbus_connection_get_unix_fd AROS_SLIB_ENTRY(__dbus_connection_get_unix_fd,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_connection_get_unix_user,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(long unsigned int *, ___uid, A1),
	struct Library *, _base, 55, Dbus);
#define __dbus_connection_get_unix_user AROS_SLIB_ENTRY(__dbus_connection_get_unix_user,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_connection_get_unix_process_id,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(long unsigned int *, ___pid, A1),
	struct Library *, _base, 56, Dbus);
#define __dbus_connection_get_unix_process_id AROS_SLIB_ENTRY(__dbus_connection_get_unix_process_id,Dbus)

AROS_LD4I(void, __dbus_connection_set_unix_user_function,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(DBusAllowUnixUserFunction, ___function, D0),
	AROS_LDA(void *, ___data, A1),
	AROS_LDA(DBusFreeFunction, ___free_data_function, D1),
	struct Library *, _base, 57, Dbus);
#define __dbus_connection_set_unix_user_function AROS_SLIB_ENTRY(__dbus_connection_set_unix_user_function,Dbus)

AROS_LD4I(dbus_bool_t, __dbus_connection_add_filter,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(DBusHandleMessageFunction, ___function, D0),
	AROS_LDA(void *, ___user_data, A1),
	AROS_LDA(DBusFreeFunction, ___free_data_function, D1),
	struct Library *, _base, 58, Dbus);
#define __dbus_connection_add_filter AROS_SLIB_ENTRY(__dbus_connection_add_filter,Dbus)

AROS_LD3I(void, __dbus_connection_remove_filter,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(DBusHandleMessageFunction, ___function, D0),
	AROS_LDA(void *, ___user_data, A1),
	struct Library *, _base, 59, Dbus);
#define __dbus_connection_remove_filter AROS_SLIB_ENTRY(__dbus_connection_remove_filter,Dbus)

AROS_LD4I(dbus_bool_t, __dbus_connection_register_object_path,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(const char *, ___path, A1),
	AROS_LDA(const DBusObjectPathVTable *, ___vtable, A2),
	AROS_LDA(void *, ___user_data, A3),
	struct Library *, _base, 60, Dbus);
#define __dbus_connection_register_object_path AROS_SLIB_ENTRY(__dbus_connection_register_object_path,Dbus)

AROS_LD4I(dbus_bool_t, __dbus_connection_register_fallback,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(const char *, ___path, A1),
	AROS_LDA(const DBusObjectPathVTable *, ___vtable, A2),
	AROS_LDA(void *, ___user_data, A3),
	struct Library *, _base, 61, Dbus);
#define __dbus_connection_register_fallback AROS_SLIB_ENTRY(__dbus_connection_register_fallback,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_connection_unregister_object_path,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(const char *, ___path, A1),
	struct Library *, _base, 62, Dbus);
#define __dbus_connection_unregister_object_path AROS_SLIB_ENTRY(__dbus_connection_unregister_object_path,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_connection_list_registered,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(const char *, ___parent_path, A1),
	AROS_LDA(char ***, ___child_entries, A2),
	struct Library *, _base, 63, Dbus);
#define __dbus_connection_list_registered AROS_SLIB_ENTRY(__dbus_connection_list_registered,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_connection_allocate_data_slot,
	AROS_LDA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 64, Dbus);
#define __dbus_connection_allocate_data_slot AROS_SLIB_ENTRY(__dbus_connection_allocate_data_slot,Dbus)

AROS_LD1I(void, __dbus_connection_free_data_slot,
	AROS_LDA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 65, Dbus);
#define __dbus_connection_free_data_slot AROS_SLIB_ENTRY(__dbus_connection_free_data_slot,Dbus)

AROS_LD4I(dbus_bool_t, __dbus_connection_set_data,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(dbus_int32_t, ___slot, D0),
	AROS_LDA(void *, ___data, A1),
	AROS_LDA(DBusFreeFunction, ___free_data_func, D1),
	struct Library *, _base, 66, Dbus);
#define __dbus_connection_set_data AROS_SLIB_ENTRY(__dbus_connection_set_data,Dbus)

AROS_LD2I(void *, __dbus_connection_get_data,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(dbus_int32_t, ___slot, D0),
	struct Library *, _base, 67, Dbus);
#define __dbus_connection_get_data AROS_SLIB_ENTRY(__dbus_connection_get_data,Dbus)

AROS_LD1I(void, __dbus_connection_set_change_sigpipe,
	AROS_LDA(dbus_bool_t, ___will_modify_sigpipe, D0),
	struct Library *, _base, 68, Dbus);
#define __dbus_connection_set_change_sigpipe AROS_SLIB_ENTRY(__dbus_connection_set_change_sigpipe,Dbus)

AROS_LD2I(void, __dbus_connection_set_max_message_size,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(long int, ___size, D0),
	struct Library *, _base, 69, Dbus);
#define __dbus_connection_set_max_message_size AROS_SLIB_ENTRY(__dbus_connection_set_max_message_size,Dbus)

AROS_LD1I(long int, __dbus_connection_get_max_message_size,
	AROS_LDA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 70, Dbus);
#define __dbus_connection_get_max_message_size AROS_SLIB_ENTRY(__dbus_connection_get_max_message_size,Dbus)

AROS_LD2I(void, __dbus_connection_set_max_received_size,
	AROS_LDA(DBusConnection *, ___connection, A0),
	AROS_LDA(long int, ___size, D0),
	struct Library *, _base, 71, Dbus);
#define __dbus_connection_set_max_received_size AROS_SLIB_ENTRY(__dbus_connection_set_max_received_size,Dbus)

AROS_LD1I(long int, __dbus_connection_get_max_received_size,
	AROS_LDA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 72, Dbus);
#define __dbus_connection_get_max_received_size AROS_SLIB_ENTRY(__dbus_connection_get_max_received_size,Dbus)

AROS_LD1I(long int, __dbus_connection_get_outgoing_size,
	AROS_LDA(DBusConnection *, ___connection, A0),
	struct Library *, _base, 73, Dbus);
#define __dbus_connection_get_outgoing_size AROS_SLIB_ENTRY(__dbus_connection_get_outgoing_size,Dbus)

AROS_LD1I(void, __dbus_error_init,
	AROS_LDA(DBusError *, ___error, A0),
	struct Library *, _base, 74, Dbus);
#define __dbus_error_init AROS_SLIB_ENTRY(__dbus_error_init,Dbus)

AROS_LD1I(void, __dbus_error_free,
	AROS_LDA(DBusError *, ___error, A0),
	struct Library *, _base, 75, Dbus);
#define __dbus_error_free AROS_SLIB_ENTRY(__dbus_error_free,Dbus)

AROS_LD3I(void, __dbus_set_error_const,
	AROS_LDA(DBusError *, ___error, A0),
	AROS_LDA(const char *, ___name, A1),
	AROS_LDA(const char *, ___message, A2),
	struct Library *, _base, 76, Dbus);
#define __dbus_set_error_const AROS_SLIB_ENTRY(__dbus_set_error_const,Dbus)

AROS_LD2I(void, __dbus_move_error,
	AROS_LDA(DBusError *, ___src, A0),
	AROS_LDA(DBusError *, ___dest, A1),
	struct Library *, _base, 77, Dbus);
#define __dbus_move_error AROS_SLIB_ENTRY(__dbus_move_error,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_error_has_name,
	AROS_LDA(const DBusError *, ___error, A0),
	AROS_LDA(const char *, ___name, A1),
	struct Library *, _base, 78, Dbus);
#define __dbus_error_has_name AROS_SLIB_ENTRY(__dbus_error_has_name,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_error_is_set,
	AROS_LDA(const DBusError *, ___error, A0),
	struct Library *, _base, 79, Dbus);
#define __dbus_error_is_set AROS_SLIB_ENTRY(__dbus_error_is_set,Dbus)

#define __dbus_set_error AROS_SLIB_ENTRY(__dbus_set_error,Dbus)

void
__dbus_set_error(DBusError * ___error, const char * ___name, const char * ___format, ...);

AROS_LD1I(void *, __dbus_malloc,
	AROS_LDA(size_t, ___bytes, D0),
	struct Library *, _base, 81, Dbus);
#define __dbus_malloc AROS_SLIB_ENTRY(__dbus_malloc,Dbus)

AROS_LD1I(void *, __dbus_malloc0,
	AROS_LDA(size_t, ___bytes, D0),
	struct Library *, _base, 82, Dbus);
#define __dbus_malloc0 AROS_SLIB_ENTRY(__dbus_malloc0,Dbus)

AROS_LD2I(void *, __dbus_realloc,
	AROS_LDA(void *, ___memory, A0),
	AROS_LDA(size_t, ___bytes, D0),
	struct Library *, _base, 83, Dbus);
#define __dbus_realloc AROS_SLIB_ENTRY(__dbus_realloc,Dbus)

AROS_LD1I(void, __dbus_free,
	AROS_LDA(void *, ___memory, A0),
	struct Library *, _base, 84, Dbus);
#define __dbus_free AROS_SLIB_ENTRY(__dbus_free,Dbus)

AROS_LD1I(void, __dbus_free_string_array,
	AROS_LDA(char **, ___str_array, A0),
	struct Library *, _base, 85, Dbus);
#define __dbus_free_string_array AROS_SLIB_ENTRY(__dbus_free_string_array,Dbus)

AROS_LD0I(void, __dbus_shutdown,
	struct Library *, _base, 86, Dbus);
#define __dbus_shutdown AROS_SLIB_ENTRY(__dbus_shutdown,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_set_reply_serial,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(dbus_uint32_t, ___reply_serial, D0),
	struct Library *, _base, 87, Dbus);
#define __dbus_message_set_reply_serial AROS_SLIB_ENTRY(__dbus_message_set_reply_serial,Dbus)

AROS_LD1I(dbus_uint32_t, __dbus_message_get_serial,
	AROS_LDA(DBusMessage *, ___message, A0),
	struct Library *, _base, 88, Dbus);
#define __dbus_message_get_serial AROS_SLIB_ENTRY(__dbus_message_get_serial,Dbus)

AROS_LD1I(dbus_uint32_t, __dbus_message_get_reply_serial,
	AROS_LDA(DBusMessage *, ___message, A0),
	struct Library *, _base, 89, Dbus);
#define __dbus_message_get_reply_serial AROS_SLIB_ENTRY(__dbus_message_get_reply_serial,Dbus)

AROS_LD1I(DBusMessage *, __dbus_message_new,
	AROS_LDA(int, ___message_type, D0),
	struct Library *, _base, 90, Dbus);
#define __dbus_message_new AROS_SLIB_ENTRY(__dbus_message_new,Dbus)

AROS_LD4I(DBusMessage *, __dbus_message_new_method_call,
	AROS_LDA(const char *, ___service, A0),
	AROS_LDA(const char *, ___path, A1),
	AROS_LDA(const char *, ___interface, A2),
	AROS_LDA(const char *, ___method, A3),
	struct Library *, _base, 91, Dbus);
#define __dbus_message_new_method_call AROS_SLIB_ENTRY(__dbus_message_new_method_call,Dbus)

AROS_LD1I(DBusMessage *, __dbus_message_new_method_return,
	AROS_LDA(DBusMessage *, ___method_call, A0),
	struct Library *, _base, 92, Dbus);
#define __dbus_message_new_method_return AROS_SLIB_ENTRY(__dbus_message_new_method_return,Dbus)

AROS_LD3I(DBusMessage *, __dbus_message_new_signal,
	AROS_LDA(const char *, ___path, A0),
	AROS_LDA(const char *, ___interface, A1),
	AROS_LDA(const char *, ___name, A2),
	struct Library *, _base, 93, Dbus);
#define __dbus_message_new_signal AROS_SLIB_ENTRY(__dbus_message_new_signal,Dbus)

AROS_LD3I(DBusMessage *, __dbus_message_new_error,
	AROS_LDA(DBusMessage *, ___reply_to, A0),
	AROS_LDA(const char *, ___error_name, A1),
	AROS_LDA(const char *, ___error_message, A2),
	struct Library *, _base, 94, Dbus);
#define __dbus_message_new_error AROS_SLIB_ENTRY(__dbus_message_new_error,Dbus)

#define __dbus_message_new_error_printf AROS_SLIB_ENTRY(__dbus_message_new_error_printf,Dbus)

DBusMessage *
__dbus_message_new_error_printf(DBusMessage * ___reply_to, const char * ___error_name, const char * ___error_format, ...);

AROS_LD1I(DBusMessage *, __dbus_message_copy,
	AROS_LDA(const DBusMessage *, ___message, A0),
	struct Library *, _base, 96, Dbus);
#define __dbus_message_copy AROS_SLIB_ENTRY(__dbus_message_copy,Dbus)

AROS_LD1I(DBusMessage *, __dbus_message_ref,
	AROS_LDA(DBusMessage *, ___message, A0),
	struct Library *, _base, 97, Dbus);
#define __dbus_message_ref AROS_SLIB_ENTRY(__dbus_message_ref,Dbus)

AROS_LD1I(void, __dbus_message_unref,
	AROS_LDA(DBusMessage *, ___message, A0),
	struct Library *, _base, 98, Dbus);
#define __dbus_message_unref AROS_SLIB_ENTRY(__dbus_message_unref,Dbus)

AROS_LD1I(int, __dbus_message_get_type,
	AROS_LDA(DBusMessage *, ___message, A0),
	struct Library *, _base, 99, Dbus);
#define __dbus_message_get_type AROS_SLIB_ENTRY(__dbus_message_get_type,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_set_path,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(const char *, ___object_path, A1),
	struct Library *, _base, 100, Dbus);
#define __dbus_message_set_path AROS_SLIB_ENTRY(__dbus_message_set_path,Dbus)

AROS_LD1I(const char *, __dbus_message_get_path,
	AROS_LDA(DBusMessage *, ___message, A0),
	struct Library *, _base, 101, Dbus);
#define __dbus_message_get_path AROS_SLIB_ENTRY(__dbus_message_get_path,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_get_path_decomposed,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(char ***, ___path, A1),
	struct Library *, _base, 102, Dbus);
#define __dbus_message_get_path_decomposed AROS_SLIB_ENTRY(__dbus_message_get_path_decomposed,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_set_interface,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(const char *, ___interface, A1),
	struct Library *, _base, 103, Dbus);
#define __dbus_message_set_interface AROS_SLIB_ENTRY(__dbus_message_set_interface,Dbus)

AROS_LD1I(const char *, __dbus_message_get_interface,
	AROS_LDA(DBusMessage *, ___message, A0),
	struct Library *, _base, 104, Dbus);
#define __dbus_message_get_interface AROS_SLIB_ENTRY(__dbus_message_get_interface,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_set_member,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(const char *, ___member, A1),
	struct Library *, _base, 105, Dbus);
#define __dbus_message_set_member AROS_SLIB_ENTRY(__dbus_message_set_member,Dbus)

AROS_LD1I(const char *, __dbus_message_get_member,
	AROS_LDA(DBusMessage *, ___message, A0),
	struct Library *, _base, 106, Dbus);
#define __dbus_message_get_member AROS_SLIB_ENTRY(__dbus_message_get_member,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_set_error_name,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(const char *, ___error_name, A1),
	struct Library *, _base, 107, Dbus);
#define __dbus_message_set_error_name AROS_SLIB_ENTRY(__dbus_message_set_error_name,Dbus)

AROS_LD1I(const char *, __dbus_message_get_error_name,
	AROS_LDA(DBusMessage *, ___message, A0),
	struct Library *, _base, 108, Dbus);
#define __dbus_message_get_error_name AROS_SLIB_ENTRY(__dbus_message_get_error_name,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_set_destination,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(const char *, ___destination, A1),
	struct Library *, _base, 109, Dbus);
#define __dbus_message_set_destination AROS_SLIB_ENTRY(__dbus_message_set_destination,Dbus)

AROS_LD1I(const char *, __dbus_message_get_destination,
	AROS_LDA(DBusMessage *, ___message, A0),
	struct Library *, _base, 110, Dbus);
#define __dbus_message_get_destination AROS_SLIB_ENTRY(__dbus_message_get_destination,Dbus)

#define __dbus_message_append_args AROS_SLIB_ENTRY(__dbus_message_append_args,Dbus)

dbus_bool_t
__dbus_message_append_args(DBusMessage * ___message, int ___first_arg_type, ...);

#define __dbus_message_get_args AROS_SLIB_ENTRY(__dbus_message_get_args,Dbus)

dbus_bool_t
__dbus_message_get_args(DBusMessage * ___message, DBusError * ___error, int ___first_arg_type, ...);

AROS_LD4I(dbus_bool_t, __dbus_message_get_args_valist,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(DBusError *, ___error, A1),
	AROS_LDA(int, ___first_arg_type, D0),
	AROS_LDA(va_list, ___var_args, D1),
	struct Library *, _base, 113, Dbus);
#define __dbus_message_get_args_valist AROS_SLIB_ENTRY(__dbus_message_get_args_valist,Dbus)

#define __dbus_message_iter_get_args AROS_SLIB_ENTRY(__dbus_message_iter_get_args,Dbus)

dbus_bool_t
__dbus_message_iter_get_args(DBusMessageIter * ___iter, DBusError * ___error, int ___first_arg_type, ...);

AROS_LD2I(dbus_bool_t, __dbus_message_iter_init,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(DBusMessageIter *, ___iter, A1),
	struct Library *, _base, 115, Dbus);
#define __dbus_message_iter_init AROS_SLIB_ENTRY(__dbus_message_iter_init,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_message_iter_has_next,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 116, Dbus);
#define __dbus_message_iter_has_next AROS_SLIB_ENTRY(__dbus_message_iter_has_next,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_message_iter_next,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 117, Dbus);
#define __dbus_message_iter_next AROS_SLIB_ENTRY(__dbus_message_iter_next,Dbus)

AROS_LD1I(int, __dbus_message_iter_get_arg_type,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 118, Dbus);
#define __dbus_message_iter_get_arg_type AROS_SLIB_ENTRY(__dbus_message_iter_get_arg_type,Dbus)

AROS_LD1I(int, __dbus_message_iter_get_array_type,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 119, Dbus);
#define __dbus_message_iter_get_array_type AROS_SLIB_ENTRY(__dbus_message_iter_get_array_type,Dbus)

AROS_LD1I(char *, __dbus_message_iter_get_string,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 120, Dbus);
#define __dbus_message_iter_get_string AROS_SLIB_ENTRY(__dbus_message_iter_get_string,Dbus)

AROS_LD1I(char *, __dbus_message_iter_get_object_path,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 121, Dbus);
#define __dbus_message_iter_get_object_path AROS_SLIB_ENTRY(__dbus_message_iter_get_object_path,Dbus)

AROS_LD4I(dbus_bool_t, __dbus_message_iter_get_custom,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(char **, ___name, A1),
	AROS_LDA(unsigned char **, ___value, A2),
	AROS_LDA(int *, ___len, A3),
	struct Library *, _base, 122, Dbus);
#define __dbus_message_iter_get_custom AROS_SLIB_ENTRY(__dbus_message_iter_get_custom,Dbus)

AROS_LD4I(dbus_bool_t, __dbus_message_iter_get_args_valist,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(DBusError *, ___error, A1),
	AROS_LDA(int, ___first_arg_type, D0),
	AROS_LDA(va_list, ___var_args, D1),
	struct Library *, _base, 123, Dbus);
#define __dbus_message_iter_get_args_valist AROS_SLIB_ENTRY(__dbus_message_iter_get_args_valist,Dbus)

AROS_LD1I(unsigned char, __dbus_message_iter_get_byte,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 124, Dbus);
#define __dbus_message_iter_get_byte AROS_SLIB_ENTRY(__dbus_message_iter_get_byte,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_message_iter_get_boolean,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 125, Dbus);
#define __dbus_message_iter_get_boolean AROS_SLIB_ENTRY(__dbus_message_iter_get_boolean,Dbus)

AROS_LD1I(dbus_int32_t, __dbus_message_iter_get_int32,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 126, Dbus);
#define __dbus_message_iter_get_int32 AROS_SLIB_ENTRY(__dbus_message_iter_get_int32,Dbus)

AROS_LD1I(dbus_uint32_t, __dbus_message_iter_get_uint32,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 127, Dbus);
#define __dbus_message_iter_get_uint32 AROS_SLIB_ENTRY(__dbus_message_iter_get_uint32,Dbus)

AROS_LD1I(dbus_int64_t, __dbus_message_iter_get_int64,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 128, Dbus);
#define __dbus_message_iter_get_int64 AROS_SLIB_ENTRY(__dbus_message_iter_get_int64,Dbus)

AROS_LD1I(dbus_uint64_t, __dbus_message_iter_get_uint64,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 129, Dbus);
#define __dbus_message_iter_get_uint64 AROS_SLIB_ENTRY(__dbus_message_iter_get_uint64,Dbus)

AROS_LD1I(double, __dbus_message_iter_get_double,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 130, Dbus);
#define __dbus_message_iter_get_double AROS_SLIB_ENTRY(__dbus_message_iter_get_double,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_init_array_iterator,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(DBusMessageIter *, ___array_iter, A1),
	AROS_LDA(int *, ___array_type, A2),
	struct Library *, _base, 131, Dbus);
#define __dbus_message_iter_init_array_iterator AROS_SLIB_ENTRY(__dbus_message_iter_init_array_iterator,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_iter_init_dict_iterator,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(DBusMessageIter *, ___dict_iter, A1),
	struct Library *, _base, 132, Dbus);
#define __dbus_message_iter_init_dict_iterator AROS_SLIB_ENTRY(__dbus_message_iter_init_dict_iterator,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_get_byte_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(unsigned char **, ___value, A1),
	AROS_LDA(int *, ___len, A2),
	struct Library *, _base, 133, Dbus);
#define __dbus_message_iter_get_byte_array AROS_SLIB_ENTRY(__dbus_message_iter_get_byte_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_get_boolean_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(unsigned char **, ___value, A1),
	AROS_LDA(int *, ___len, A2),
	struct Library *, _base, 134, Dbus);
#define __dbus_message_iter_get_boolean_array AROS_SLIB_ENTRY(__dbus_message_iter_get_boolean_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_get_int32_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(dbus_int32_t **, ___value, A1),
	AROS_LDA(int *, ___len, A2),
	struct Library *, _base, 135, Dbus);
#define __dbus_message_iter_get_int32_array AROS_SLIB_ENTRY(__dbus_message_iter_get_int32_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_get_uint32_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(dbus_uint32_t **, ___value, A1),
	AROS_LDA(int *, ___len, A2),
	struct Library *, _base, 136, Dbus);
#define __dbus_message_iter_get_uint32_array AROS_SLIB_ENTRY(__dbus_message_iter_get_uint32_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_get_int64_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(dbus_int64_t **, ___value, A1),
	AROS_LDA(int *, ___len, A2),
	struct Library *, _base, 137, Dbus);
#define __dbus_message_iter_get_int64_array AROS_SLIB_ENTRY(__dbus_message_iter_get_int64_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_get_uint64_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(dbus_uint64_t **, ___value, A1),
	AROS_LDA(int *, ___len, A2),
	struct Library *, _base, 138, Dbus);
#define __dbus_message_iter_get_uint64_array AROS_SLIB_ENTRY(__dbus_message_iter_get_uint64_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_get_double_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(double **, ___value, A1),
	AROS_LDA(int *, ___len, A2),
	struct Library *, _base, 139, Dbus);
#define __dbus_message_iter_get_double_array AROS_SLIB_ENTRY(__dbus_message_iter_get_double_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_get_string_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(char ***, ___value, A1),
	AROS_LDA(int *, ___len, A2),
	struct Library *, _base, 140, Dbus);
#define __dbus_message_iter_get_string_array AROS_SLIB_ENTRY(__dbus_message_iter_get_string_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_get_object_path_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(char ***, ___value, A1),
	AROS_LDA(int *, ___len, A2),
	struct Library *, _base, 141, Dbus);
#define __dbus_message_iter_get_object_path_array AROS_SLIB_ENTRY(__dbus_message_iter_get_object_path_array,Dbus)

AROS_LD1I(char *, __dbus_message_iter_get_dict_key,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 142, Dbus);
#define __dbus_message_iter_get_dict_key AROS_SLIB_ENTRY(__dbus_message_iter_get_dict_key,Dbus)

AROS_LD2I(void, __dbus_message_append_iter_init,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(DBusMessageIter *, ___iter, A1),
	struct Library *, _base, 143, Dbus);
#define __dbus_message_append_iter_init AROS_SLIB_ENTRY(__dbus_message_append_iter_init,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_message_iter_append_nil,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	struct Library *, _base, 144, Dbus);
#define __dbus_message_iter_append_nil AROS_SLIB_ENTRY(__dbus_message_iter_append_nil,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_iter_append_boolean,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(dbus_bool_t, ___value, D0),
	struct Library *, _base, 145, Dbus);
#define __dbus_message_iter_append_boolean AROS_SLIB_ENTRY(__dbus_message_iter_append_boolean,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_iter_append_byte,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(unsigned char, ___value, D0),
	struct Library *, _base, 146, Dbus);
#define __dbus_message_iter_append_byte AROS_SLIB_ENTRY(__dbus_message_iter_append_byte,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_iter_append_int32,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(dbus_int32_t, ___value, D0),
	struct Library *, _base, 147, Dbus);
#define __dbus_message_iter_append_int32 AROS_SLIB_ENTRY(__dbus_message_iter_append_int32,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_iter_append_uint32,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(dbus_uint32_t, ___value, D0),
	struct Library *, _base, 148, Dbus);
#define __dbus_message_iter_append_uint32 AROS_SLIB_ENTRY(__dbus_message_iter_append_uint32,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_iter_append_int64,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(dbus_int64_t, ___value, D0),
	struct Library *, _base, 149, Dbus);
#define __dbus_message_iter_append_int64 AROS_SLIB_ENTRY(__dbus_message_iter_append_int64,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_iter_append_uint64,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(dbus_uint64_t, ___value, D0),
	struct Library *, _base, 150, Dbus);
#define __dbus_message_iter_append_uint64 AROS_SLIB_ENTRY(__dbus_message_iter_append_uint64,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_iter_append_double,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(double, ___value, D0),
	struct Library *, _base, 151, Dbus);
#define __dbus_message_iter_append_double AROS_SLIB_ENTRY(__dbus_message_iter_append_double,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_iter_append_string,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(const char *, ___value, A1),
	struct Library *, _base, 152, Dbus);
#define __dbus_message_iter_append_string AROS_SLIB_ENTRY(__dbus_message_iter_append_string,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_iter_append_object_path,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(const char *, ___value, A1),
	struct Library *, _base, 153, Dbus);
#define __dbus_message_iter_append_object_path AROS_SLIB_ENTRY(__dbus_message_iter_append_object_path,Dbus)

AROS_LD4I(dbus_bool_t, __dbus_message_iter_append_custom,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(const char *, ___name, A1),
	AROS_LDA(const unsigned char *, ___data, A2),
	AROS_LDA(int, ___len, D0),
	struct Library *, _base, 154, Dbus);
#define __dbus_message_iter_append_custom AROS_SLIB_ENTRY(__dbus_message_iter_append_custom,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_iter_append_dict_key,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(const char *, ___value, A1),
	struct Library *, _base, 155, Dbus);
#define __dbus_message_iter_append_dict_key AROS_SLIB_ENTRY(__dbus_message_iter_append_dict_key,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_append_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(DBusMessageIter *, ___array_iter, A1),
	AROS_LDA(int, ___element_type, D0),
	struct Library *, _base, 156, Dbus);
#define __dbus_message_iter_append_array AROS_SLIB_ENTRY(__dbus_message_iter_append_array,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_iter_append_dict,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(DBusMessageIter *, ___dict_iter, A1),
	struct Library *, _base, 157, Dbus);
#define __dbus_message_iter_append_dict AROS_SLIB_ENTRY(__dbus_message_iter_append_dict,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_append_args_valist,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(int, ___first_arg_type, D0),
	AROS_LDA(va_list, ___var_args, D1),
	struct Library *, _base, 158, Dbus);
#define __dbus_message_append_args_valist AROS_SLIB_ENTRY(__dbus_message_append_args_valist,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_append_boolean_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(const unsigned char *, ___value, A1),
	AROS_LDA(int, ___len, D0),
	struct Library *, _base, 159, Dbus);
#define __dbus_message_iter_append_boolean_array AROS_SLIB_ENTRY(__dbus_message_iter_append_boolean_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_append_int32_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(const dbus_int32_t *, ___value, A1),
	AROS_LDA(int, ___len, D0),
	struct Library *, _base, 160, Dbus);
#define __dbus_message_iter_append_int32_array AROS_SLIB_ENTRY(__dbus_message_iter_append_int32_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_append_uint32_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(const dbus_uint32_t *, ___value, A1),
	AROS_LDA(int, ___len, D0),
	struct Library *, _base, 161, Dbus);
#define __dbus_message_iter_append_uint32_array AROS_SLIB_ENTRY(__dbus_message_iter_append_uint32_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_append_int64_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(const dbus_int64_t *, ___value, A1),
	AROS_LDA(int, ___len, D0),
	struct Library *, _base, 162, Dbus);
#define __dbus_message_iter_append_int64_array AROS_SLIB_ENTRY(__dbus_message_iter_append_int64_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_append_uint64_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(const dbus_uint64_t *, ___value, A1),
	AROS_LDA(int, ___len, D0),
	struct Library *, _base, 163, Dbus);
#define __dbus_message_iter_append_uint64_array AROS_SLIB_ENTRY(__dbus_message_iter_append_uint64_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_append_double_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(const double *, ___value, A1),
	AROS_LDA(int, ___len, D0),
	struct Library *, _base, 164, Dbus);
#define __dbus_message_iter_append_double_array AROS_SLIB_ENTRY(__dbus_message_iter_append_double_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_append_byte_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(const unsigned char *, ___value, A1),
	AROS_LDA(int, ___len, D0),
	struct Library *, _base, 165, Dbus);
#define __dbus_message_iter_append_byte_array AROS_SLIB_ENTRY(__dbus_message_iter_append_byte_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_append_string_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(const char **, ___value, A1),
	AROS_LDA(int, ___len, D0),
	struct Library *, _base, 166, Dbus);
#define __dbus_message_iter_append_string_array AROS_SLIB_ENTRY(__dbus_message_iter_append_string_array,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_iter_append_object_path_array,
	AROS_LDA(DBusMessageIter *, ___iter, A0),
	AROS_LDA(const char **, ___value, A1),
	AROS_LDA(int, ___len, D0),
	struct Library *, _base, 167, Dbus);
#define __dbus_message_iter_append_object_path_array AROS_SLIB_ENTRY(__dbus_message_iter_append_object_path_array,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_set_sender,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(const char *, ___sender, A1),
	struct Library *, _base, 168, Dbus);
#define __dbus_message_set_sender AROS_SLIB_ENTRY(__dbus_message_set_sender,Dbus)

AROS_LD2I(void, __dbus_message_set_no_reply,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(dbus_bool_t, ___no_reply, D0),
	struct Library *, _base, 169, Dbus);
#define __dbus_message_set_no_reply AROS_SLIB_ENTRY(__dbus_message_set_no_reply,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_message_get_no_reply,
	AROS_LDA(DBusMessage *, ___message, A0),
	struct Library *, _base, 170, Dbus);
#define __dbus_message_get_no_reply AROS_SLIB_ENTRY(__dbus_message_get_no_reply,Dbus)

AROS_LD2I(void, __dbus_message_set_auto_activation,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(dbus_bool_t, ___auto_activation, D0),
	struct Library *, _base, 171, Dbus);
#define __dbus_message_set_auto_activation AROS_SLIB_ENTRY(__dbus_message_set_auto_activation,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_message_get_auto_activation,
	AROS_LDA(DBusMessage *, ___message, A0),
	struct Library *, _base, 172, Dbus);
#define __dbus_message_get_auto_activation AROS_SLIB_ENTRY(__dbus_message_get_auto_activation,Dbus)

AROS_LD1I(const char *, __dbus_message_get_sender,
	AROS_LDA(DBusMessage *, ___message, A0),
	struct Library *, _base, 173, Dbus);
#define __dbus_message_get_sender AROS_SLIB_ENTRY(__dbus_message_get_sender,Dbus)

AROS_LD1I(const char *, __dbus_message_get_signature,
	AROS_LDA(DBusMessage *, ___message, A0),
	struct Library *, _base, 174, Dbus);
#define __dbus_message_get_signature AROS_SLIB_ENTRY(__dbus_message_get_signature,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_is_method_call,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(const char *, ___interface, A1),
	AROS_LDA(const char *, ___method, A2),
	struct Library *, _base, 175, Dbus);
#define __dbus_message_is_method_call AROS_SLIB_ENTRY(__dbus_message_is_method_call,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_message_is_signal,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(const char *, ___interface, A1),
	AROS_LDA(const char *, ___signal_name, A2),
	struct Library *, _base, 176, Dbus);
#define __dbus_message_is_signal AROS_SLIB_ENTRY(__dbus_message_is_signal,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_is_error,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(const char *, ___error_name, A1),
	struct Library *, _base, 177, Dbus);
#define __dbus_message_is_error AROS_SLIB_ENTRY(__dbus_message_is_error,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_has_destination,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(const char *, ___service, A1),
	struct Library *, _base, 178, Dbus);
#define __dbus_message_has_destination AROS_SLIB_ENTRY(__dbus_message_has_destination,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_has_sender,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(const char *, ___service, A1),
	struct Library *, _base, 179, Dbus);
#define __dbus_message_has_sender AROS_SLIB_ENTRY(__dbus_message_has_sender,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_message_has_signature,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(const char *, ___signature, A1),
	struct Library *, _base, 180, Dbus);
#define __dbus_message_has_signature AROS_SLIB_ENTRY(__dbus_message_has_signature,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_set_error_from_message,
	AROS_LDA(DBusError *, ___error, A0),
	AROS_LDA(DBusMessage *, ___message, A1),
	struct Library *, _base, 181, Dbus);
#define __dbus_set_error_from_message AROS_SLIB_ENTRY(__dbus_set_error_from_message,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_message_allocate_data_slot,
	AROS_LDA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 182, Dbus);
#define __dbus_message_allocate_data_slot AROS_SLIB_ENTRY(__dbus_message_allocate_data_slot,Dbus)

AROS_LD1I(void, __dbus_message_free_data_slot,
	AROS_LDA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 183, Dbus);
#define __dbus_message_free_data_slot AROS_SLIB_ENTRY(__dbus_message_free_data_slot,Dbus)

AROS_LD4I(dbus_bool_t, __dbus_message_set_data,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(dbus_int32_t, ___slot, D0),
	AROS_LDA(void *, ___data, A1),
	AROS_LDA(DBusFreeFunction, ___free_data_func, D1),
	struct Library *, _base, 184, Dbus);
#define __dbus_message_set_data AROS_SLIB_ENTRY(__dbus_message_set_data,Dbus)

AROS_LD2I(void *, __dbus_message_get_data,
	AROS_LDA(DBusMessage *, ___message, A0),
	AROS_LDA(dbus_int32_t, ___slot, D0),
	struct Library *, _base, 185, Dbus);
#define __dbus_message_get_data AROS_SLIB_ENTRY(__dbus_message_get_data,Dbus)

AROS_LD1I(int, __dbus_message_type_from_string,
	AROS_LDA(const char *, ___type_str, A0),
	struct Library *, _base, 186, Dbus);
#define __dbus_message_type_from_string AROS_SLIB_ENTRY(__dbus_message_type_from_string,Dbus)

AROS_LD1I(DBusPendingCall *, __dbus_pending_call_ref,
	AROS_LDA(DBusPendingCall *, ___pending, A0),
	struct Library *, _base, 187, Dbus);
#define __dbus_pending_call_ref AROS_SLIB_ENTRY(__dbus_pending_call_ref,Dbus)

AROS_LD1I(void, __dbus_pending_call_unref,
	AROS_LDA(DBusPendingCall *, ___pending, A0),
	struct Library *, _base, 188, Dbus);
#define __dbus_pending_call_unref AROS_SLIB_ENTRY(__dbus_pending_call_unref,Dbus)

AROS_LD4I(dbus_bool_t, __dbus_pending_call_set_notify,
	AROS_LDA(DBusPendingCall *, ___pending, A0),
	AROS_LDA(DBusPendingCallNotifyFunction, ___function, D0),
	AROS_LDA(void *, ___user_data, A1),
	AROS_LDA(DBusFreeFunction, ___free_user_data, D1),
	struct Library *, _base, 189, Dbus);
#define __dbus_pending_call_set_notify AROS_SLIB_ENTRY(__dbus_pending_call_set_notify,Dbus)

AROS_LD1I(void, __dbus_pending_call_cancel,
	AROS_LDA(DBusPendingCall *, ___pending, A0),
	struct Library *, _base, 190, Dbus);
#define __dbus_pending_call_cancel AROS_SLIB_ENTRY(__dbus_pending_call_cancel,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_pending_call_get_completed,
	AROS_LDA(DBusPendingCall *, ___pending, A0),
	struct Library *, _base, 191, Dbus);
#define __dbus_pending_call_get_completed AROS_SLIB_ENTRY(__dbus_pending_call_get_completed,Dbus)

AROS_LD1I(DBusMessage *, __dbus_pending_call_get_reply,
	AROS_LDA(DBusPendingCall *, ___pending, A0),
	struct Library *, _base, 192, Dbus);
#define __dbus_pending_call_get_reply AROS_SLIB_ENTRY(__dbus_pending_call_get_reply,Dbus)

AROS_LD1I(void, __dbus_pending_call_block,
	AROS_LDA(DBusPendingCall *, ___pending, A0),
	struct Library *, _base, 193, Dbus);
#define __dbus_pending_call_block AROS_SLIB_ENTRY(__dbus_pending_call_block,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_pending_call_allocate_data_slot,
	AROS_LDA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 194, Dbus);
#define __dbus_pending_call_allocate_data_slot AROS_SLIB_ENTRY(__dbus_pending_call_allocate_data_slot,Dbus)

AROS_LD1I(void, __dbus_pending_call_free_data_slot,
	AROS_LDA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 195, Dbus);
#define __dbus_pending_call_free_data_slot AROS_SLIB_ENTRY(__dbus_pending_call_free_data_slot,Dbus)

AROS_LD4I(dbus_bool_t, __dbus_pending_call_set_data,
	AROS_LDA(DBusPendingCall *, ___pending, A0),
	AROS_LDA(dbus_int32_t, ___slot, D0),
	AROS_LDA(void *, ___data, A1),
	AROS_LDA(DBusFreeFunction, ___free_data_func, D1),
	struct Library *, _base, 196, Dbus);
#define __dbus_pending_call_set_data AROS_SLIB_ENTRY(__dbus_pending_call_set_data,Dbus)

AROS_LD2I(void *, __dbus_pending_call_get_data,
	AROS_LDA(DBusPendingCall *, ___pending, A0),
	AROS_LDA(dbus_int32_t, ___slot, D0),
	struct Library *, _base, 197, Dbus);
#define __dbus_pending_call_get_data AROS_SLIB_ENTRY(__dbus_pending_call_get_data,Dbus)

AROS_LD2I(DBusServer *, __dbus_server_listen,
	AROS_LDA(const char *, ___address, A0),
	AROS_LDA(DBusError *, ___error, A1),
	struct Library *, _base, 198, Dbus);
#define __dbus_server_listen AROS_SLIB_ENTRY(__dbus_server_listen,Dbus)

AROS_LD1I(DBusServer *, __dbus_server_ref,
	AROS_LDA(DBusServer *, ___server, A0),
	struct Library *, _base, 199, Dbus);
#define __dbus_server_ref AROS_SLIB_ENTRY(__dbus_server_ref,Dbus)

AROS_LD1I(void, __dbus_server_unref,
	AROS_LDA(DBusServer *, ___server, A0),
	struct Library *, _base, 200, Dbus);
#define __dbus_server_unref AROS_SLIB_ENTRY(__dbus_server_unref,Dbus)

AROS_LD1I(void, __dbus_server_disconnect,
	AROS_LDA(DBusServer *, ___server, A0),
	struct Library *, _base, 201, Dbus);
#define __dbus_server_disconnect AROS_SLIB_ENTRY(__dbus_server_disconnect,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_server_get_is_connected,
	AROS_LDA(DBusServer *, ___server, A0),
	struct Library *, _base, 202, Dbus);
#define __dbus_server_get_is_connected AROS_SLIB_ENTRY(__dbus_server_get_is_connected,Dbus)

AROS_LD1I(char *, __dbus_server_get_address,
	AROS_LDA(DBusServer *, ___server, A0),
	struct Library *, _base, 203, Dbus);
#define __dbus_server_get_address AROS_SLIB_ENTRY(__dbus_server_get_address,Dbus)

AROS_LD4I(void, __dbus_server_set_new_connection_function,
	AROS_LDA(DBusServer *, ___server, A0),
	AROS_LDA(DBusNewConnectionFunction, ___function, D0),
	AROS_LDA(void *, ___data, A1),
	AROS_LDA(DBusFreeFunction, ___free_data_function, D1),
	struct Library *, _base, 204, Dbus);
#define __dbus_server_set_new_connection_function AROS_SLIB_ENTRY(__dbus_server_set_new_connection_function,Dbus)

AROS_LD6I(dbus_bool_t, __dbus_server_set_watch_functions,
	AROS_LDA(DBusServer *, ___server, A0),
	AROS_LDA(DBusAddWatchFunction, ___add_function, D0),
	AROS_LDA(DBusRemoveWatchFunction, ___remove_function, D1),
	AROS_LDA(DBusWatchToggledFunction, ___toggled_function, D2),
	AROS_LDA(void *, ___data, A1),
	AROS_LDA(DBusFreeFunction, ___free_data_function, D3),
	struct Library *, _base, 205, Dbus);
#define __dbus_server_set_watch_functions AROS_SLIB_ENTRY(__dbus_server_set_watch_functions,Dbus)

AROS_LD6I(dbus_bool_t, __dbus_server_set_timeout_functions,
	AROS_LDA(DBusServer *, ___server, A0),
	AROS_LDA(DBusAddTimeoutFunction, ___add_function, D0),
	AROS_LDA(DBusRemoveTimeoutFunction, ___remove_function, D1),
	AROS_LDA(DBusTimeoutToggledFunction, ___toggled_function, D2),
	AROS_LDA(void *, ___data, A1),
	AROS_LDA(DBusFreeFunction, ___free_data_function, D3),
	struct Library *, _base, 206, Dbus);
#define __dbus_server_set_timeout_functions AROS_SLIB_ENTRY(__dbus_server_set_timeout_functions,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_server_set_auth_mechanisms,
	AROS_LDA(DBusServer *, ___server, A0),
	AROS_LDA(const char **, ___mechanisms, A1),
	struct Library *, _base, 207, Dbus);
#define __dbus_server_set_auth_mechanisms AROS_SLIB_ENTRY(__dbus_server_set_auth_mechanisms,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_server_allocate_data_slot,
	AROS_LDA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 208, Dbus);
#define __dbus_server_allocate_data_slot AROS_SLIB_ENTRY(__dbus_server_allocate_data_slot,Dbus)

AROS_LD1I(void, __dbus_server_free_data_slot,
	AROS_LDA(dbus_int32_t *, ___slot_p, A0),
	struct Library *, _base, 209, Dbus);
#define __dbus_server_free_data_slot AROS_SLIB_ENTRY(__dbus_server_free_data_slot,Dbus)

AROS_LD4I(dbus_bool_t, __dbus_server_set_data,
	AROS_LDA(DBusServer *, ___server, A0),
	AROS_LDA(int, ___slot, D0),
	AROS_LDA(void *, ___data, A1),
	AROS_LDA(DBusFreeFunction, ___free_data_func, D1),
	struct Library *, _base, 210, Dbus);
#define __dbus_server_set_data AROS_SLIB_ENTRY(__dbus_server_set_data,Dbus)

AROS_LD2I(void *, __dbus_server_get_data,
	AROS_LDA(DBusServer *, ___server, A0),
	AROS_LDA(int, ___slot, D0),
	struct Library *, _base, 211, Dbus);
#define __dbus_server_get_data AROS_SLIB_ENTRY(__dbus_server_get_data,Dbus)

AROS_LD1I(void, __dbus_internal_do_not_use_run_tests,
	AROS_LDA(const char *, ___test_data_dir, A0),
	struct Library *, _base, 212, Dbus);
#define __dbus_internal_do_not_use_run_tests AROS_SLIB_ENTRY(__dbus_internal_do_not_use_run_tests,Dbus)

AROS_LD0I(DBusMutex *, __dbus_mutex_new,
	struct Library *, _base, 213, Dbus);
#define __dbus_mutex_new AROS_SLIB_ENTRY(__dbus_mutex_new,Dbus)

AROS_LD1I(void, __dbus_mutex_free,
	AROS_LDA(DBusMutex *, ___mutex, A0),
	struct Library *, _base, 214, Dbus);
#define __dbus_mutex_free AROS_SLIB_ENTRY(__dbus_mutex_free,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_mutex_lock,
	AROS_LDA(DBusMutex *, ___mutex, A0),
	struct Library *, _base, 215, Dbus);
#define __dbus_mutex_lock AROS_SLIB_ENTRY(__dbus_mutex_lock,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_mutex_unlock,
	AROS_LDA(DBusMutex *, ___mutex, A0),
	struct Library *, _base, 216, Dbus);
#define __dbus_mutex_unlock AROS_SLIB_ENTRY(__dbus_mutex_unlock,Dbus)

AROS_LD0I(DBusCondVar *, __dbus_condvar_new,
	struct Library *, _base, 217, Dbus);
#define __dbus_condvar_new AROS_SLIB_ENTRY(__dbus_condvar_new,Dbus)

AROS_LD1I(void, __dbus_condvar_free,
	AROS_LDA(DBusCondVar *, ___cond, A0),
	struct Library *, _base, 218, Dbus);
#define __dbus_condvar_free AROS_SLIB_ENTRY(__dbus_condvar_free,Dbus)

AROS_LD2I(void, __dbus_condvar_wait,
	AROS_LDA(DBusCondVar *, ___cond, A0),
	AROS_LDA(DBusMutex *, ___mutex, A1),
	struct Library *, _base, 219, Dbus);
#define __dbus_condvar_wait AROS_SLIB_ENTRY(__dbus_condvar_wait,Dbus)

AROS_LD3I(dbus_bool_t, __dbus_condvar_wait_timeout,
	AROS_LDA(DBusCondVar *, ___cond, A0),
	AROS_LDA(DBusMutex *, ___mutex, A1),
	AROS_LDA(int, ___timeout_milliseconds, D0),
	struct Library *, _base, 220, Dbus);
#define __dbus_condvar_wait_timeout AROS_SLIB_ENTRY(__dbus_condvar_wait_timeout,Dbus)

AROS_LD1I(void, __dbus_condvar_wake_one,
	AROS_LDA(DBusCondVar *, ___cond, A0),
	struct Library *, _base, 221, Dbus);
#define __dbus_condvar_wake_one AROS_SLIB_ENTRY(__dbus_condvar_wake_one,Dbus)

AROS_LD1I(void, __dbus_condvar_wake_all,
	AROS_LDA(DBusCondVar *, ___cond, A0),
	struct Library *, _base, 222, Dbus);
#define __dbus_condvar_wake_all AROS_SLIB_ENTRY(__dbus_condvar_wake_all,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_threads_init,
	AROS_LDA(const DBusThreadFunctions *, ___functions, A0),
	struct Library *, _base, 223, Dbus);
#define __dbus_threads_init AROS_SLIB_ENTRY(__dbus_threads_init,Dbus)

AROS_LD1I(int, __dbus_timeout_get_interval,
	AROS_LDA(DBusTimeout *, ___timeout, A0),
	struct Library *, _base, 224, Dbus);
#define __dbus_timeout_get_interval AROS_SLIB_ENTRY(__dbus_timeout_get_interval,Dbus)

AROS_LD1I(void *, __dbus_timeout_get_data,
	AROS_LDA(DBusTimeout *, ___timeout, A0),
	struct Library *, _base, 225, Dbus);
#define __dbus_timeout_get_data AROS_SLIB_ENTRY(__dbus_timeout_get_data,Dbus)

AROS_LD3I(void, __dbus_timeout_set_data,
	AROS_LDA(DBusTimeout *, ___timeout, A0),
	AROS_LDA(void *, ___data, A1),
	AROS_LDA(DBusFreeFunction, ___free_data_function, D0),
	struct Library *, _base, 226, Dbus);
#define __dbus_timeout_set_data AROS_SLIB_ENTRY(__dbus_timeout_set_data,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_timeout_handle,
	AROS_LDA(DBusTimeout *, ___timeout, A0),
	struct Library *, _base, 227, Dbus);
#define __dbus_timeout_handle AROS_SLIB_ENTRY(__dbus_timeout_handle,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_timeout_get_enabled,
	AROS_LDA(DBusTimeout *, ___timeout, A0),
	struct Library *, _base, 228, Dbus);
#define __dbus_timeout_get_enabled AROS_SLIB_ENTRY(__dbus_timeout_get_enabled,Dbus)

AROS_LD1I(int, __dbus_watch_get_fd,
	AROS_LDA(DBusWatch *, ___watch, A0),
	struct Library *, _base, 229, Dbus);
#define __dbus_watch_get_fd AROS_SLIB_ENTRY(__dbus_watch_get_fd,Dbus)

AROS_LD1I(unsigned int, __dbus_watch_get_flags,
	AROS_LDA(DBusWatch *, ___watch, A0),
	struct Library *, _base, 230, Dbus);
#define __dbus_watch_get_flags AROS_SLIB_ENTRY(__dbus_watch_get_flags,Dbus)

AROS_LD1I(void *, __dbus_watch_get_data,
	AROS_LDA(DBusWatch *, ___watch, A0),
	struct Library *, _base, 231, Dbus);
#define __dbus_watch_get_data AROS_SLIB_ENTRY(__dbus_watch_get_data,Dbus)

AROS_LD3I(void, __dbus_watch_set_data,
	AROS_LDA(DBusWatch *, ___watch, A0),
	AROS_LDA(void *, ___data, A1),
	AROS_LDA(DBusFreeFunction, ___free_data_function, D0),
	struct Library *, _base, 232, Dbus);
#define __dbus_watch_set_data AROS_SLIB_ENTRY(__dbus_watch_set_data,Dbus)

AROS_LD1I(dbus_bool_t, __dbus_watch_get_enabled,
	AROS_LDA(DBusWatch *, ___watch, A0),
	struct Library *, _base, 233, Dbus);
#define __dbus_watch_get_enabled AROS_SLIB_ENTRY(__dbus_watch_get_enabled,Dbus)

AROS_LD2I(dbus_bool_t, __dbus_watch_handle,
	AROS_LDA(DBusWatch *, ___watch, A0),
	AROS_LDA(unsigned int, ___flags, D0),
	struct Library *, _base, 234, Dbus);
#define __dbus_watch_handle AROS_SLIB_ENTRY(__dbus_watch_handle,Dbus)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GATEPROTO_DBUS_H */
