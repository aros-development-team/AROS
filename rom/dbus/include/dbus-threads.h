/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus-threads.h  D-BUS threads handling
 *
 * Copyright (C) 2002  Red Hat Inc.
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

#ifndef DBUS_THREADS_H
#define DBUS_THREADS_H

#include <dbus/dbus-macros.h>
#include <dbus/dbus-types.h>

DBUS_BEGIN_DECLS;

typedef struct DBusMutex DBusMutex;
typedef struct DBusCondVar DBusCondVar;

typedef DBusMutex*  (* DBusMutexNewFunction)    (void);
typedef void        (* DBusMutexFreeFunction)   (DBusMutex *mutex);
typedef dbus_bool_t (* DBusMutexLockFunction)   (DBusMutex *mutex);
typedef dbus_bool_t (* DBusMutexUnlockFunction) (DBusMutex *mutex);

typedef DBusCondVar*  (* DBusCondVarNewFunction)         (void);
typedef void          (* DBusCondVarFreeFunction)        (DBusCondVar *cond);
typedef void          (* DBusCondVarWaitFunction)        (DBusCondVar *cond,
							  DBusMutex   *mutex);
typedef dbus_bool_t   (* DBusCondVarWaitTimeoutFunction) (DBusCondVar *cond,
							  DBusMutex   *mutex,
							  int          timeout_milliseconds);
typedef void          (* DBusCondVarWakeOneFunction) (DBusCondVar *cond);
typedef void          (* DBusCondVarWakeAllFunction) (DBusCondVar *cond);

typedef enum 
{
  DBUS_THREAD_FUNCTIONS_MUTEX_NEW_MASK      = 1 << 0,
  DBUS_THREAD_FUNCTIONS_MUTEX_FREE_MASK     = 1 << 1,
  DBUS_THREAD_FUNCTIONS_MUTEX_LOCK_MASK     = 1 << 2,
  DBUS_THREAD_FUNCTIONS_MUTEX_UNLOCK_MASK   = 1 << 3,
  DBUS_THREAD_FUNCTIONS_CONDVAR_NEW_MASK    = 1 << 4,
  DBUS_THREAD_FUNCTIONS_CONDVAR_FREE_MASK   = 1 << 5,
  DBUS_THREAD_FUNCTIONS_CONDVAR_WAIT_MASK   = 1 << 6,
  DBUS_THREAD_FUNCTIONS_CONDVAR_WAIT_TIMEOUT_MASK   = 1 << 7,
  DBUS_THREAD_FUNCTIONS_CONDVAR_WAKE_ONE_MASK = 1 << 8,
  DBUS_THREAD_FUNCTIONS_CONDVAR_WAKE_ALL_MASK = 1 << 9,

  DBUS_THREAD_FUNCTIONS_ALL_MASK     = (1 << 10) - 1
} DBusThreadFunctionsMask;

/**
 * Functions that must be implemented to make the D-BUS
 * library thread-aware. 
 */
typedef struct
{
  unsigned int mask; /**< Mask indicating which functions are present. */

  DBusMutexNewFunction mutex_new; /**< Function to create a mutex */
  DBusMutexFreeFunction mutex_free; /**< Function to free a mutex */
  DBusMutexLockFunction mutex_lock; /**< Function to lock a mutex */
  DBusMutexUnlockFunction mutex_unlock; /**< Function to unlock a mutex */

  DBusCondVarNewFunction condvar_new; /**< Function to create a condition variable */
  DBusCondVarFreeFunction condvar_free; /**< Function to free a condition variable */
  DBusCondVarWaitFunction condvar_wait; /**< Function to wait on a condition */
  DBusCondVarWaitTimeoutFunction condvar_wait_timeout; /**< Function to wait on a condition with a timeout */
  DBusCondVarWakeOneFunction condvar_wake_one; /**< Function to wake one thread waiting on the condition */
  DBusCondVarWakeAllFunction condvar_wake_all; /**< Function to wake all threads waiting on the condition */
  
  void (* padding1) (void); /**< Reserved for future expansion */
  void (* padding2) (void); /**< Reserved for future expansion */
  void (* padding3) (void); /**< Reserved for future expansion */
  void (* padding4) (void); /**< Reserved for future expansion */
  void (* padding5) (void); /**< Reserved for future expansion */
  void (* padding6) (void); /**< Reserved for future expansion */
  void (* padding7) (void); /**< Reserved for future expansion */
  void (* padding8) (void); /**< Reserved for future expansion */
  
} DBusThreadFunctions;


DBusMutex*   dbus_mutex_new            (void);
void         dbus_mutex_free           (DBusMutex                 *mutex);
dbus_bool_t  dbus_mutex_lock           (DBusMutex                 *mutex);
dbus_bool_t  dbus_mutex_unlock         (DBusMutex                 *mutex);

DBusCondVar* dbus_condvar_new          (void);
void         dbus_condvar_free         (DBusCondVar               *cond);
void         dbus_condvar_wait         (DBusCondVar               *cond,
					DBusMutex                 *mutex);
dbus_bool_t  dbus_condvar_wait_timeout (DBusCondVar               *cond,
					DBusMutex                 *mutex,
					int                        timeout_milliseconds);
void         dbus_condvar_wake_one     (DBusCondVar               *cond);
void         dbus_condvar_wake_all     (DBusCondVar               *cond);

dbus_bool_t  dbus_threads_init         (const DBusThreadFunctions *functions);



DBUS_END_DECLS;

#endif /* DBUS_THREADS_H */
