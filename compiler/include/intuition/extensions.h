#ifndef INTUITION_EXTENSIONS_H
#define INTUITION_EXTENSIONS_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Defines for WindowAction() *********************************************/
/*= Commands ===============================================================*/
#define WAC_BASE                 (0x0001)
#define WAC_HIDEWINDOW           (WAC_BASE + 0)
#define WAC_SHOWWINDOW           (WAC_BASE + 1)
#define WAC_SENDIDCMPCLOSE       (WAC_BASE + 2)
#define WAC_MOVEWINDOW           (WAC_BASE + 3)
#define WAC_SIZEWINDOW           (WAC_BASE + 4)
#define WAC_CHANGEWINDOWBOX      (WAC_BASE + 5)
#define WAC_WINDOWTOFRONT        (WAC_BASE + 6)
#define WAC_WINDOWTOBACK         (WAC_BASE + 7)
#define WAC_ZIPWINDOW            (WAC_BASE + 8)
#define WAC_MOVEWINDOWINFRONTOF  (WAC_BASE + 9)
#define WAC_ACTIVATEWINDOW       (WAC_BASE + 10)

/*= Tags ===================================================================*/
#define WAT_BASE                 (TAG_USER)

/*- WAC_MOVEWINDOW ---------------------------------------------------------*/
#define WAT_MOVEWINDOWX          (WAT_BASE + 1)
#define WAT_MOVEWINDOWY          (WAT_BASE + 2)

/*- WAC_SIZEWINDOW ---------------------------------------------------------*/
#define WAT_SIZEWINDOWX          (WAT_BASE + 3)
#define WAT_SIZEWINDOWY          (WAT_BASE + 4)

/*- WAC_CHANGEWINDOWBOX ----------------------------------------------------*/
#define WAT_WINDOWBOXLEFT        (WAT_BASE + 5)
#define WAT_WINDOWBOXTOP         (WAT_BASE + 6)
#define WAT_WINDOWBOXWIDTH       (WAT_BASE + 7)
#define WAT_WINDOWBOXHEIGHT      (WAT_BASE + 8)

/*- WAC_MOVEWINDOWINFRONTOF ------------------------------------------------*/
#define WAT_MOVEWBEHINDWINDOW    (WAT_BASE + 9)


#endif /* INTUITION_EXTENSIONS_H */
