/*
    Copyright © 2003, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUIMASTER_SUPPORT_AROS_H_
#define _MUIMASTER_SUPPORT_AROS_H_

/*** OS4 Exec Interface support *********************************************/
#define EXEC_INTERFACE_DECLARE(x)
#define EXEC_INTERFACE_GET_MAIN(interface,libbase) 1
#define EXEC_INTERFACE_DROP(interface)
#define EXEC_INTERFACE_ASSIGN(a,b)

/*** Misc OS4 Functions *****************************************************/
LONG HexToLong(STRPTR s, ULONG *val);

#endif
