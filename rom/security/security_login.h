/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

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

/* Graphical login (security_logingui.c, Zune LoginWindow.mcc) */
#define LOGINGUI_OK           (1)
#define LOGINGUI_CANCEL       (0)
#define LOGINGUI_UNAVAILABLE  (-1)     /* no MUI / no LoginWindow.mcc: use the console */
extern LONG LoginGUI(struct SecurityBase *secBase, CONST_STRPTR pubscreen, CONST_STRPTR prompt, BOOL cancelok,
                     STRPTR uid, ULONG uidsize, STRPTR pwd, ULONG pwdsize);
extern void LoginMessageGUI(struct SecurityBase *secBase, CONST_STRPTR title, CONST_STRPTR text, CONST_STRPTR gadgets);

/* Console helpers: read a line / a hidden password from a filehandle */
extern BOOL ReadLineCon(struct SecurityBase *secBase, BPTR input, STRPTR buf, ULONG size);
extern BOOL ReadPasswordCon(struct SecurityBase *secBase, BPTR input, BPTR output, STRPTR buf, ULONG size, struct LocaleInfo *li);

#endif /* _SECURITY_LOGIN_H */
