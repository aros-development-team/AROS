/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:41:42  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#ifndef UTILITY_INTERN_H
#define UTILITY_INTERN_H

/*
	I could use a table for this conversion, but the utility.library
	functions are patched by locale anyway, so this would be some
	kind of overkill.
*/
#define TOLOWER(a) \
(((a)>='A'&&(a)<='Z')||((a)>=0xc0&&(a)<=0xde&&(a)!=0xd7)?(a)+0x20:(a))

/* Needed for close() */
#define expunge() \
__AROS_LC0(BPTR, expunge, struct UtilityBase *, UtilityBase, 3, Utility)

#endif
