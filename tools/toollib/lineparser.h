#ifndef TOOLLIB_LINEPARSER_H
#define TOOLLIB_LINEPARSER_H

/*
    Copyright (C) 1995-1999 AROS - The Amiga Research OS
    $Id$

    Desc: Header for the simple line parser
*/

#ifndef TOOLLIB_TOOLLIB_H
#include <toollib/toollib.h>
#endif

extern char * get_line PARAMS((FILE *fd));
extern char *keyword PARAMS((char *line));
extern int get_words PARAMS((char *line, char ***outarray));

#endif /* TOOLLIB_LINEPARSER_H */
