#ifndef _AROS_SYMBOLSETS_H
#define _AROS_SYMBOLSETS_H

/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Symbol sets support
    Lang: english
*/

#define ADD2SET(symbol, set, pri)\
	const int __aros_set_##pri##_##set##_element_##symbol;

#define ADD2INIT(symbol, pri)\
	ADD2SET(symbol, __INIT_LIST__, pri)

#define ADD2EXIT(symbol, pri)\
	ADD2SET(symbol, __EXIT_LIST__, pri)

#endif
