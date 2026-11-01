#ifndef DOS_PIPE_H
#define DOS_PIPE_H

/* Readiness extensions Copyright (C) 2026 John KNIPPER <code@jkn.me>.
 * Pipe nonblocking protocol adapted for POSIXC and both IO directions.
 * SPDX-License-Identifier: APL-1.1
 * Compatible with pomaros/aros-core 89db046157d415e98c05ca18316ca7ae63b07023.
 * DoPkt(handler, action, fh_Arg1, enable, 0, 0, 0) returns 1 on success (0 on failure).
 * An older handler returns ERROR_ACTION_NOT_KNOWN. Nonblocking reads/writes
 * return available bytes immediately, zero for read EOF, or -1/PWOB when
 * no progress is possible. Existing blocking handles retain their behavior.
 */
#define PIPE_ATOMIC_WRITE       512 /* matches POSIXC PIPE_BUF */
#define ACTION_PIPE_SET_NONBLOCK 0x50534e42L
#define ERROR_PIPE_WOULD_BLOCK   0x50574f42L

#endif
