#ifndef IA_LOG_H
#define IA_LOG_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* The install log. Opened on the first line written, flushed after every
   one, so that a failed or hung install still leaves a readable record. */

void LOG_Printf(CONST_STRPTR fmt, ...);
void LOG_Close(void);

#endif /* IA_LOG_H */
