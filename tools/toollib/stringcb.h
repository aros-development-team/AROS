#ifndef TOOLLIB_STRINGCB_H
#define TOOLLIB_STRINGCB_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef TOOLLIB_TOOLLIB_H
#   include <toollib/toollib.h>
#endif
#ifndef TOOLLIB_MYSTREAM_H
#   include <toollib/mystream.h>
#endif
#ifndef TOOLLIB_VSTRING_H
#   include <toollib/vstring.h>
#endif

typedef struct
{
    MyStream	 stream;
    const char * string;
    String	 out;
    int 	 pos,
		 max,
		 line;
}
StringStream;

extern StringStream * StrStr_New     PARAMS ((const char * string));
extern void	      StrStr_Delete  PARAMS ((StringStream *));

#endif /* TOOLLIB_STRINGCB_H */
