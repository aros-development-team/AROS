/*
**	classbase.h - include file for Font DataType class
**	Copyright © 1995-96 Michael Letowski
*/

#ifndef CLASSBASE_H
#define CLASSBASE_H

#include <exec/exec.h>
#include <dos/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/datatypes.h>
#include <proto/diskfont.h>
#include <string.h>

// Type used to store a bool in the prefs
typedef IPTR PBOOL;

#define min(x,y) (((x) > (y))?(y):(x))
#define max(x,y) (((x) > (y))?(x):(y))
#define clamp(x,l,h) min(max(x,l),h)

#define ftst(x,y) ((x)&(y))
#define fset(x,y) (x) |= (y)

#define tswap(x,y,t) t = x, x = y, y = t
#define Color32(x) ((x)*0x01010101)

#define ThisProcessS ((struct Process *)FindTask(NULL))

#endif	/* CLASSBASE_H */
