/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Function prototypes for functions to call from the query function
    Lang: English
*/

#include <exec/types.h>

LONG rxsupp_allocmem(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_baddr(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_closeport(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_delay(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_delete(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_forbid(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_freemem(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_getarg(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_getpkt(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_makedir(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_next(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_null(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_offset(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_openport(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_permit(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_rename(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_reply(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_showdir(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_showlist(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_statef(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_typepkt(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_waitpkt(struct Library *, struct RexxMsg *, UBYTE **);
