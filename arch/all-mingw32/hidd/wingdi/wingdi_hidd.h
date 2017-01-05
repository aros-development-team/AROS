/* Semi-public definitions for GDI class */

#define GDI_LIBNAME "wingdi.hidd"

#define CLID_Hidd_Gfx_WinGDI "hidd.gfx.wingdi"

struct GDIBase
{
    struct Library library;	/* Common library header	 */
    ULONG	   displaynum;	/* Next available display number */
    OOP_Class	  *gfxclass;	/* Display driver class		 */
};
