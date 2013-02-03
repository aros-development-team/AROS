/*
 * Include file for the NewMouse standard way of handling wheeled mice.
 * 
 * Copyright (c) 1999 by Alessandro Zumo. All Rights Reserved.
*/

#ifndef NEWMOUSE_H
#define NEWMOUSE_H

#ifndef IECLASS_NEWMOUSE
#define IECLASS_NEWMOUSE	(0x16)  /* IECLASS_MAX + 1 as of V40 */ 
#endif

/* These are issued both under IECLASS_NEWMOUSE and IECLASS_RAWKEY 	*/ 
/* by the NewMouse driver											*/

#ifndef NM_WHEEL_UP
#define NM_WHEEL_UP			(0x7A)
#endif

#ifndef NM_WHEEL_DOWN
#define NM_WHEEL_DOWN		(0x7B)
#endif

#ifndef NM_WHEEL_LEFT
#define NM_WHEEL_LEFT		(0x7C)
#endif

#ifndef NM_WHEEL_RIGHT
#define NM_WHEEL_RIGHT		(0x7D)
#endif

#ifndef NM_BUTTON_FOURTH
#define NM_BUTTON_FOURTH	(0x7E)
#endif

#endif /* NEWMOUSE_H */
