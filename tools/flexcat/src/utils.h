/*
 * $Id$
 *
 * Copyright (C) 1993-1999 by Jochen Wiedmann and Marcin Orlowski
 * Copyright (C) 2002-2010 by the FlexCat Open Source Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef  FLEXCAT_UTILS_H
#define  FLEXCAT_UTILS_H

/* Functions */
void            MyExit ( int Code );
char           *AllocString ( const char *str );
char           *AddString ( char *str, const char *addedstr );
char           *ConvertString ( char *str, const char *from_charset, const char *to_charset);
char           *AddCatalogChunk ( char *ID, const char *string );
int             gethex ( int c );
int             getoctal ( int c );
char           *ReadLine ( FILE * fp, int AllowComment );
void            OverSpace ( char **strptr );
void            Expunge ( void );
int             ReadChar ( char **strptr, char *dest );
char           *AllocFileName ( char *filename, int howto );
char           *AddFileName ( char *pathname, char *filename );
void            Usage ( void );

#if !defined(AMIGA)
int             Stricmp ( const char *str1, const char *str2 );
int             Strnicmp ( const char *str1, const char *str2, int len );
#endif // AMIGA

#if defined(__amigaos3__) || defined(__MORPHOS__)
char *strptime(const char *string, const char *fmt, struct tm *res);
#endif

#if defined(__amigaos4__)
#define GETINTERFACE(iface, base) (iface = (APTR)GetInterface((struct Library *)(base), "main", 1L, NULL))
#define DROPINTERFACE(iface)      (DropInterface((struct Interface *)iface), iface = NULL)
#else // __amigaos4__
#define GETINTERFACE(iface, base) TRUE
#define DROPINTERFACE(iface)      ((void)0)
#endif // __amigaos4__

#endif  /* FLEXCAT_UTILS_H */
