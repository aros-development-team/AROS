/*
 * $Id$
 *
 * :ts=8
 *
 * SMB file system wrapper for AmigaOS, using the AmiTCP V3 API
 *
 * Copyright (C) 2000-2009 by Olaf `Olsen' Barthel <obarthel -at- gmx -dot- net>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _ASSERT_H
#define _ASSERT_H 1

/****************************************************************************/

/* IMPORTANT: If DEBUG is redefined, it must happen only here. This
 *            will cause all modules to depend upon it to be rebuilt
 *            by the smakefile (that is, provided the smakefile has
 *            all the necessary dependency lines in place).
 */

/*#define DEBUG*/

/****************************************************************************/

#ifdef ASSERT
#undef ASSERT
#endif	/* ASSERT */

#define PUSH_ASSERTS()	PUSHDEBUGLEVEL(0)
#define PUSH_REPORTS()	PUSHDEBUGLEVEL(1)
#define PUSH_CALLS()	PUSHDEBUGLEVEL(2)
#define PUSH_ALL()	PUSHDEBUGLEVEL(2)
#define POP()		POPDEBUGLEVEL()

#if defined(DEBUG) /*&& defined(__SASC)*/
 void _ASSERT(int x,const char *xs,const char *file,int line,const char *function);
 void _SHOWVALUE(unsigned long value,int size,const char *name,const char *file,int line);
 void _SHOWPOINTER(void *p,const char *name,const char *file,int line);
 void _SHOWSTRING(const char *string,const char *name,const char *file,int line);
 void _SHOWMSG(const char *msg,const char *file,int line);
 void _ENTER(const char *file,int line,const char *function);
 void _LEAVE(const char *file,int line,const char *function);
 void _RETURN(const char *file,int line,const char *function,unsigned long result);
 void _DPRINTF_HEADER(const char *file,int line);
 void _DPRINTF(const char *format,...);
 void _DLOG(const char *format,...);
 int  _SETDEBUGLEVEL(int level);
 void _PUSHDEBUGLEVEL(int level);
 void _POPDEBUGLEVEL(void);
 int  _GETDEBUGLEVEL(void);
 void _SETPROGRAMNAME(char *name);
 void _INDENT(void);

 #define ASSERT(x)		_ASSERT((int)(x),#x,__FILE__,__LINE__,__FUNC__)
 #define ENTER()		_ENTER(__FILE__,__LINE__,__FUNC__)
 #define LEAVE()		_LEAVE(__FILE__,__LINE__,__FUNC__)
 #define RETURN(r)		_RETURN(__FILE__,__LINE__,__FUNC__,(unsigned long)r)
 #define SHOWVALUE(v)		_SHOWVALUE((ULONG)v,sizeof(v),#v,__FILE__,__LINE__)
 #define SHOWPOINTER(p)		_SHOWPOINTER(p,#p,__FILE__,__LINE__)
 #define SHOWSTRING(s)		_SHOWSTRING(s,#s,__FILE__,__LINE__)
 #define SHOWMSG(s)		_SHOWMSG(s,__FILE__,__LINE__)
 #define D(s)			do { _DPRINTF_HEADER(__FILE__,__LINE__); _DPRINTF s; } while(0)
 #define PRINTHEADER()		_DPRINTF_HEADER(__FILE__,__LINE__)
 #define PRINTF(s)		_DLOG s
 #define LOG(s)			do { _DPRINTF_HEADER(__FILE__,__LINE__); _DLOG("<%s()>:",__FUNC__); _DLOG s; } while(0)
 #define SETDEBUGLEVEL(l)	_SETDEBUGLEVEL(l)
 #define PUSHDEBUGLEVEL(l)	_PUSHDEBUGLEVEL(l)
 #define POPDEBUGLEVEL()	_POPDEBUGLEVEL()
 #define SETPROGRAMNAME(n)	_SETPROGRAMNAME(n)
 #define GETDEBUGLEVEL()	_GETDEBUGLEVEL()
 #define INDENT()		_INDENT()

 extern int __debug_level;

 #undef DEBUG
 #define DEBUG 1
#else
 #define ASSERT(x)		((void)0)
 #define ENTER()		((void)0)
 #define LEAVE()		((void)0)
 #define RETURN(r)		((void)0)
 #define SHOWVALUE(v)		((void)0)
 #define SHOWPOINTER(p)		((void)0)
 #define SHOWSTRING(s)		((void)0)
 #define SHOWMSG(s)		((void)0)
 #define D(s)			((void)0)
 #define PRINTHEADER()		((void)0)
 #define PRINTF(s)		((void)0)
 #define LOG(s)			((void)0)
 #define SETDEBUGLEVEL(l)	((void)0)
 #define PUSHDEBUGLEVEL(l)	((void)0)
 #define POPDEBUGLEVEL()	((void)0)
 #define SETPROGRAMNAME(n)	((void)0)
 #define GETDEBUGLEVEL()	(0)
 #define INDENT()		((void)0)

 #ifdef DEBUG
 #undef DEBUG
 #endif /* DEBUG */

 #define DEBUG 0
#endif /* DEBUG */

/****************************************************************************/

#endif /* _ASSERT_H */
