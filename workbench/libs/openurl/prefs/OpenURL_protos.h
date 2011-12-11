/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2009 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#include <SDI/SDI_compiler.h>

#ifdef __MORPHOS__
APTR DoSuperNew(struct IClass *cl, APTR obj, ...);
#elif defined(__AROS__)
#define DoSuperNew(cl, obj, ...)				\
({								\
    IPTR __args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
    DoSuperNewTagList(cl, obj, NULL, (struct TagItem *)__args);	\
})
#else
Object * VARARGS68K DoSuperNew(struct IClass *cl, Object *obj, ...);
#endif

/* loc.c */
void initStrings ( void );
void uninitStrings( void );
STRPTR getString ( ULONG id );
void localizeStrings ( STRPTR *s );
void localizeNewMenu ( struct NewMenu *nm );
ULONG getKeyChar (STRPTR string , ULONG id);

/* utils.c */
#if defined(__amigaos4__)
void SetAmiUpdateENVVariable( CONST_STRPTR varname );
#endif
Object *olabel ( ULONG id );
Object *ollabel ( ULONG id );
Object *ollabel1 ( ULONG id );
Object *olabel1 ( ULONG id );
Object *olabel2 ( ULONG id );
Object *oflabel ( ULONG text );
Object *obutton ( ULONG text , ULONG help );
Object *oibutton ( ULONG spec , ULONG help );
Object *otbutton ( ULONG label , ULONG help );
Object *ocheckmark ( ULONG key , ULONG help );
Object *opopbutton ( ULONG img , ULONG help );
Object *ostring ( ULONG maxlen , ULONG key , ULONG help );
Object *opopport ( ULONG maxLen , ULONG key , ULONG help );
Object *opopph ( CONST_STRPTR *syms , STRPTR *names , ULONG maxLen , ULONG key , ULONG asl , ULONG help );
ULONG openWindow ( Object *app , Object *win );
IPTR delEntry ( Object *obj , APTR entry );
void STDARGS msprintf ( STRPTR to , STRPTR fmt , ... ) VARARGS68K;
int STDARGS msnprintf ( STRPTR buf , int size , STRPTR fmt , ... ) VARARGS68K;

/* ftpeditwin.c */
BOOL initFTPEditWinClass ( void );
void disposeFTPEditWinClass ( void );

/* mailereditwin.c */
BOOL initMailerEditWinClass ( void );
void disposeMailerEditWinClass ( void );

/* browsereditwin.c */
BOOL initBrowserEditWinClass ( void );
void disposeBrowserEditWinClass ( void );

/* applist.c */
BOOL initAppListClass ( void );
void disposeAppListClass ( void );

/* win.c */
BOOL initWinClass ( void );
void disposeWinClass ( void );

/* about.c */
BOOL initAboutClass ( void );
void disposeAboutClass ( void );

/* app.c */
BOOL initAppClass ( void );
void disposeAppClass ( void );

/* popport.c */
BOOL initPopportClass ( void );
void disposePopportClass ( void );

/* popph.c */
BOOL initPopphClass ( void );
void disposePopphClass ( void );
