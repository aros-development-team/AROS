/*
    Copyright © 2003, The AROS Development Team.
    $Id$
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/mui.h>
#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <aros/debug.h>

#include <stdio.h>

struct Library *OOPBase = NULL;
struct Library *MUIMasterBase = NULL;
struct UtilityBase *UtilityBase = NULL;

OOP_AttrBase __IHidd_PCIDev;
OOP_AttrBase __IHidd_PCIDrv;
OOP_AttrBase HiddAttrBase;

int openLibs()
{
    if ((OOPBase=OpenLibrary("oop.library", 0)) != NULL)
    {
	__IHidd_PCIDev = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
	__IHidd_PCIDrv = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
	HiddAttrBase = OOP_ObtainAttrBase(IID_Hidd);
	
	if ((MUIMasterBase=OpenLibrary("muimaster.library", 0)) != NULL)
	{
	    if ((UtilityBase=(struct UtilityBase*)OpenLibrary("utility.library", 0)) != NULL)
	    {
		return 1;
	    }
	}
    }
    return 0;
}

void closeLibs()
{
    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd);

    if (OOPBase != NULL)
	CloseLibrary(OOPBase);
    
    if (UtilityBase != NULL)
	CloseLibrary((struct Library *)UtilityBase);

    if (MUIMasterBase != NULL)
	CloseLibrary(MUIMasterBase);

    MUIMasterBase = NULL;
    UtilityBase = NULL;
    OOPBase = NULL;
}

Object *MakeLabel(STRPTR str)
{
  return (MUI_MakeObject(MUIO_Label, str, 0));
}

LONG xget(Object * obj, ULONG attr)
{
    LONG x = 0;
    get(obj, attr, &x);
    return x;
}

Object *app;
Object *MainWindow;
Object *DriverList;
Object *StrDriverName, *StrDriverHWName, *StrDriverDirect;

Object *StrDescription, *VendorID, *ProductID, *RevisionID;
Object *Interface, *_Class, *SubClass, *IRQLine;
Object *ROMBase, *ROMSize;
Object *RangeList;
Object *Status;

struct Hook pci_hook;
struct Hook display_hook;
struct Hook select_hook;

AROS_UFH3(void, pci_callback,
    AROS_UFHA(struct Hook *,	hook,	A0),
    AROS_UFHA(OOP_Object *,	obj,	A2),
    AROS_UFHA(APTR,		msg,	A1))
{
    AROS_USERFUNC_INIT

    DoMethod(DriverList, MUIM_List_InsertSingle, (IPTR)obj, MUIV_List_Insert_Bottom);

    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, display_function,
    AROS_UFHA(struct Hook *,	h,  A0),
    AROS_UFHA(char **, strings, A2),
    AROS_UFHA(OOP_Object *, obj, A1))
{
    AROS_USERFUNC_INIT

    static char buf[20];
    unsigned int bus, dev, sub;

    if (obj)
    {
	OOP_GetAttr(obj, aHidd_PCIDevice_Bus, (APTR)&bus);
	OOP_GetAttr(obj, aHidd_PCIDevice_Dev, (APTR)&dev);
	OOP_GetAttr(obj, aHidd_PCIDevice_Sub, (APTR)&sub);

	snprintf(buf, 19, "%02x.%02x.%01x", bus, dev, sub);
	strings[0] = buf;
    }

    AROS_USERFUNC_EXIT
}

void memoryPrint(STRPTR buffer, unsigned int base, unsigned int size, unsigned int type)
{
    if(type & ADDRF_IO)
    {
	snprintf(buffer, 59, "%s%s at 0x%04x (size 0x%04x)",
	    (type & ADDRF_PREFETCH) ? "Prefetchable " :"",
	    (type & ADDRF_IO)?"I/O":"Memory",
	    base, size);
    }
    else
    {
	snprintf(buffer, 59, "%s%s at 0x%08x (size 0x%08x)",
	    (type & ADDRF_PREFETCH) ? "Prefetchable " :"",
	    (type & ADDRF_IO)?"I/O":"Memory",
	    base, size);
    }
}

AROS_UFH3(void, select_function,
    AROS_UFHA(struct Hook *,	h,  A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT
    
    ULONG active;
    OOP_Object *obj, *drv;
    STRPTR class, subclass, interface, str;

    active = xget(object, MUIA_List_Active);
    if (active != MUIV_List_Active_Off)
    {
	unsigned int val, val2, val3;
	static char buf[80];

	static char ranges[6][60];
	DoMethod(object, MUIM_List_GetEntry, active, (IPTR)&obj);
	
	OOP_GetAttr(obj, aHidd_PCIDevice_Driver, (APTR)&drv);
	OOP_GetAttr(drv, aHidd_Name, (APTR)&str);
	set(StrDriverName, MUIA_Text_Contents, str);
	OOP_GetAttr(drv, aHidd_HardwareName, (APTR)&str);
	set(StrDriverHWName, MUIA_Text_Contents, str);
	OOP_GetAttr(drv, aHidd_PCIDriver_DirectBus, (APTR)&val);
	set(StrDriverDirect, MUIA_Text_Contents, (IPTR)((val)?"yes":"no"));

	OOP_GetAttr(obj, aHidd_PCIDevice_ClassDesc, (APTR)&class);
	OOP_GetAttr(obj, aHidd_PCIDevice_SubClassDesc, (APTR)&subclass);
	OOP_GetAttr(obj, aHidd_PCIDevice_InterfaceDesc, (APTR)&interface);
	snprintf(buf, 79, "%s %s %s", class, subclass, interface);
	set(StrDescription, MUIA_Text_Contents, buf);
	
	OOP_GetAttr(obj, aHidd_PCIDevice_VendorID, (APTR)&val);
	snprintf(buf, 79, "0x%04x", val);
	set(VendorID, MUIA_Text_Contents, buf);

	OOP_GetAttr(obj, aHidd_PCIDevice_ProductID, (APTR)&val);
	snprintf(buf, 79, "0x%04x", val);
	set(ProductID, MUIA_Text_Contents, buf);
 
	OOP_GetAttr(obj, aHidd_PCIDevice_RevisionID, (APTR)&val);
	snprintf(buf, 79, "0x%04x", val);
	set(RevisionID, MUIA_Text_Contents, buf);

	OOP_GetAttr(obj, aHidd_PCIDevice_Interface, (APTR)&val);
	snprintf(buf, 79, "0x%02x", val);
	set(Interface, MUIA_Text_Contents, buf);

	OOP_GetAttr(obj, aHidd_PCIDevice_Class, (APTR)&val);
	snprintf(buf, 79, "0x%02x", val);
	set(_Class, MUIA_Text_Contents, buf);

	OOP_GetAttr(obj, aHidd_PCIDevice_SubClass, (APTR)&val);
	snprintf(buf, 79, "0x%02x", val);
	set(SubClass, MUIA_Text_Contents, buf);

	OOP_GetAttr(obj, aHidd_PCIDevice_Sub, (APTR)&val);
	snprintf(buf, 79, "0x%02x", val);
	set(SubClass, MUIA_Text_Contents, buf);
 	
	OOP_GetAttr(obj, aHidd_PCIDevice_IRQLine, (APTR)&val);
	OOP_GetAttr(obj, aHidd_PCIDevice_INTLine, (APTR)&val2);
	if (val)
	{
	    snprintf(buf, 79, "%d (%c)", val2, val + 'A' - 1);
	}
	else snprintf(buf, 79, "N/A");
	set(IRQLine, MUIA_Text_Contents, buf);

	OOP_GetAttr(obj, aHidd_PCIDevice_RomBase, (APTR)&val);
	OOP_GetAttr(obj, aHidd_PCIDevice_RomSize, (APTR)&val2);
	snprintf(buf, 79, "0x%08x", val2);
	if (val2)
	{
	    set(ROMSize, MUIA_Text_Contents, buf);
	    if (val)
	    {
		snprintf(buf, 79, "0x%08x", val);
		set(ROMBase, MUIA_Text_Contents, buf);
	    }
	    else set(ROMBase, MUIA_Text_Contents, "--unused--");
	}
	else
	{
	    set(ROMBase, MUIA_Text_Contents, "N/A");
	    set(ROMSize, MUIA_Text_Contents, "N/A");
	}

	DoMethod(RangeList, MUIM_List_Clear);

	OOP_GetAttr(obj, aHidd_PCIDevice_Base0, (APTR)&val);
	OOP_GetAttr(obj, aHidd_PCIDevice_Size0, (APTR)&val2);
	OOP_GetAttr(obj, aHidd_PCIDevice_Type0, (APTR)&val3);

	if (val)
	{
	    memoryPrint(ranges[0], val, val2, val3);
	    DoMethod(RangeList, MUIM_List_InsertSingle, (IPTR)ranges[0], MUIV_List_Insert_Bottom);
	}

	OOP_GetAttr(obj, aHidd_PCIDevice_Base1, (APTR)&val);
	OOP_GetAttr(obj, aHidd_PCIDevice_Size1, (APTR)&val2);
	OOP_GetAttr(obj, aHidd_PCIDevice_Type1, (APTR)&val3);

	if (val)
	{
	    memoryPrint(ranges[1], val, val2, val3);
	    DoMethod(RangeList, MUIM_List_InsertSingle, (IPTR)ranges[1], MUIV_List_Insert_Bottom);
	}

	OOP_GetAttr(obj, aHidd_PCIDevice_isBridge, (APTR)&val);
	if (!val)
	{

	    OOP_GetAttr(obj, aHidd_PCIDevice_Base2, (APTR)&val);
	    OOP_GetAttr(obj, aHidd_PCIDevice_Size2, (APTR)&val2);
	    OOP_GetAttr(obj, aHidd_PCIDevice_Type2, (APTR)&val3);

	    if (val)
	    {
	        memoryPrint(ranges[2], val, val2, val3);
		DoMethod(RangeList, MUIM_List_InsertSingle, (IPTR)ranges[2], MUIV_List_Insert_Bottom);
	    }

	    OOP_GetAttr(obj, aHidd_PCIDevice_Base3, (APTR)&val);
	    OOP_GetAttr(obj, aHidd_PCIDevice_Size3, (APTR)&val2);
	    OOP_GetAttr(obj, aHidd_PCIDevice_Type3, (APTR)&val3);

	    if (val)
	    {
	        memoryPrint(ranges[3], val, val2, val3);
		DoMethod(RangeList, MUIM_List_InsertSingle, (IPTR)ranges[3], MUIV_List_Insert_Bottom);
	    }

	    OOP_GetAttr(obj, aHidd_PCIDevice_Base4, (APTR)&val);
	    OOP_GetAttr(obj, aHidd_PCIDevice_Size4, (APTR)&val2);
	    OOP_GetAttr(obj, aHidd_PCIDevice_Type4, (APTR)&val3);

	    if (val)
	    {
	        memoryPrint(ranges[4], val, val2, val3);
		DoMethod(RangeList, MUIM_List_InsertSingle, (IPTR)ranges[4], MUIV_List_Insert_Bottom);
	    }

	    OOP_GetAttr(obj, aHidd_PCIDevice_Base5, (APTR)&val);
	    OOP_GetAttr(obj, aHidd_PCIDevice_Size5, (APTR)&val2);
	    OOP_GetAttr(obj, aHidd_PCIDevice_Type5, (APTR)&val3);

	    if (val)
	    {
		memoryPrint(ranges[5], val, val2, val3);
		DoMethod(RangeList, MUIM_List_InsertSingle, (IPTR)ranges[5], MUIV_List_Insert_Bottom);
	    }
	}
	else
	{
	    OOP_GetAttr(obj, aHidd_PCIDevice_SubBus, (APTR)&val);
	    OOP_GetAttr(obj, aHidd_PCIDevice_ISAEnable, (APTR)&val2);
	    OOP_GetAttr(obj, aHidd_PCIDevice_VGAEnable, (APTR)&val3);

	    snprintf(ranges[2], 59, "Bridge handles bus %d (ISA %senabled, VGA %senabled)",
		val, (val2)?" ":"not ", (val3)?" ":"not ");
	    DoMethod(RangeList, MUIM_List_InsertSingle, (IPTR)ranges[2], MUIV_List_Insert_Bottom);

	    OOP_GetAttr(obj, aHidd_PCIDevice_MemoryBase, (APTR)&val);
	    OOP_GetAttr(obj, aHidd_PCIDevice_MemoryLimit, (APTR)&val2);
	    
	    snprintf(ranges[3], 59, "Memory ranges from %08x to %08x",
		val, val2);
	    DoMethod(RangeList, MUIM_List_InsertSingle, (IPTR)ranges[3], MUIV_List_Insert_Bottom);

	    OOP_GetAttr(obj, aHidd_PCIDevice_PrefetchableBase, (APTR)&val);
	    OOP_GetAttr(obj, aHidd_PCIDevice_PrefetchableLimit, (APTR)&val2);
	    
	    snprintf(ranges[4], 59, "Prefetchable ranges memory from %08x to %08x",
		val, val2);
	    DoMethod(RangeList, MUIM_List_InsertSingle, (IPTR)ranges[4], MUIV_List_Insert_Bottom);
	    
	    OOP_GetAttr(obj, aHidd_PCIDevice_IOBase, (APTR)&val);
	    OOP_GetAttr(obj, aHidd_PCIDevice_IOLimit, (APTR)&val2);
	    
	    snprintf(ranges[5], 59, "IO ranges from %04x to %04x",
		val, val2);
	    DoMethod(RangeList, MUIM_List_InsertSingle, (IPTR)ranges[5], MUIV_List_Insert_Bottom);

	}
    {
		ULONG io, mem, master, snoop, is66;
		OOP_GetAttr(obj, aHidd_PCIDevice_isIO, (APTR)&io);
		OOP_GetAttr(obj, aHidd_PCIDevice_isMEM, (APTR)&mem);
		OOP_GetAttr(obj, aHidd_PCIDevice_isMaster, (APTR)&master);
		OOP_GetAttr(obj, aHidd_PCIDevice_paletteSnoop, (APTR)&snoop);
		OOP_GetAttr(obj, aHidd_PCIDevice_is66MHz, (APTR)&is66);

		snprintf(buf, 79, "IO: %s, MEM: %s, Master: %s, PaletteSnoop: %s, 66MHz capable: %s",
		    io ? "yes":"no",
		    mem ? "yes":"no",
		    master ? "yes":"no",
		    snoop ? "yes":"no",
		    is66 ? "yes":"no");
		set(Status, MUIA_Text_Contents, buf);
    }
    }

    AROS_USERFUNC_EXIT
}

BOOL GUIinit()
{
    BOOL retval = FALSE;
    
    app = ApplicationObject,
	    MUIA_Application_Title,	    (IPTR)"PCI Tool",
	    MUIA_Application_Version,	    (IPTR)"$VER: pcitool 0.0.1 (23.01.04)",
	    MUIA_Application_Copyright,	    (IPTR)"© 2004, The AROS Development Team",
	    MUIA_Application_Author,	    (IPTR)"Michal Schulz",
	    MUIA_Application_Base,	    (IPTR)"PCITool",
	    MUIA_Application_Description,   (IPTR)"PCI querying and managment",

	    SubWindow, MainWindow = WindowObject,
                MUIA_Window_Title,	(IPTR) "PCI Tool",
//		MUIA_Window_Height,	MUIV_Window_Height_Visible(50),
//		MUIA_Window_Width,	MUIV_Window_Width_Visible(60),

		WindowContents, HGroup,
		    MUIA_Group_SameWidth, FALSE,
		    
		    Child, ListviewObject,
			MUIA_Listview_List, DriverList = ListObject,
			    ReadListFrame,
			    MUIA_List_AdjustWidth, TRUE,
			    MUIA_List_DisplayHook, &display_hook,
			End, // List
		    End, // ListView
		    Child, VGroup,
			Child, VGroup, GroupFrameT("Driver info"),
			    Child, HGroup,
				Child, ColGroup(2),
				    MUIA_Weight, 100,
				    Child, Label("Driver name:"),
				    Child, StrDriverName = TextObject,
					StringFrame,
					MUIA_Text_SetMax, FALSE,
					MUIA_Text_Contents, "",
				    End,
				End,
				Child, ColGroup(2),
				    MUIA_Weight, 100,
				    Child, Label("Direct Bus:"),
				    Child, StrDriverDirect = TextObject,
					StringFrame,
					MUIA_Text_SetMax, FALSE,
					MUIA_Text_Contents, "",
				    End,
				End,
			    End,
			    Child, ColGroup(2),
				MUIA_Weight, 180,
				Child, Label("Hardware info:"),
				Child, StrDriverHWName = TextObject,
				    StringFrame,
				    MUIA_Text_SetMax, FALSE,
				    MUIA_Text_Contents, "",
				End,
			    End,
			End, // HGroup
			Child, VGroup, GroupFrameT("PCI device info"),
			    Child, ColGroup(2),
				Child, Label("Description:"),
				Child, StrDescription = TextObject,
				    StringFrame,
				    MUIA_Text_SetMax, FALSE,
				    MUIA_Text_Contents, "",
				End,
			    End,
			    Child, HGroup,
				Child, ColGroup(2),
				    Child, Label("VendorID:"),
				    Child, VendorID = TextObject,
					StringFrame,
					MUIA_Text_SetMax, FALSE,
					MUIA_Text_Contents, "0x0000",
				    End,
				End,
				Child, ColGroup(2),
				    Child, Label("ProductID:"),
				    Child, ProductID = TextObject,
					StringFrame,
					MUIA_Text_SetMax, FALSE,
					MUIA_Text_Contents, "0x0000",
				    End,
				End,
				Child, ColGroup(2),
				    Child, Label("RevisionID:"),
				    Child, RevisionID = TextObject,
					StringFrame,
					MUIA_Text_SetMax, FALSE,
					MUIA_Text_Contents, "0x0000",
				    End,
				End,
			    End,
			    Child, HGroup,
				Child, ColGroup(2),
				    MUIA_Weight, 0,
				    Child, Label("Interface:"),
				    Child, Interface = TextObject,
					StringFrame,
					MUIA_Text_SetMax, FALSE,
					MUIA_Text_Contents, "0x00",
				    End,
				End,
				Child, ColGroup(2),
				    MUIA_Weight, 0,
				    Child, Label("Class:"),
				    Child, _Class = TextObject,
					StringFrame,
					MUIA_Text_SetMax, FALSE,
					MUIA_Text_Contents, "0x00",
				    End,
				End,
				Child, ColGroup(2),
				    MUIA_Weight, 0,
				    Child, Label("SubClass:"),
				    Child, SubClass = TextObject,
					StringFrame,
					MUIA_Text_SetMax, FALSE,
					MUIA_Text_Contents, "0x00",
				    End,
				End,
				Child, ColGroup(2),
				    Child, Label("IRQ:"),
				    Child, IRQLine = TextObject,
					StringFrame,
					MUIA_Text_SetMax, FALSE,
					MUIA_Text_Contents, "15",
				    End,
				End,
			    End,
			    Child, HGroup,
				Child, ColGroup(2),
				    MUIA_Weight, 0,
				    Child, Label("ROM Base:"),
				    Child, ROMBase = TextObject,
					StringFrame,
					MUIA_Text_SetMax, FALSE,
					MUIA_Text_Contents, "0x00000000",
				    End,
				End,
				Child, ColGroup(2),
				    MUIA_Weight, 0,
				    Child, Label("ROM Size:"),
				    Child, ROMSize = TextObject,
					StringFrame,
					MUIA_Text_SetMax, FALSE,
					MUIA_Text_Contents, "0x00000000",
				    End,
				End,
				Child, HVSpace,
			    End,
			    Child, HGroup,
				Child, RangeList =  ListviewObject,
				    MUIA_Listview_List, DriverList = ListObject,
				    ReadListFrame,
				    MUIA_List_AdjustWidth, FALSE,
				    End, // List
				End, // ListView
			    End,
			    Child, Status = TextObject,
				StringFrame,
				MUIA_Text_SetMax, FALSE,
			    End,
			End,
		    End,
		End, // WindowContents
	    End, // MainWindow
	End; // ApplicationObject

    if (app)
    {
        /* Quit application if the windowclosegadget or the esc key is pressed. */
	
	DoMethod(MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
	         (IPTR)app, 2, 
		 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

	DoMethod(DriverList, MUIM_Notify, MUIA_Listview_SelectChange, TRUE,
		(IPTR)DriverList, 2,
		MUIM_CallHook, (IPTR)&select_hook);

	retval=TRUE;
    }

    return retval;
}

void loop(void)
{
    ULONG sigs = 0;

    while((LONG) DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs) != MUIV_Application_ReturnID_Quit)
    {
	if (sigs)
	{
	    sigs = Wait(sigs);
	}
    }
} /* loop(void)*/


int main(int argc, char *argv[])
{
    OOP_Object *o;
    
    pci_hook.h_Entry = (APTR)pci_callback;
    display_hook.h_Entry = (APTR)display_function;
    select_hook.h_Entry = (APTR)select_function;

    if(openLibs())
    {
    	if(GUIinit())
    	{
	    o = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
	    if (o)
	    {	
		HIDD_PCI_EnumDevices(o, &pci_hook, NULL);
		OOP_DisposeObject(o);
	    }
	    
      	    set(MainWindow, MUIA_Window_Open, TRUE);
	    
    	    if(xget(MainWindow, MUIA_Window_Open))
	    {
                loop();
	    }

      	    set(MainWindow, MUIA_Window_Open, FALSE);
	    DisposeObject(app);

//	    deinit_gui();
    	}
	
	closeLibs();
    }
    return 0;
} /* main(int argc, char *argv[]) */
