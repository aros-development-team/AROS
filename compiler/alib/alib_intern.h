#ifndef _ALIB_INTERN_H
#define _ALIB_INTERN_H
/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#ifndef AROS_SYSTEM_H
#   include <aros/system.h>
#endif
#ifdef AROS_SLOWSTACKTAGS
#   include <stdarg.h>
#   ifndef UTILITY_TAGITEM_H
#	include <utility/tagitem.h>
#   endif
#endif
#ifdef AROS_SLOWSTACKMETHODS
#   ifndef AROS_SLOWSTACKTAGS
#	include <stdarg.h>
#   endif
#endif

ULONG CallHookPkt (struct Hook * hook, APTR object, APTR paramPacket);
#ifndef AROS_SLOWCALLHOOKPKT
#   define CallHookPkt(h,o,p)   ((*(((struct Hook *)(h))->h_Entry)) (((struct Hook *)(h)), ((APTR)(o)), ((APTR)(p))))
#endif

#ifdef AROS_SLOWSTACKMETHODS
    Msg  GetMsgFromStack  (ULONG MethodID, va_list args);
    void FreeMsgFromStack (Msg msg);
#endif /* AROS_SLOWSTACKMETHODS */

#ifdef AROS_SLOWSTACKTAGS
    struct TagItem * GetTagsFromStack  (ULONG firstTag, va_list args);
    void	     FreeTagsFromStack (struct TagItem * tags);
#endif /* AROS_SLOWSTACKTAGS */

#endif /* _ALIB_INTERN_H */
