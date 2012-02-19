#ifndef ABOUTBOX_MCC_H
#define ABOUTBOX_MCC_H

/*
    Copyright © 2011, Thore Böckelmann. All rights reserved.
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/iffparse.h>

/*** Name *******************************************************************/
#define MUIC_Aboutbox  "Aboutbox.mcc"

/*** Protected Attributes ***************************************************/
#define MUIA_Aboutbox_Credits            0xFED10001ul    /* [I..] STRPTR  v20.1  */
#define MUIA_Aboutbox_LogoData           0xFED10002ul    /* [I..] APTR    v20.2  */
#define MUIA_Aboutbox_LogoFallbackMode   0xFED10003ul    /* [I..] ULONG   v20.2  */
#define MUIA_Aboutbox_LogoFile           0xFED10004ul    /* [I..] STRPTR  v20.2  */
#define MUIA_Aboutbox_Build              0xFED1001Eul    /* [I..] STRPTR  v20.12 */

/*** Macros *****************************************************************/
#define AboutboxObject MUIOBJMACRO_START(MUIC_Aboutbox)


/*** Special Values *********************************************************/

/*
 *   the fallback mode defines in which order Aboutbox.mcc tries to get valid image
 *   data for the logo:
 *
 *   D = PROGDIR:<executablefilename>.info
 *   E = file specified in MUIA_Aboutbox_LogoFile
 *   I = data specified with MUIA_Aboutbox_LogoData
 */
#define MUIV_Aboutbox_LogoFallbackMode_NoLogo     0
#define MUIV_Aboutbox_LogoFallbackMode_Auto       MAKE_ID('D' , 'E' , 'I' , '\0')

#endif /* ABOUTBOX_MCC_H */
