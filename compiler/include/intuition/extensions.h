#ifndef INTUITION_EXTENSIONS_H
#define INTUITION_EXTENSIONS_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Sysiclass **************************************************************/
/*= SYSIA_Which ============================================================*/
#define ICONIFYIMAGE             (0x12L)
#define LOCKIMAGE                (0x13L)
#define MUIIMAGE                 (0x14L)
#define POPUPIMAGE               (0x15L)
#define SNAPSHOTIMAGE            (0x16L)
#define JUMPIMAGE                (0x17L)
#define MENUTOGGLEIMAGE          (0x19L)
#define SUBMENUIMAGE             (0x1AL)

/*** Window attributes ******************************************************/

#define WA_ExtraTitlebarGadgets                 (WA_Dummy + 151)
#define WA_ExtraGadgetsStartID                  (WA_Dummy + 152)
#define WA_ExtraGadget_Iconify                  (WA_Dummy + 153)
#define WA_ExtraGadget_Lock                     (WA_Dummy + 154)
#define WA_ExtraGadget_MUI                      (WA_Dummy + 155)
#define WA_ExtraGadget_PopUp                    (WA_Dummy + 156)
#define WA_ExtraGadget_Snapshot                 (WA_Dummy + 157)
#define WA_ExtraGadget_Jump                     (WA_Dummy + 158)


/*= WA_ExtraTitlebarGadgets ================================================*/
/*- Flags ------------------------------------------------------------------*/
#define ETG_ICONIFY              (0x01L)
#define ETG_LOCK                 (0x02L)
#define ETG_MUI                  (0x04L)
#define ETG_POPUP                (0x08L)
#define ETG_SNAPSHOT             (0x10L)
#define ETG_JUMP                 (0x20L)

/*- Gadget ID offsets ------------------------------------------------------*/
#define ETD_Iconify              (0)
#define ETD_Lock                 (1)
#define ETD_MUI                  (2)
#define ETD_PopUp                (3)
#define ETD_Snapshot             (4)
#define ETD_Jump                 (5)

/*- Gadget IDs -------------------------------------------------------------*/
#define ETI_Dummy                (0xFFD0)
#define ETI_Iconify              (ETI_Dummy + ETD_Iconify)
#define ETI_Lock                 (ETI_Dummy + ETD_Lock)
#define ETI_MUI                  (ETI_Dummy + ETD_MUI)
#define ETI_PopUp                (ETI_Dummy + ETD_PopUp)
#define ETI_Snapshot             (ETI_Dummy + ETD_Snapshot)
#define ETI_Jump                 (ETI_Dummy + ETD_Jump)



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
