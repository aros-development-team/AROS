/*
    Copyright © 2003, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUIMASTER_SUPPORT_AROS_H_
#define _MUIMASTER_SUPPORT_AROS_H_

/*** OS4 VARARGS support ****************************************************/
#ifndef VARARGS68K
#define VARARGS68K
#endif

/*** OS4 Exec Interface support *********************************************/
#define EXEC_INTERFACE_DECLARE(x)
#define EXEC_INTERFACE_GET_MAIN(interface,libbase) 1
#define EXEC_INTERFACE_DROP(interface)
#define EXEC_INTERFACE_ASSIGN(a,b)

/*** Misc Functions *****************************************************/
LONG HexToIPTR(CONST_STRPTR s, IPTR *val);
LONG HexToLong(CONST_STRPTR s, ULONG *val);

#endif
