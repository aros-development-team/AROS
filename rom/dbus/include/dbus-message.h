/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus-message.h DBusMessage object
 *
 * Copyright (C) 2002, 2003 Red Hat Inc.
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

#ifndef DBUS_MESSAGE_H
#define DBUS_MESSAGE_H

#include <dbus/dbus-macros.h>
#include <dbus/dbus-types.h>
#include <dbus/dbus-arch-deps.h>
#include <dbus/dbus-memory.h>
#include <dbus/dbus-errors.h>
#include <stdarg.h>

DBUS_BEGIN_DECLS;

typedef struct DBusMessage DBusMessage;
typedef struct DBusMessageIter DBusMessageIter;

/**
 * DBusMessageIter struct; contains no public fields 
 */
struct DBusMessageIter
{ 
  void *dummy1;         /**< Don't use this */
  void *dummy2;         /**< Don't use this */
  dbus_uint32_t dummy3; /**< Don't use this */
  int dummy4;           /**< Don't use this */
  int dummy5;           /**< Don't use this */
  int dummy6;           /**< Don't use this */
  int dummy7;           /**< Don't use this */
  int dummy8;           /**< Don't use this */
  int dummy9;           /**< Don't use this */
  int dummy10;          /**< Don't use this */
  int dummy11;          /**< Don't use this */
  int pad1;             /**< Don't use this */
  int pad2;             /**< Don't use this */
  void *pad3;           /**< Don't use this */
};

DBusMessage* dbus_message_new               (int          message_type);
DBusMessage* dbus_message_new_method_call   (const char  *service,
                                             const char  *path,
                                             const char  *interface,
                                             const char  *method);
DBusMessage* dbus_message_new_method_return (DBusMessage *method_call);
DBusMessage* dbus_message_new_signal        (const char  *path,
                                             const char  *interface,
                                             const char  *name);
DBusMessage* dbus_message_new_error         (DBusMessage *reply_to,
                                             const char  *error_name,
                                             const char  *error_message);
DBusMessage* dbus_message_new_error_printf  (DBusMessage *reply_to,
                                             const char  *error_name,
                                             const char  *error_format,
					     ...);

DBusMessage* dbus_message_copy              (const DBusMessage *message);

DBusMessage*  dbus_message_ref              (DBusMessage   *message);
void          dbus_message_unref            (DBusMessage   *message);
int           dbus_message_get_type         (DBusMessage   *message);
dbus_bool_t   dbus_message_set_path         (DBusMessage   *message,
                                             const char    *object_path);
const char*   dbus_message_get_path         (DBusMessage   *message);
dbus_bool_t   dbus_message_set_interface    (DBusMessage   *message,
                                             const char    *interface);
const char*   dbus_message_get_interface    (DBusMessage   *message);
dbus_bool_t   dbus_message_set_member       (DBusMessage   *message,
                                             const char    *member);
const char*   dbus_message_get_member       (DBusMessage   *message);
dbus_bool_t   dbus_message_set_error_name   (DBusMessage   *message,
                                             const char    *name);
const char*   dbus_message_get_error_name   (DBusMessage   *message);
dbus_bool_t   dbus_message_set_destination  (DBusMessage   *message,
                                             const char    *destination);
const char*   dbus_message_get_destination  (DBusMessage   *message);
dbus_bool_t   dbus_message_set_sender       (DBusMessage   *message,
                                             const char    *sender);
const char*   dbus_message_get_sender       (DBusMessage   *message);
const char*   dbus_message_get_signature    (DBusMessage   *message);
void          dbus_message_set_no_reply     (DBusMessage   *message,
                                             dbus_bool_t    no_reply);
dbus_bool_t   dbus_message_get_no_reply     (DBusMessage   *message);
dbus_bool_t   dbus_message_is_method_call   (DBusMessage   *message,
                                             const char    *interface,
                                             const char    *method);
dbus_bool_t   dbus_message_is_signal        (DBusMessage   *message,
                                             const char    *interface,
                                             const char    *signal_name);
dbus_bool_t   dbus_message_is_error         (DBusMessage   *message,
                                             const char    *error_name);
dbus_bool_t   dbus_message_has_destination  (DBusMessage   *message,
                                             const char    *service);
dbus_bool_t   dbus_message_has_sender       (DBusMessage   *message,
                                             const char    *service);
dbus_bool_t   dbus_message_has_signature    (DBusMessage   *message,
                                             const char    *signature);
dbus_uint32_t dbus_message_get_serial       (DBusMessage   *message);
dbus_bool_t   dbus_message_set_reply_serial (DBusMessage   *message,
                                             dbus_uint32_t  reply_serial);
dbus_uint32_t dbus_message_get_reply_serial (DBusMessage   *message);

void          dbus_message_set_auto_activation (DBusMessage   *message,
						dbus_bool_t    auto_activation);
dbus_bool_t   dbus_message_get_auto_activation (DBusMessage   *message);

dbus_bool_t   dbus_message_get_path_decomposed (DBusMessage   *message,
                                                char        ***path);

dbus_bool_t dbus_message_append_args          (DBusMessage     *message,
					       int              first_arg_type,
					       ...);
dbus_bool_t dbus_message_append_args_valist   (DBusMessage     *message,
					       int              first_arg_type,
					       va_list          var_args);
dbus_bool_t dbus_message_get_args             (DBusMessage     *message,
					       DBusError       *error,
					       int              first_arg_type,
					       ...);
dbus_bool_t dbus_message_get_args_valist      (DBusMessage     *message,
					       DBusError       *error,
					       int              first_arg_type,
					       va_list          var_args);
dbus_bool_t dbus_message_iter_get_args        (DBusMessageIter *iter,
					       DBusError       *error,
					       int              first_arg_type,
					       ...);
dbus_bool_t dbus_message_iter_get_args_valist (DBusMessageIter *iter,
					       DBusError       *error,
					       int              first_arg_type,
					       va_list          var_args);


dbus_bool_t            dbus_message_iter_init            (DBusMessage      *message,
                                                          DBusMessageIter  *iter);
dbus_bool_t            dbus_message_iter_has_next        (DBusMessageIter  *iter);
dbus_bool_t            dbus_message_iter_next            (DBusMessageIter  *iter);
int                    dbus_message_iter_get_arg_type    (DBusMessageIter  *iter);
int                    dbus_message_iter_get_array_type  (DBusMessageIter  *iter);
unsigned char          dbus_message_iter_get_byte        (DBusMessageIter  *iter);
dbus_bool_t            dbus_message_iter_get_boolean     (DBusMessageIter  *iter);
dbus_int32_t           dbus_message_iter_get_int32       (DBusMessageIter  *iter);
dbus_uint32_t          dbus_message_iter_get_uint32      (DBusMessageIter  *iter);
#ifdef DBUS_HAVE_INT64
dbus_int64_t           dbus_message_iter_get_int64       (DBusMessageIter  *iter);
dbus_uint64_t          dbus_message_iter_get_uint64      (DBusMessageIter  *iter);
#endif /* DBUS_HAVE_INT64 */
double                 dbus_message_iter_get_double      (DBusMessageIter  *iter);
char *                 dbus_message_iter_get_string      (DBusMessageIter  *iter);
char *                 dbus_message_iter_get_object_path (DBusMessageIter  *iter);
char *                 dbus_message_iter_get_dict_key    (DBusMessageIter  *iter);
dbus_bool_t            dbus_message_iter_get_custom      (DBusMessageIter  *iter,
                                                          char            **name,
                                                          unsigned char   **value,
                                                          int              *len);

dbus_bool_t dbus_message_iter_init_array_iterator (DBusMessageIter   *iter,
						   DBusMessageIter   *array_iter,
						   int               *array_type);
dbus_bool_t dbus_message_iter_init_dict_iterator  (DBusMessageIter   *iter,
						   DBusMessageIter   *dict_iter);
dbus_bool_t dbus_message_iter_get_byte_array      (DBusMessageIter   *iter,
						   unsigned char    **value,
						   int               *len);
dbus_bool_t dbus_message_iter_get_boolean_array   (DBusMessageIter   *iter,
						   unsigned char    **value,
						   int               *len);
dbus_bool_t dbus_message_iter_get_int32_array     (DBusMessageIter   *iter,
						   dbus_int32_t     **value,
						   int               *len);
dbus_bool_t dbus_message_iter_get_uint32_array    (DBusMessageIter   *iter,
						   dbus_uint32_t    **value,
						   int               *len);
#ifdef DBUS_HAVE_INT64
dbus_bool_t dbus_message_iter_get_int64_array     (DBusMessageIter   *iter,
						   dbus_int64_t     **value,
						   int               *len);
dbus_bool_t dbus_message_iter_get_uint64_array    (DBusMessageIter   *iter,
						   dbus_uint64_t    **value,
						   int               *len);
#endif /* DBUS_HAVE_INT64 */
dbus_bool_t dbus_message_iter_get_double_array    (DBusMessageIter   *iter,
						   double           **value,
						   int               *len);
dbus_bool_t dbus_message_iter_get_string_array    (DBusMessageIter   *iter,
						   char            ***value,
						   int               *len);
dbus_bool_t dbus_message_iter_get_object_path_array (DBusMessageIter   *iter,
                                                     char            ***value,
                                                     int               *len);


void        dbus_message_append_iter_init          (DBusMessage          *message,
						    DBusMessageIter      *iter);
dbus_bool_t dbus_message_iter_append_nil           (DBusMessageIter      *iter);
dbus_bool_t dbus_message_iter_append_boolean       (DBusMessageIter      *iter,
						    dbus_bool_t           value);
dbus_bool_t dbus_message_iter_append_byte          (DBusMessageIter      *iter,
						    unsigned char         value);
dbus_bool_t dbus_message_iter_append_int32         (DBusMessageIter      *iter,
						    dbus_int32_t          value);
dbus_bool_t dbus_message_iter_append_uint32        (DBusMessageIter      *iter,
						    dbus_uint32_t         value);
#ifdef DBUS_HAVE_INT64
dbus_bool_t dbus_message_iter_append_int64         (DBusMessageIter      *iter,
						    dbus_int64_t          value);
dbus_bool_t dbus_message_iter_append_uint64        (DBusMessageIter      *iter,
						    dbus_uint64_t         value);
#endif /* DBUS_HAVE_INT64 */
dbus_bool_t dbus_message_iter_append_double        (DBusMessageIter      *iter,
						    double                value);
dbus_bool_t dbus_message_iter_append_string        (DBusMessageIter      *iter,
						    const char           *value);
dbus_bool_t dbus_message_iter_append_object_path   (DBusMessageIter      *iter,
						    const char           *value);
dbus_bool_t dbus_message_iter_append_custom        (DBusMessageIter      *iter,
						    const char           *name,
						    const unsigned char  *data,
						    int                   len);
dbus_bool_t dbus_message_iter_append_dict_key      (DBusMessageIter      *iter,
						    const char           *value);
dbus_bool_t dbus_message_iter_append_array         (DBusMessageIter      *iter,
						    DBusMessageIter      *array_iter,
						    int                   element_type);
dbus_bool_t dbus_message_iter_append_dict          (DBusMessageIter      *iter,
						    DBusMessageIter      *dict_iter);

/* Helpers for normal types: */
dbus_bool_t dbus_message_iter_append_boolean_array (DBusMessageIter      *iter,
						    unsigned const char  *value,
						    int                   len);
dbus_bool_t dbus_message_iter_append_int32_array   (DBusMessageIter      *iter,
						    const dbus_int32_t   *value,
						    int                   len);
dbus_bool_t dbus_message_iter_append_uint32_array  (DBusMessageIter      *iter,
						    const dbus_uint32_t  *value,
						    int                   len);
#ifdef DBUS_HAVE_INT64
dbus_bool_t dbus_message_iter_append_int64_array   (DBusMessageIter      *iter,
						    const dbus_int64_t   *value,
						    int                   len);
dbus_bool_t dbus_message_iter_append_uint64_array  (DBusMessageIter      *iter,
						    const dbus_uint64_t  *value,
						    int                   len);
#endif /* DBUS_HAVE_INT64 */
dbus_bool_t dbus_message_iter_append_double_array  (DBusMessageIter      *iter,
						    const double         *value,
						    int                   len);
dbus_bool_t dbus_message_iter_append_byte_array    (DBusMessageIter      *iter,
						    unsigned const char  *value,
						    int                   len);
dbus_bool_t dbus_message_iter_append_string_array  (DBusMessageIter      *iter,
						    const char          **value,
						    int                   len);
dbus_bool_t dbus_message_iter_append_object_path_array (DBusMessageIter      *iter,
							const char          **value,
							int                   len);



dbus_bool_t  dbus_set_error_from_message  (DBusError    *error,
                                           DBusMessage  *message);


dbus_bool_t dbus_message_allocate_data_slot (dbus_int32_t     *slot_p);
void        dbus_message_free_data_slot     (dbus_int32_t     *slot_p);
dbus_bool_t dbus_message_set_data           (DBusMessage      *message,
                                             dbus_int32_t      slot,
                                             void             *data,
                                             DBusFreeFunction  free_data_func);
void*       dbus_message_get_data           (DBusMessage      *message,
                                             dbus_int32_t      slot);

int dbus_message_type_from_string (const char *type_str);

DBUS_END_DECLS;

#endif /* DBUS_MESSAGE_H */
