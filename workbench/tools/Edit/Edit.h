/******************************************************
** Events.h : Standard datatypes and prototypes      **
** public port. Written by T.Pierron and C.Guillaume.**
** Free software under terms of GNU license.         **
******************************************************/


#ifndef	EDIT_H
#define	EDIT_H

void del_block(Project p);
void indent_by(Project p, UBYTE ch, BYTE method);
void change_case(Project p, UBYTE method);
void mark_all(Project p);
LONG copy_mark_to_buf(Project p, UBYTE *Buf, LONG Max);

#endif
