/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2013 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#ifndef MAILERS_H
#define MAILERS_H

#include <intuition/classusr.h>
#include <exec/lists.h>

struct URL_MailerNode;

Object * make_edit_mail_win(void);

BOOL updateMailerList( struct List * list, struct MinList PrefsMailerList );

void updateMailerWindow( struct URL_MailerNode  * pMailer );
void updateMailerNode( void );

#endif // MAILERS_H

