/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus-protocol.h  D-Bus protocol constants
 *
 * Copyright (C) 2002, 2003  CodeFactory AB
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

#ifndef DBUS_PROTOCOL_H
#define DBUS_PROTOCOL_H

/* Don't include anything in here from anywhere else. It's
 * intended for use by any random library.
 */

#ifdef  __cplusplus
extern "C" {
#endif

/* Message byte order */
#define DBUS_LITTLE_ENDIAN ('l')  /* LSB first */
#define DBUS_BIG_ENDIAN    ('B')  /* MSB first */    

/* Protocol version */
#define DBUS_MAJOR_PROTOCOL_VERSION 0

/* Data types */
#define DBUS_TYPE_INVALID       ((int) '\0')
#define DBUS_TYPE_NIL           ((int) 'v')
#define DBUS_TYPE_BYTE          ((int) 'y')
#define DBUS_TYPE_BOOLEAN       ((int) 'b')
#define DBUS_TYPE_INT32         ((int) 'i')
#define DBUS_TYPE_UINT32        ((int) 'u')
#define DBUS_TYPE_INT64         ((int) 'x')
#define DBUS_TYPE_UINT64        ((int) 't')
#define DBUS_TYPE_DOUBLE        ((int) 'd')
#define DBUS_TYPE_STRING        ((int) 's')
#define DBUS_TYPE_CUSTOM        ((int) 'c')
#define DBUS_TYPE_ARRAY         ((int) 'a')
#define DBUS_TYPE_DICT          ((int) 'm')
#define DBUS_TYPE_OBJECT_PATH   ((int) 'o')

#define DBUS_NUMBER_OF_TYPES    (13)

/* Max length in bytes of a service or interface or member name */
#define DBUS_MAXIMUM_NAME_LENGTH 256

/* Max length of a match rule string */
#define DBUS_MAXIMUM_MATCH_RULE_LENGTH 1024

/* Types of message */
#define DBUS_MESSAGE_TYPE_INVALID       0
#define DBUS_MESSAGE_TYPE_METHOD_CALL   1
#define DBUS_MESSAGE_TYPE_METHOD_RETURN 2
#define DBUS_MESSAGE_TYPE_ERROR         3
#define DBUS_MESSAGE_TYPE_SIGNAL        4
  
/* Header flags */
#define DBUS_HEADER_FLAG_NO_REPLY_EXPECTED 0x1
#define DBUS_HEADER_FLAG_AUTO_ACTIVATION   0x2
  
/* Header fields */
#define DBUS_HEADER_FIELD_INVALID        0
#define DBUS_HEADER_FIELD_PATH           1
#define DBUS_HEADER_FIELD_INTERFACE      2
#define DBUS_HEADER_FIELD_MEMBER         3
#define DBUS_HEADER_FIELD_ERROR_NAME     4
#define DBUS_HEADER_FIELD_REPLY_SERIAL   5
#define DBUS_HEADER_FIELD_DESTINATION    6
#define DBUS_HEADER_FIELD_SENDER         7
#define DBUS_HEADER_FIELD_SIGNATURE      8

#define DBUS_HEADER_FIELD_LAST DBUS_HEADER_FIELD_SIGNATURE

/* Services */
#define DBUS_SERVICE_ORG_FREEDESKTOP_DBUS      "org.freedesktop.DBus"

/* Paths */
#define DBUS_PATH_ORG_FREEDESKTOP_DBUS  "/org/freedesktop/DBus"
#define DBUS_PATH_ORG_FREEDESKTOP_LOCAL "/org/freedesktop/Local"
  
/* Interfaces, these #define don't do much other than
 * catch typos at compile time
 */
#define DBUS_INTERFACE_ORG_FREEDESKTOP_DBUS  "org.freedesktop.DBus"
#define DBUS_INTERFACE_ORG_FREEDESKTOP_INTROSPECTABLE "org.freedesktop.Introspectable"
  
/* This is a special interface whose methods can only be invoked
 * by the local implementation (messages from remote apps aren't
 * allowed to specify this interface).
 */
#define DBUS_INTERFACE_ORG_FREEDESKTOP_LOCAL "org.freedesktop.Local"
  
/* Service owner flags */
#define DBUS_SERVICE_FLAG_PROHIBIT_REPLACEMENT 0x1
#define DBUS_SERVICE_FLAG_REPLACE_EXISTING     0x2

/* Service replies */
#define DBUS_SERVICE_REPLY_PRIMARY_OWNER  0x1
#define DBUS_SERVICE_REPLY_IN_QUEUE       0x2
#define DBUS_SERVICE_REPLY_SERVICE_EXISTS 0x4
#define DBUS_SERVICE_REPLY_ALREADY_OWNER  0x8

/* Activation replies */
#define DBUS_ACTIVATION_REPLY_ACTIVATED      0x0
#define DBUS_ACTIVATION_REPLY_ALREADY_ACTIVE 0x1

/* Errors */
/* WARNING these get autoconverted to an enum in dbus-glib.h. Thus,
 * if you change the order it breaks the ABI. Keep them in order.
 * Also, don't change the formatting since that will break the sed
 * script.
 */
#define DBUS_ERROR_FAILED                     "org.freedesktop.DBus.Error.Failed"
#define DBUS_ERROR_NO_MEMORY                  "org.freedesktop.DBus.Error.NoMemory"
#define DBUS_ERROR_ACTIVATE_SERVICE_NOT_FOUND "org.freedesktop.DBus.Error.ServiceNotFound"
#define DBUS_ERROR_SERVICE_DOES_NOT_EXIST     "org.freedesktop.DBus.Error.ServiceDoesNotExist"
#define DBUS_ERROR_SERVICE_HAS_NO_OWNER       "org.freedesktop.DBus.Error.ServiceHasNoOwner"
#define DBUS_ERROR_NO_REPLY                   "org.freedesktop.DBus.Error.NoReply"
#define DBUS_ERROR_IO_ERROR                   "org.freedesktop.DBus.Error.IOError"
#define DBUS_ERROR_BAD_ADDRESS                "org.freedesktop.DBus.Error.BadAddress"
#define DBUS_ERROR_NOT_SUPPORTED              "org.freedesktop.DBus.Error.NotSupported"
#define DBUS_ERROR_LIMITS_EXCEEDED            "org.freedesktop.DBus.Error.LimitsExceeded"
#define DBUS_ERROR_ACCESS_DENIED              "org.freedesktop.DBus.Error.AccessDenied"
#define DBUS_ERROR_AUTH_FAILED                "org.freedesktop.DBus.Error.AuthFailed"
#define DBUS_ERROR_NO_SERVER                  "org.freedesktop.DBus.Error.NoServer"
#define DBUS_ERROR_TIMEOUT                    "org.freedesktop.DBus.Error.Timeout"
#define DBUS_ERROR_NO_NETWORK                 "org.freedesktop.DBus.Error.NoNetwork"
#define DBUS_ERROR_ADDRESS_IN_USE             "org.freedesktop.DBus.Error.AddressInUse"
#define DBUS_ERROR_DISCONNECTED               "org.freedesktop.DBus.Error.Disconnected"
#define DBUS_ERROR_INVALID_ARGS               "org.freedesktop.DBus.Error.InvalidArgs"
#define DBUS_ERROR_FILE_NOT_FOUND             "org.freedesktop.DBus.Error.FileNotFound"
#define DBUS_ERROR_UNKNOWN_METHOD             "org.freedesktop.DBus.Error.UnknownMethod"
#define DBUS_ERROR_TIMED_OUT                  "org.freedesktop.DBus.Error.TimedOut"
#define DBUS_ERROR_MATCH_RULE_NOT_FOUND       "org.freedesktop.DBus.Error.MatchRuleNotFound"
#define DBUS_ERROR_MATCH_RULE_INVALID         "org.freedesktop.DBus.Error.MatchRuleInvalid"
#define DBUS_ERROR_SPAWN_EXEC_FAILED          "org.freedesktop.DBus.Error.Spawn.ExecFailed"
#define DBUS_ERROR_SPAWN_FORK_FAILED          "org.freedesktop.DBus.Error.Spawn.ForkFailed"
#define DBUS_ERROR_SPAWN_CHILD_EXITED         "org.freedesktop.DBus.Error.Spawn.ChildExited"
#define DBUS_ERROR_SPAWN_CHILD_SIGNALED       "org.freedesktop.DBus.Error.Spawn.ChildSignaled"
#define DBUS_ERROR_SPAWN_FAILED               "org.freedesktop.DBus.Error.Spawn.Failed"
#define DBUS_ERROR_UNIX_PROCESS_ID_UNKNOWN    "org.freedesktop.DBus.Error.UnixProcessIdUnknown"

#ifdef __cplusplus
}
#endif

#endif /* DBUS_PROTOCOL_H */
