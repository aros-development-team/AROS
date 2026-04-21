#!/usr/bin/env python3
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
# Author: Fabian Schmieder
"""
m68kemu thunk generator — reads Amiga FD files + NDK clib protos for type info.

Usage: python3 m68kemu_thunkgen.py <fd_directory> [<ndk_clib_directory>] > m68kemu_thunks_gen.c

If clib directory is provided, function prototypes are parsed for accurate
parameter types (shadow lookups, string translation, struct translation, etc.).
Without it, falls back to generic APTR/ULONG behavior.
"""

import sys, os, re, glob

# ── Libraries to SKIP entirely ──
SKIP_LIBRARIES = {
    "workbench", "usergroup", "timer", "task", "rexxsyslib", "rexxsupport",
    "reqtools", "realtime", "openurl", "nonvolatile", "muiscreen", "muimaster",
    "misc", "mathtrans", "mathieeesingtrans", "mathieeesingbas",
    "mathieeedoubtrans", "mathieeedoubbas", "mathffp", "lowlevel", "input",
    "identify", "console", "colorwheel", "cardres", "camd", "battclock",
    "asyncio", "amigaguide", "codesets", "utf8proc", "glu", "png", "tiff",
    "expat", "zstd", "lzma", "bz2", "bullet", "freetype2",
    "aros", "hostlib", "kernel", "kms", "gallium", "x11gfx", "efi",
    "openfirmware", "rtas", "vmm", "hpet", "prometheus", "pccard",
    "ata", "scsi", "poseidon", "usbclass", "lddemon", "debug",
    "oop", "log", "processor", "bootloader", "partition",
    "posixc", "stdc", "stdcio", "m68kemu", "fd",
    "pngdt", "dxtn", "jfif", "coolimages", "diskimage", "nvdisk", "ramdrive",
    "z1", "uuid",
}

# ── Functions to SKIP (manual impl, dangerous, or internal) ──
SKIP_FUNCTIONS = {
    "AddPort", "AllocMem", "AllocPooled", "AllocVec", "AllocVecPooled",
    "AvailMem", "CacheClearU", "Close", "CloseLibrary", "CloseScreen",
    "CloseWindow", "CopyMem", "CreatePool", "CurrentDir", "Delay",
    "DeleteFile", "DeletePool", "Draw", "Examine", "FPuts", "FindName",
    "FindTask", "Forbid", "FreeArgs", "FreeMem", "FreePooled", "FreeVec",
    "FreeVecPooled2", "GetMsg", "GetProgramName", "InitSemaphore", "Input",
    "IoErr", "Lock", "Move", "ObtainSemaphore", "OldOpenLibrary", "Open",
    "OpenLibrary", "OpenScreen", "OpenWindowTagList", "Output", "Permit",
    "PutStr", "RawDoFmt", "Read", "ReadArgs", "RectFill",
    "ReleaseSemaphore", "RemPort", "Rename", "ReplyMsg", "ScrollRaster",
    "Seek", "SetABPenDrMd", "SetAPen", "SetBPen", "SetDrMd", "SetFont",
    "SetIoErr", "SetSignal", "Text", "TextLength", "TypeOfMem", "UnLock",
    "VPrintf", "WaitPort", "Write",
    "AVL_AddNode", "AVL_FindNode", "AVL_RemNodeByAddress", "AddDevice",
    "AddIntServer", "AddLibrary", "AddMemHandler", "AddMemList",
    "AddResetCallback", "Alert", "AllocDosObject", "CachePostDMA",
    "CachePreDMA", "Cause", "CliInitNewcli", "CliInitRun", "CloseLib",
    "ColdReboot", "CreateNewProc", "CreateProc", "Debug", "Dispatch",
    "DoPkt", "ExAll", "ExAllEnd", "ExNext", "Exception", "Execute",
    "ExitIntr", "FreeDosObject", "InitCode", "InitResident", "InitStruct",
    "InternalLoadSeg", "InternalUnLoadSeg", "LoadSeg", "MakeFunctions",
    "MakeLibrary", "MatchEnd", "MatchFirst", "MatchNext", "NewLoadSeg",
    "OpenLib", "RemDevice", "RemIntServer", "RemLibrary", "RemMemHandler",
    "RemResetCallback", "ReplyPkt", "Reschedule", "RunCommand", "RunProcess",
    "Schedule", "SendPkt", "SetFunction", "SetIntVector", "SetSR",
    "StackSwap", "SumKickData", "SumLibrary", "SuperState", "Supervisor",
    "Switch", "SystemTagList", "UnLoadSeg", "UserState", "VFPrintf",
    "VFWritef", "WaitPkt", "close", "open", "private",
}

# ── Functions that return void ──
VOID_FUNCTIONS = {
    "Text", "ZipWindow", "WriteExpansionWord", "WriteExpansionByte",
    "WriteChunkyPixels", "WriteBattClock", "WindowAction", "WaitBOVP",
    "URL_OldFreePrefs", "URL_FreePrefsA", "UpdateBitMap", "UnLockTaskList",
    "UnlockRexxBase", "UnlockRealTime", "UnlockIBase", "UnlockCAMD",
    "UnLockBitMapTagList", "UnLockBitMap", "ThinLayerInfo", "TextExtent",
    "SyncSBitMap", "SubTime", "StripFont", "StoreItemInContext",
    "StopTimerInt", "StartTimerInt", "StartClusterNotify", "SortLayerCR",
    "SkipSysEx", "ShowTitle", "setutent", "SetTranslate", "SetRGB4CM",
    "SetRGB32CM", "SetRast", "setpwent", "SetPointer", "SetLocalItemPurge",
    "SetKeyMapDefault", "setgrent", "SetFilterIX", "SetFilter",
    "SetDisplayDriverCallback", "SetDefaultScreenFont", "SetCurrentBinding",
    "SetCollision", "SetABPenDrMd", "SendIntuiMessage",
    "ScrollWindowRasterNoFill", "ScrollRasterBF", "ScrollLayer",
    "ScreenPosition", "ScreenDepth", "rtUnlockWindow", "rtUnlockPrefs",
    "rtSpread", "rtSetWaitPointer", "rtSetReqPosition",
    "rtScreenToFrontSafely", "rtFreeRequest", "rtFreeReqBuffer",
    "rtFreeFileList", "rtCloseWindowSafely", "RouteCxMsg",
    "RestoreTaskStorage", "ResetBattClock", "ReplyAmigaGuideMsg",
    "RemVSprite", "RemVBlankInt", "RemTimerInt", "RemoveMidiLink",
    "RemoveClass", "RemNamedObject", "RemKBInt", "RemIBob", "RemFont",
    "RemConSnipHook", "RemConfigDev", "ReleasePen", "ReleaseGIRPort",
    "ReleaseDTDrawInfo", "ReleaseDataType", "ReleaseConfigBinding",
    "ReleaseCard", "RefreshTagItemClones", "RefreshGList",
    "RefreshDTObjectA", "QueryTaskTagList", "QueryKeys", "QBSBlit",
    "QBlit", "PutSysEx", "PutMidi", "ProcessPixelArray", "PrintIText",
    "PolyDraw", "ParseMidi", "ObtainConfigBinding", "MUIS_RemInfoClient",
    "MUIS_FreePubScreenDesc", "MUIS_ClosePubFile", "MUIS_AddInfoClient",
    "MUI_RequestIDCMP", "MUI_RemoveClipRegion", "MUI_RemoveClipping",
    "MUI_ReleasePen", "MUI_RejectIDCMP", "MUI_Redraw", "MUI_FreeClass",
    "MUI_FreeAslRequest", "MUI_EndRefresh", "MUI_DisposeObject",
    "MoveWindowInFrontOf", "MoveScreen", "MapTags", "LockRexxBase",
    "LockDataType", "LoadView", "LendMenus", "InsertCxObj", "InitVPort",
    "InitView", "InitRequester", "InitMasks", "InitLayers", "InitIFFasDOS",
    "InitIFFasClip", "InitIFF", "InitGMasks", "InitGels", "InitArea",
    "IdHardwareUpdate", "HelpControl", "GT_ReplyIMsg", "GT_EndRefresh",
    "GT_BeginRefresh", "GfxFree", "GfxAssociate", "GetUpTime", "GetSysTime",
    "GetRPAttrsA", "GetRGB32", "GadgetMouse", "FreeTaskStorageSlot",
    "FreeSpriteData", "FreeNVData", "FreeMonitorList", "FreeMiscResource",
    "FreeLocalItem", "FreeIntuiMessage", "FreeICData", "FreeExpansionMem",
    "FreeDTMethods", "FreeDBufInfo", "FreeConfigDev", "FreeCModeList",
    "FreeBrokerList", "FreeBoardMem", "FormatDate", "FontExtent",
    "FlushMidi", "FilterTagChanges", "ExpungeXRef", "EraseImage",
    "endutent", "EndUpdate", "EndRefresh", "endpwent", "endgrent",
    "EndClusterNotify", "DrawImageState", "DrawImage", "DrawIconStateA",
    "DrawEllipse", "DrawBorder", "DrawBevelBoxA", "DoHookClipRects",
    "DoCollision", "DoCDrawMethodTagList", "DivertCxMsg",
    "DisposeLayerInfo", "DisposeFontContents", "DisposeCxMsg",
    "DisplayBeep", "DeleteRexxMsg", "DeletePlayer", "DeleteMidi",
    "DeleteArgstring", "CWait", "CVideoCtrlTagList", "CurrentTime",
    "CopySBitMap", "ConvertRGBToHSB", "ConvertHSBToRGB", "ConfigChain",
    "CollectPixelsLayer", "CMove", "ClosePortRsrc", "CloseMidiDevice",
    "CloseClipboard", "ClearScreen", "ClearRexxMsg", "ClearEOL",
    "ChangeVPBitMap", "ChangeToSelectedIconColor", "ChangeDecoration",
    "CBump", "BltTemplateAlpha", "BltClear", "BitMapScale", "BeginRefresh",
    "AskFont", "ApplyTagChanges", "Animate", "Amiga2Date", "AlohaWorkbench",
    "AllocBoardMem", "AddVSprite", "AddTime", "AddIEvents", "AddFont",
    "AddConSnipHook", "AddConfigDev", "AddClass", "AddBob", "AddAnimOb",
    "ActivateAslRequest", "AbortAslRequest", "UnlockLayerInfo",
    "LockLayerInfo", "UnlockLayers", "LockLayers", "UnlockLayer",
    "LockLayer", "UnlockLayerRom", "LockLayerRom", "MoveSprite",
    "ChangeSprite", "FreeGBuffers", "DrawGList", "SortGList",
    "SwapBitsRastPortClipRect", "DisposeRegion", "ClearRegion",
    "ClearRectRegion", "XorRectRegion", "OrRectRegion", "AndRectRegion",
    "SetWriteMask", "SetMaxPen", "BltMaskBitMapRastPort", "BltPattern",
    "BltTemplate", "ClipBlit", "BltBitMapRastPort", "BltBitMap",
    "WritePixelArray8", "WritePixelLine8", "SetRPAttrsA",
    "ScrollWindowRaster", "EraseRect", "SetPointerA", "ModifyProp",
    "NewModifyProp", "RemoveGadget", "AddGadget", "RemoveGList", "AddGList",
    "OffGadget", "OnGadget", "OffMenu", "OnMenu", "SetMenuStrip",
    "UnlockPubScreenList", "LockPubScreenList", "SetDefaultPubScreen",
    "ChangeWindowBox", "FreeScreenBuffer", "UnlockPubScreen", "SetAttrsA",
    "ReleaseNamedObject", "FreeNamedObject", "SetAmigaGuideAttrsA",
    "CloseAmigaGuide", "UnlockAmigaGuideBase", "Forbid", "Permit",
    "Disable", "Enable", "InitSemaphore", "ObtainSemaphore",
    "ReleaseSemaphore", "AddPort", "RemPort", "AddSemaphore",
    "RemSemaphore", "CopyMem", "CopyMemQuick", "CacheClearU", "CacheClearE",
    "FreeMem", "FreeVec", "FreePooled", "DeletePool", "FreeVecPooled",
    "CloseLibrary", "Close", "UnLock", "Delay", "FreeArgs", "FreeDosObject",
    "AbortPkt", "FreeDeviceProc", "FreeDosEntry", "EndNotify",
    "SortDirList", "SetIoErr", "RawIOInit", "RawPutChar", "ChildFree",
    "ChildOrphan", "RemResetCallback", "Move", "Draw", "SetAPen", "SetBPen",
    "SetDrMd", "SetFont", "RectFill", "ScrollRaster", "SetRGB4", "SetRGB32",
    "WaitBlit", "WaitTOF", "InitRastPort", "InitBitMap", "FreeRaster",
    "FreeBitMap", "FreeColorMap", "DisownBlitter", "OwnBlitter", "CloseFont",
    "FreeSprite", "LoadRGB4", "SetWindowPointerA", "RefreshGList", "LoadRGB32", "ScrollVPort", "MakeVPort",
    "MrgCop", "FreeVPortCopLists", "FreeCopList", "FreeCprList",
    "CloseWindow", "CloseScreen", "ClearMenuStrip", "ClearPointer",
    "ActivateWindow", "ActivateGadget", "RefreshGadgets",
    "RefreshWindowFrame", "MoveWindow", "SizeWindow", "WindowToFront",
    "WindowToBack", "ScreenToFront", "ScreenToBack", "ModifyIDCMP",
    "ReportMouse", "FreeScreenDrawInfo", "FreeRemember", "DisposeObject",
    "SetWindowTitles", "SetWindowPointerA", "RefreshGList", "EndRequest", "FreeSysRequest",
    "FreeGadgets", "FreeMenus", "FreeVisualInfo", "GT_SetGadgetAttrsA",
    "GT_RefreshWindow", "InstallClipRegion", "DeleteLayer", "FreeIFF",
    "CloseIFF", "FreeAslRequest", "FreeFileRequest", "FreeDiskObject",
    "FreeFreeList", "CloseCatalog", "CloseLocale", "CloseWorkbenchObjectA",
    "FreeTagItems", "DisposeDTObject", "DeleteCxObj", "DeleteCxObjAll",
    "ClearCxObjError", "SetCxObjPri", "AttachCxObj", "EnqueueCxObj",
    "RemoveCxObj", "ActivateCxObj", "AddHead", "AddResource", "AddTail",
    "CloseDevice", "Deallocate", "DeleteIORequest", "DeleteMsgPort",
    "Enqueue", "Exit", "FreeEntry", "FreeSignal", "FreeTrap", "Insert",
    "NewMinList", "ObtainSemaphoreList", "ObtainSemaphoreShared", "PutMsg",
    "ReleaseSemaphoreList", "Remove", "RemResource", "RemTask", "SendIO",
    "Signal", "UnLockDosList", "Vacate",
}

# Functions that return struct pointers — wrap with shadow_create_by_name
SHADOW_RETURNS = {
    "FindName": "Node", "FindTask": "Task", "FindPort": "MsgPort",
    "FindSemaphore": "SignalSemaphore", "FindResident": "Resident",
    "CreateIORequest": "IORequest", "CreateMsgPort": "MsgPort",
    "OpenFont": "TextFont", "AllocBitMap": "BitMap",
    "CreateRastPort": "RastPort", "CloneRastPort": "RastPort",
    "GetColorMap": "ColorMap",
    "CreateGadgetA": "Gadget", "CreateMenusA": "Menu",
    "FindTagItem": "TagItem", "NextTagItem": "TagItem",
    "AllocNamedObjectA": "NamedObject",
    "LockPubScreen": "Screen",
    "BuildEasyRequestArgs": "Window",
    "GetDeviceProc": "DevProc",
    "FindTaskByPID": "Task",
}

# ── D-register args that are pointers (fallback when no proto info) ──
DPTR_OVERRIDES = {
    "Open": {"d1"}, "DeleteFile": {"d1"}, "Rename": {"d1", "d2"},
    "Read": {"d2"}, "Write": {"d2"}, "Lock": {"d1"},
    "PutStr": {"d1"}, "FPuts": {"d2"}, "VPrintf": {"d1", "d2"},
    "FWrite": {"d2"}, "FRead": {"d2"}, "FGets": {"d2"},
    "SetFileDate": {"d1"}, "NameFromLock": {"d2"}, "NameFromFH": {"d2"},
    "FilePart": {"d1"}, "PathPart": {"d1"}, "AddPart": {"d1", "d2"},
    "GetProgramName": {"d1"}, "SetComment": {"d1", "d2"},
    "SetProtection": {"d1"}, "MakeDosEntry": {"d1"},
    "SetVar": {"d1", "d2"}, "GetVar": {"d1", "d2"},
    "DeleteVar": {"d1"}, "FindVar": {"d1"},
    "MatchPattern": {"d1", "d2"}, "MatchPatternNoCase": {"d1", "d2"},
    "ParsePattern": {"d1", "d2"}, "ParsePatternNoCase": {"d1", "d2"},
    "CreateDir": {"d1"}, "SetCurrentDirName": {"d1"},
    "GetCurrentDirName": {"d1"}, "SetProgramName": {"d1"},
    "Fault": {"d3"}, "PrintFault": {"d2"},
    "Relabel": {"d1", "d2"}, "SplitName": {"d1", "d3"},
    "AssignLock": {"d1"}, "AssignLate": {"d1", "d2"},
    "AssignPath": {"d1", "d2"}, "AssignAdd": {"d1"},
    "GetDeviceProc": {"d1"},
    "Text": {"d0"},
    "Stricmp": {"d0", "d1"}, "Strnicmp": {"d0", "d1"},
    "GetDiskObject": {"d0"}, "PutDiskObject": {"d0"},
    "GetDiskObjectNew": {"d0"},
    "OpenCatalog": {"d0"},
}

# ── Type classification for proto-aware thunk generation ──

# Opaque struct pointers: use shadow_lookup to find native pointer
SHADOW_LOOKUP_TYPES = {
    "RastPort", "ViewPort", "Screen", "Window", "ColorMap", "BitMap",
    "Layer", "Region", "TextFont", "GadgetInfo", "DrawInfo", "Menu",
    "Gadget", "Requester", "IntuiMessage", "MsgPort", "IORequest",
    "Library", "Device", "Task", "Process", "SignalSemaphore",
}

# Input structs needing field-by-field translation — these MUST be manual thunks
# because m68k and native struct layouts differ (pointer sizes, alignment).
# CopyMem from m68k memory to native struct is NOT correct.
INPUT_STRUCT_TYPES = {"NewWindow", "NewScreen", "TextAttr", "EasyStruct",
                      "NewGadget", "Border", "IntuiText"}

# String types
STRING_TYPES = {"STRPTR", "CONST_STRPTR", "UBYTE *", "char *", "const char *"}

# Scalar/passthrough types
SCALAR_TYPES = {"ULONG", "LONG", "WORD", "UWORD", "BOOL", "UBYTE", "BYTE",
                "BPTR", "BSTR", "Tag", "APTR", "CONST_APTR", "PLANEPTR"}

# ── Proto file parser ──

def parse_clib_protos(clib_dir):
    """Parse all *_protos.h files, return dict: funcname -> (return_type, [(param_type, param_name), ...])"""
    protos = {}
    if not clib_dir or not os.path.isdir(clib_dir):
        return protos

    # Match individual parameter: type + name (handle * in type or attached to name)
    def parse_param(p):
        """Parse 'CONST struct Foo *bar' -> ('CONST struct Foo *', 'bar')"""
        p = p.strip()
        if not p or p == "VOID":
            return None
        if '(' in p:
            return ("APTR", "funcptr")
        # Try: name is last \w+ token, possibly preceded by *
        m = re.match(r'^(.*?)\s*(\**)(\w+)$', p)
        if m:
            base_type = m.group(1).strip()
            stars = m.group(2)
            name = m.group(3)
            if stars:
                full_type = base_type + ' ' + stars if base_type else stars
            else:
                full_type = base_type
            return (full_type.strip(), name)
        return ("ULONG", p)

    for proto_file in sorted(glob.glob(os.path.join(clib_dir, "*_protos.h"))):
        with open(proto_file, encoding='latin-1') as f:
            text = f.read()

        # Remove C comments and preprocessor lines
        text = re.sub(r'/\*.*?\*/', ' ', text, flags=re.DOTALL)
        text = re.sub(r'#[^\n]*', ' ', text)
        text = re.sub(r'\\\n', ' ', text)
        text = re.sub(r'\s+', ' ', text)

        # Split on semicolons, find __stdargs declarations
        for chunk in text.split(';'):
            m = re.search(
                r'((?:CONST\s+)?(?:struct\s+\w+\s*\*|[\w]+(?:\s*\*)?)\s*)'
                r'__stdargs\s+(\w+)\s*\(\s*(.*?)\s*\)',
                chunk
            )
            if not m:
                continue
            ret_type = m.group(1).strip()
            func_name = m.group(2).strip()
            params_str = m.group(3).strip()

            if '...' in params_str:
                continue

            params = []
            if params_str and params_str != "VOID":
                for p in params_str.split(','):
                    p = p.strip()
                    if not p:
                        continue
                    result = parse_param(p)
                    if result:
                        params.append(result)

            protos[func_name] = (ret_type, params)

    return protos


def classify_type(ptype):
    """Classify a C type string into a category for thunk generation.
    Returns: ('shadow', struct_name), ('input_struct', struct_name),
             ('string',), ('taglist',), ('scalar',), ('pointer',)
    """
    ptype = ptype.strip()

    # Remove CONST qualifier
    clean = re.sub(r'\bCONST\b', '', ptype).strip()
    clean = re.sub(r'\bconst\b', '', clean).strip()

    # Check for string types
    if clean in STRING_TYPES or ptype in STRING_TYPES:
        return ('string',)

    # Check for struct pointer
    sm = re.match(r'struct\s+(\w+)\s*\*', clean)
    if sm:
        sname = sm.group(1)
        if sname == "TagItem":
            return ('taglist',)
        if sname in SHADOW_LOOKUP_TYPES:
            return ('shadow', sname)
        if sname in INPUT_STRUCT_TYPES:
            return ('input_struct', sname)
        return ('pointer',)

    # Check scalar
    base = clean.rstrip(' *')
    if base in SCALAR_TYPES or clean in SCALAR_TYPES:
        if '*' in clean:
            return ('pointer',)
        return ('scalar',)

    # Anything with a pointer is a generic pointer
    if '*' in clean:
        return ('pointer',)

    # Default: scalar
    return ('scalar',)


def classify_return_type(rtype):
    """Classify return type. Returns: ('shadow_create', struct_name), ('plain',), ('void',)"""
    rtype = rtype.strip()
    if rtype == "VOID" or rtype == "void":
        return ('void',)

    clean = re.sub(r'\bCONST\b', '', rtype).strip()
    clean = re.sub(r'\bconst\b', '', clean).strip()

    sm = re.match(r'struct\s+(\w+)\s*\*', clean)
    if sm:
        sname = sm.group(1)
        # Only shadow_create for types we know how to shadow
        if sname in SHADOW_LOOKUP_TYPES:
            return ('shadow_create', sname)
    return ('plain',)


# ── FD parser ──

def parse_fd(filename):
    """Parse an Amiga FD file, return (base_name, list of (name, bias, args, regs))."""
    funcs = []
    bias = 0
    public = True
    base_name = None

    with open(filename) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("*"):
                continue
            if line.startswith("##base"):
                base_name = line.split()[1].lstrip("_")
                continue
            if line.startswith("##bias"):
                bias = int(line.split()[1])
                continue
            if line == "##public":
                public = True
                continue
            if line == "##private":
                public = False
                continue
            if line.startswith("##"):
                continue
            if not public:
                bias += 6
                continue
            m = re.match(r'(\w+)\(([^)]*)\)\(([^)]*)\)', line)
            if m:
                name = m.group(1)
                args = [a.strip() for a in m.group(2).split(",") if a.strip()]
                regs = [r.strip().lower() for r in m.group(3).split(",") if r.strip()]
                funcs.append((name, bias, args, regs))
            bias += 6

    return base_name, funcs


def gen_thunk(libname, funcname, bias, args, regs, protos):
    """Generate a C thunk function. Returns None if skipped."""
    if funcname in SKIP_FUNCTIONS:
        return None

    for reg in regs:
        if len(reg) != 2 or reg[0] not in ("a", "d") or not reg[1].isdigit():
            return None

    proto = protos.get(funcname)
    is_void = funcname in VOID_FUNCTIONS

    # If we have proto info, check return type for void
    if proto and not is_void:
        ret_class = classify_return_type(proto[0])
        if ret_class[0] == 'void':
            is_void = True

    # Determine shadow return from proto if not already in SHADOW_RETURNS
    shadow_ret = SHADOW_RETURNS.get(funcname)
    if not shadow_ret and proto and not is_void:
        ret_class = classify_return_type(proto[0])
        if ret_class[0] == 'shadow_create':
            shadow_ret = ret_class[1]

    lines = []
    sig_parts = ", ".join(f"{a}={r}" for a, r in zip(args, regs))
    lines.append(f"/* -{bias}: {funcname}({sig_parts}) */")
    lines.append(f"static IPTR thunk_{libname}_{funcname}(struct M68KEmuContext *ctx, void *cpu)")
    lines.append("{")

    call_args = []
    needs_taglist_free = False
    taglist_args = []

    # Match FD params to proto params by position
    proto_params = proto[1] if proto and len(proto) > 1 else None

    for i, (arg, reg) in enumerate(zip(args, regs)):
        rtype = reg[0]
        rnum = int(reg[1])

        # Get type info from proto if available
        ptype = None
        if proto_params and i < len(proto_params):
            ptype = proto_params[i][0]

        if ptype:
            cls = classify_type(ptype)
        else:
            cls = None

        if cls and cls[0] == 'shadow':
            sname = cls[1]
            if sname == "RastPort":
                lines.append(f"    struct RastPort *arg_{arg} = resolve_rp(ctx, THUNK_A({rnum}));")
            else:
                macro = f"THUNK_A({rnum})" if rtype == "a" else f"THUNK_D({rnum})"
                lines.append(f"    struct {sname} *arg_{arg} = (struct {sname} *)m68k_to_host_or_shadow(ctx, {macro});")
            call_args.append(f"arg_{arg}")

        elif cls and cls[0] == 'input_struct':
            # Functions with input struct parameters need manual thunks
            # because m68k and native struct layouts differ
            return None

        elif cls and cls[0] == 'taglist':
            lines.append(f"    struct TagItem *arg_{arg} = m68k_to_native_taglist(ctx, THUNK_A({rnum}));")
            needs_taglist_free = True
            taglist_args.append(arg)
            call_args.append(f"arg_{arg}")

        elif cls and cls[0] == 'string':
            if rtype == "a":
                lines.append(f"    CONST_STRPTR arg_{arg} = (CONST_STRPTR)m68k_to_host(ctx, THUNK_A({rnum}));")
            else:
                lines.append(f"    CONST_STRPTR arg_{arg} = (CONST_STRPTR)m68k_to_host(ctx, THUNK_D({rnum}));")
            call_args.append(f"arg_{arg}")

        elif cls and cls[0] == 'pointer':
            macro = "THUNK_PTR" if rtype == "a" else "THUNK_DPTR"
            lines.append(f"    APTR arg_{arg} = {macro}({rnum});")
            call_args.append(f"arg_{arg}")

        elif cls and cls[0] == 'scalar':
            lines.append(f"    ULONG arg_{arg} = THUNK_D({rnum});")
            call_args.append(f"arg_{arg}")

        else:
            # No proto info — use fallback (original behavior)
            dptr = DPTR_OVERRIDES.get(funcname, set())
            is_ptr = (rtype == "a") or (reg in dptr)
            if is_ptr:
                if arg.lower() in ("taglist", "tags", "tagitems"):
                    lines.append(f"    struct TagItem *arg_{arg} = m68k_to_native_taglist(ctx, THUNK_A({rnum}));")
                    needs_taglist_free = True
                    taglist_args.append(arg)
                else:
                    macro = "THUNK_PTR" if rtype == "a" else "THUNK_DPTR"
                    lines.append(f"    APTR arg_{arg} = {macro}({rnum});")
            else:
                lines.append(f"    ULONG arg_{arg} = THUNK_D({rnum});")
            call_args.append(f"arg_{arg}")

    call = f"{funcname}({', '.join(call_args)})"
    if is_void:
        lines.append(f"    {call};")
        for ta in taglist_args:
            lines.append(f"    if (arg_{ta}) FreeMem(arg_{ta}, 0);")
        lines.append("    return 0;")
    elif shadow_ret:
        lines.append(f"    void *_ret = (void *){call};")
        for ta in taglist_args:
            lines.append(f"    if (arg_{ta}) FreeMem(arg_{ta}, 0);")
        lines.append(f"    if (!_ret) return 0;")
        lines.append(f'    return shadow_create_by_name(ctx, "{shadow_ret}", _ret);')
    else:
        if needs_taglist_free:
            lines.append(f"    IPTR _ret = (IPTR){call};")
            for ta in taglist_args:
                lines.append(f"    if (arg_{ta}) FreeMem(arg_{ta}, 0);")
            lines.append(f"    return _ret;")
        else:
            lines.append(f"    return (IPTR){call};")

    lines.append("}")
    lines.append("")
    return "\n".join(lines)


def libname_from_fd(fd_path):
    """Extract library name from FD filename."""
    return os.path.basename(fd_path).replace("_lib.fd", "")


def proto_header(libname):
    return f"#include <proto/{libname}.h>"


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <fd_directory> [<ndk_clib_directory>]", file=sys.stderr)
        sys.exit(1)

    fd_dir = sys.argv[1]
    clib_dir = sys.argv[2] if len(sys.argv) > 2 else None

    # Parse proto files for type info
    protos = parse_clib_protos(clib_dir)
    if protos:
        print(f"/* Parsed {len(protos)} prototypes from {clib_dir} */", file=sys.stderr)

    fd_files = sorted(glob.glob(os.path.join(fd_dir, "*_lib.fd")))

    libraries = {}
    for fd_path in fd_files:
        libname = libname_from_fd(fd_path)
        if libname in SKIP_LIBRARIES:
            continue
        base_name, funcs = parse_fd(fd_path)
        if funcs:
            libraries[libname] = (base_name, funcs)

    # Emit header
    print("/* Auto-generated by m68kemu_thunkgen.py — DO NOT EDIT */")
    print("/* Generated from AROS FD files" + (f" + NDK clib protos" if protos else "") + " */")
    print("")
    print("#include <exec/types.h>")
    print("#include <exec/memory.h>")
    print("#include <string.h>")
    print("")

    for libname in sorted(libraries.keys()):
        print(proto_header(libname))
    print("")

    print('#include "m68kemu_intern.h"')
    print('#include "m68kemu_thunks.h"')
    print('#include "m68kemu_shadow.h"')
    print("")

    # resolve_rp and shadow_lookup for proto-aware thunks
    if protos:
        print("/* shadow_lookup is defined in m68kemu_thunks.c */")
        print("extern void *shadow_lookup(struct M68KEmuContext *ctx, ULONG m68k_addr);")
        print("")
        print("/* Shadow-aware RastPort lookup (mirrors m68kemu_thunks.c) */")
        print("static struct RastPort *resolve_rp(struct M68KEmuContext *ctx, ULONG m68k_rp)")
        print("{")
        print("    struct RastPort *rp = (struct RastPort *)shadow_lookup(ctx, m68k_rp);")
        print("    if (rp) return rp;")
        print("    return (struct RastPort *)m68k_to_host(ctx, m68k_rp);")
        print("}")
        print("")

    # Generate thunks per library
    all_tables = {}
    total = 0

    for libname in sorted(libraries.keys()):
        base_name, funcs = libraries[libname]
        thunks = []
        table = []

        for name, bias, args, regs in funcs:
            code = gen_thunk(libname, name, bias, args, regs, protos)
            if code:
                thunks.append(code)
                table.append((bias, f"thunk_{libname}_{name}"))

        if thunks:
            print(f"/* ── {libname}.library ({len(thunks)} thunks) ── */")
            print("")
            for t in thunks:
                print(t)
            all_tables[libname] = table
            total += len(thunks)

    # Emit per-library tables
    print("/* ── Thunk tables ── */")
    print("")

    for libname in sorted(all_tables.keys()):
        table = all_tables[libname]
        print(f"const struct M68KThunkEntry m68kemu_thunks_{libname}_gen[] = {{")
        for bias, func in table:
            print(f"    {{ {bias}, {func} }},")
        print("    { 0, NULL }")
        print("};")
        print(f"const ULONG m68kemu_thunks_{libname}_gen_count = {len(table)};")
        print("")

    # Emit master registration table
    print("/* ── Library registration table ── */")
    print(f"const struct M68KLibThunkSet m68kemu_all_gen_libs[] = {{")
    for libname in sorted(all_tables.keys()):
        print(f'    {{ "{libname}.library", m68kemu_thunks_{libname}_gen, m68kemu_thunks_{libname}_gen_count }},')
    print("    { NULL, NULL, 0 }")
    print("};")
    print(f"const ULONG m68kemu_all_gen_libs_count = {len(all_tables)};")
    print("")
    print(f"/* Total: {total} auto-generated thunks across {len(all_tables)} libraries */")


if __name__ == "__main__":
    main()
