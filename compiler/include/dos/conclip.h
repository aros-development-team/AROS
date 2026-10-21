/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#ifndef DOS_CONCLIP_H
#define DOS_CONCLIP_H
/* Optional ConClip text snapshot protocol. The initial fields match the legacy
 * edit-hook message; old servers reply without modifying status. */
#include <exec/ports.h>
struct SGWork;
#define CONCLIP_READ_TEXT 0x100
#define CONCLIP_WRITE_TEXT 0x101
#define CONCLIP_WRITE_MAX (1024UL * 1024UL)
#define CONCLIP_TEXT_VERSION 1
#define CONCLIP_TEXT_UNSUPPORTED 0
#define CONCLIP_TEXT_OK 1
#define CONCLIP_TEXT_ERROR 2
struct ConClipTextMessage {
    struct Message msg;
    struct SGWork *sgw; /* extended requests: unused, must be NULL */
    WORD code;
    ULONG version, status;
    UBYTE *data;
    ULONG length, capacity;
};
/* READ: Concatenates all FTXT/CHRS chunks in stream order. A malformed/short
 * later chunk fails the whole snapshot without transferring a partial prefix.
 * On OK, data belongs to the caller: FreeMem(data, capacity). Capacity includes
 * twelve spare bytes for terminal framing. ERROR never transfers ownership. */
/* WRITE: caller retains data until reply; length <= capacity and WRITE_MAX.
 * Zero length writes an empty FTXT/CHRS. Status OK means all chunk writes and
 * pops succeeded; ERROR may have modified clipboard contents (no rollback).
 * Old servers reply UNSUPPORTED. Existing struct layout/version are unchanged. */
#endif
