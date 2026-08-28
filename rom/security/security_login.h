/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: security.library login support
*/
#ifndef _SECURITY_LOGIN_H
#define _SECURITY_LOGIN_H

#include <exec/types.h>
#include <dos/dos.h>
#include <utility/tagitem.h>

struct SecurityBase;
struct LocaleInfo;

/* Parsed secT_#? tags */
struct secTags
{
    BPTR                Input;
    BPTR                Output;
    STRPTR              PubScrName;
    struct Task         *Task;
    STRPTR              UserID;
    STRPTR              Password;
    ULONG               DefProtection;
    BOOL                Graphical;
    BOOL                Own;
    BOOL                Global;
    BOOL                Quiet;
    BOOL                All;
    BOOL                NoLog;
};

extern BOOL InterpretTagList(struct SecurityBase *secBase, struct TagItem *taglist, struct secTags *tags);

/* Console helpers: read a line / a hidden password from a filehandle */
extern BOOL ReadLineCon(struct SecurityBase *secBase, BPTR input, STRPTR buf, ULONG size);
extern BOOL ReadPasswordCon(struct SecurityBase *secBase, BPTR input, BPTR output, STRPTR buf, ULONG size, struct LocaleInfo *li);

#endif /* _SECURITY_LOGIN_H */
