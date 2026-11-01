/*
    Copyright (C) 2002-2026, The AROS Development Team.
    Copyright (C) 1999, David Le Corfec.
    All rights reserved.

*/

#include <exec/types.h>
#include <devices/timer.h>
#include <dos/dostags.h>
#include <dos/datetime.h>
#include <utility/date.h>
#include <prefs/prefhdr.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <clib/alib_protos.h>
#include <libraries/commodities.h>
#include <rexx/errors.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/commodities.h>
#include <proto/muimaster.h>
#include <proto/iffparse.h>
#include <proto/rexxsyslib.h>
#include <proto/workbench.h>
#include <proto/icon.h>

//#define MYDEBUG 1
#include "debug.h"
#include "prefs.h"

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"

#include <string.h>

extern struct Library *MUIMasterBase;


struct TrackingNode
{
    struct MinNode tn_Node;
    Object *tn_Application;
};

struct MUI_ApplicationData
{
    struct MUI_GlobalInfo   app_GlobalInfo;
    APTR                    app_WindowFamily; /* delegates window list */
    struct MinList          app_IHList;
    struct MinList          app_MethodQueue;
    struct SignalSemaphore  app_MethodSemaphore;
    struct MinList          app_ReturnIDQueue;
    struct Hook             *app_BrokerHook;
    struct MsgPort          *app_BrokerPort;
    struct MsgPort          *app_TimerPort;
    struct timerequest      *app_TimerReq;
    struct Task             *app_Task;
    CxObj                   *app_Broker;
    Object                  *app_Menustrip;
    Object                  *app_AboutWin;
    APTR                    app_RIDMemChunk;
    STRPTR                  app_Author;
    STRPTR                  app_Base;
    STRPTR                  app_Copyright;
    STRPTR                  app_Description;
    STRPTR                  app_HelpFile;
    STRPTR                  app_Title;
    STRPTR                  app_Version;
    BOOL                    app_VersionAllocated;
    STRPTR                  app_Version_Number;
    STRPTR                  app_Version_Date;
    STRPTR                  app_Version_Extra;
    ULONG                   app_TimerOutstanding;
    ULONG                   app_MenuAction; /* Remember last action */
    BOOL                    app_ForceQuit;
    BOOL                    app_Iconified;
    BOOL                    app_SingleTask;
    BOOL                    app_Active;
    BYTE                    app_BrokerPri;
    struct TrackingNode     app_TrackingNode;
    BOOL                    app_is_TNode_in_list;
    ULONG searchwinid;
    LONG winposused;       //dont add other vars before windowpos all is save
                           //together
    struct windowpos        winpos[MAXWINS];
    struct MsgPort          *app_RexxPort;
    struct RexxMsg          *app_RexxMsg;
    struct Hook             *app_RexxHook;
    struct MUI_Command      *app_Commands;
    STRPTR                  app_RexxString;
    BOOL                    app_UseRexx;
    struct MsgPort          *app_AppPort; /* Port for AppIcon/AppMenu/AppWindow */
    struct AppIcon          *app_AppIcon;
    struct DiskObject       *app_DiskObject; /* This is only pointer to
                                              * client-managed object */
    struct DiskObject       *app_DefaultDiskObject; /* This is complete
                                                     * object managed by
                                                     * the class */
    struct MsgPort          *app_CfgNotifyPort;   /* notification of changes to the prefs file */
    struct NotifyRequest    app_CfgNotifyRequest;
    STRPTR                  app_CfgNotifyName;
};

struct timerequest_ext
{
    struct timerequest treq;
    struct MUI_InputHandlerNode *ihn;
};

/****** Application.mui/MUIC_Application ************
*
*   NAME
*       MUIC_Application
*
*   SUPERCLASS
*       Notify.mui
*
*   DESCRIPTION
*       Own the application window family, optional menustrip, configuration
*       object, message ports, and event queues. Construction binds the
*       application to the creating task. Process input and perform ordinary
*       object mutations in that task; the pushed-method queue has a separate
*       semaphore but does not protect object lifetimes.
*       Window tags attach owned children. OM_ADDMEMBER appends through the
*       internal Family and connects the child to this application; OM_REMMEMBER
*       disconnects and unlinks without disposing it. Both member methods return
*       TRUE without checking child results. Supply live, correctly owned
*       Window-compatible children and remove them before separate disposal.
*       Application disposal removes and disposes remaining windows, the
*       menustrip, and configuration data, then releases its own resources.
*       Caller-supplied metadata strings, hooks, command tables, and DiskObject
*       remain borrowed unless an attribute explicitly documents copying. Stop
*       input processing and callbacks, remove input handlers, and resolve
*       queued methods before disposal.
*
*   METHODS
*       FindUData and GetUData examine the application first, then delegate to
*       the internal window Family. That Family itself participates in matching
*       before its children, including its default UserData value zero. GetUData
*       returns a match result, not the attribute getter's success.
*       SetUData sets the application on a match, visits the internal Family,
*       and returns TRUE regardless of matches. SetUDataOnce stops at the
*       application or first delegated match. The application menustrip is not
*       traversed by these overrides. Keep membership stable during callbacks.
*       MUIA_ShowMe is also accepted by the setter and propagated to windows.
*       The iteration rereads the window list after each setter and assumes that
*       a processed window does not remove itself. Local MUIA_Version and
*       MUIA_Revision both return 1.
*
*   NOTES
*       Normal construction returns NULL on required allocation, port, timer,
*       configuration, or child validation failure. Optional broker or Rexx-port
*       creation failure does not fail construction. The constructor creates
*       configuration notification for ENV:zune/<base>.prefs when possible;
*       failure to watch that file is not reported. NewInput processes changes
*       and queues configuration replacement.
*       Disposal can perform configuration-file I/O and wait for timer
*       completion. Depending on the configured window-position mode, it
*       snapshots windows and writes ENV preferences; mode 2 also reloads and
*       saves ENVARC preferences to retain persistent settings while updating
*       positions.
*
*   BUGS
*       The early disposal path dereferences mgi_Prefs whenever app_Base is
*       non-NULL. Failure after assigning the base but before preferences are
*       available can therefore access NULL instead of cleanly returning
*       construction failure.
*       Disposal does not drain or free app_MethodQueue and does not enumerate
*       and remove registered timer handlers. Pending method payloads and
*       handler resources must not be assumed to be reclaimed by application
*       disposal. Cross-task queue insertion is not an application-lifetime
*       guarantee.
*
*/
/****** Application.mui/MUIA_Application_Author ************
*
*   NAME
*       MUIA_Application_Author
*
*   FUNCTION
*       Store the application author string. Default NULL.
*
*   INPUTS
*       STRPTR; initialize and get.
*
*   NOTES
*       The pointer is stored without copying and is not freed. Keep the string
*       alive while the application uses it; there is no runtime setter for this
*       attribute.
*
*/
/****** Application.mui/MUIA_Application_Copyright ************
*
*   NAME
*       MUIA_Application_Copyright
*
*   FUNCTION
*       Store the application copyright string. Default NULL.
*
*   INPUTS
*       STRPTR; initialize and get.
*
*   NOTES
*       The pointer is stored without copying and is not freed. Keep the string
*       alive while the application uses it; there is no runtime setter for this
*       attribute.
*
*/
/****** Application.mui/MUIA_Application_Description ************
*
*   NAME
*       MUIA_Application_Description
*
*   FUNCTION
*       Store the application description string. Default "?".
*
*   INPUTS
*       STRPTR; initialize and get.
*
*   NOTES
*       The pointer is stored without copying and is not freed. Keep the string
*       alive while the application uses it; there is no runtime setter for this
*       attribute.
*
*/
/****** Application.mui/MUIA_Application_Title ************
*
*   NAME
*       MUIA_Application_Title
*
*   FUNCTION
*       Store the application title string. Default "Unnamed".
*
*   INPUTS
*       STRPTR; initialize and get.
*
*   NOTES
*       The pointer is stored without copying and is not freed. Keep the string
*       alive while the application uses it; there is no runtime setter for this
*       attribute.
*
*/
/****** Application.mui/MUIA_Application_Version_Number ************
*
*   NAME
*       MUIA_Application_Version_Number
*
*   FUNCTION
*       Store the application version number string. Default NULL.
*
*   INPUTS
*       STRPTR; initialize and get.
*
*   NOTES
*       The pointer is stored without copying and is not freed. Keep the string
*       alive while the application uses it; there is no runtime setter for this
*       attribute. This value participates in version-string construction only
*       when Version is explicitly NULL; Date is not format-validated.
*
*/
/****** Application.mui/MUIA_Application_Version_Date ************
*
*   NAME
*       MUIA_Application_Version_Date
*
*   FUNCTION
*       Store the application version date string. Default NULL.
*
*   INPUTS
*       STRPTR; initialize and get.
*
*   NOTES
*       The pointer is stored without copying and is not freed. Keep the string
*       alive while the application uses it; there is no runtime setter for this
*       attribute. This value participates in version-string construction only
*       when Version is explicitly NULL; Date is not format-validated.
*
*/
/****** Application.mui/MUIA_Application_Version_Extra ************
*
*   NAME
*       MUIA_Application_Version_Extra
*
*   FUNCTION
*       Store the application version extra string. Default NULL.
*
*   INPUTS
*       STRPTR; initialize and get.
*
*   NOTES
*       The pointer is stored without copying and is not freed. Keep the string
*       alive while the application uses it; there is no runtime setter for this
*       attribute. This value participates in version-string construction only
*       when Version is explicitly NULL; Date is not format-validated.
*
*/
/****** Application.mui/MUIA_Application_Version ************
*
*   NAME
*       MUIA_Application_Version
*
*   FUNCTION
*       Store the supplied version pointer. If explicitly set to NULL, and both
*       Title and Version_Number are non-NULL, allocate "$VER: <title> <number>"
*       with optional " (<date>)" and " [<extra>]" suffixes.
*
*   INPUTS
*       STRPTR; initialize and get.
*
*   NOTES
*       A supplied string is borrowed. An automatically generated string is
*       owned by the application and freed at disposal. The getter returns a
*       borrowed pointer in either case. Failed generation leaves NULL without
*       failing construction.
*
*   BUGS
*       Omission does not request generation: the constructor initializes
*       Version to "Unnamed 0.0". This differs from the header comment that
*       promises generation when the attribute is unspecified.
*
*/
/****** Application.mui/MUIA_Application_Base ************
*
*   NAME
*       MUIA_Application_Base
*
*   FUNCTION
*       Choose the base name for application registration, Rexx port naming, and
*       configuration paths. Default "UNNAMED". NULL or any of : / ( ) # ? * ,
*       causes construction failure.
*
*   INPUTS
*       STRPTR; initialize and get.
*
*   NOTES
*       The stored result is an allocated copy, freed at application disposal.
*       With SingleTask FALSE, the constructor tries suffixes .1 through .999
*       against tracked application bases. SingleTask TRUE uses the supplied
*       name without a suffix. The getter returns the actual borrowed stored
*       name.
*
*   BUGS
*       Suffix construction uses a 255-byte buffer and can truncate. Exhaustion
*       of the suffix search is not rejected; this is not an unconditional
*       unique-name guarantee. The validation does not make every accepted name
*       safe as an unquoted shell argument; OpenConfigWindow inserts it without
*       quoting.
*
*/
/****** Application.mui/MUIA_Application_SingleTask ************
*
*   NAME
*       MUIA_Application_SingleTask
*
*   FUNCTION
*       Use a shared unsuffixed base when true; default FALSE. If a tracked
*       application already has the base, queue MUIM_Set of DoubleStart=TRUE on
*       that application and fail this construction.
*
*   INPUTS
*       BOOL; initialize only.
*
*   NOTES
*       The notification is deferred through the other application's
*       pushed-method queue. This attribute does not make the application or its
*       objects safe for concurrent use.
*
*   BUGS
*       The target application's lifetime until the queued notification executes
*       is not retained. Registration and name lookup do not provide a general
*       cross-task lifetime guarantee.
*
*/
/****** Application.mui/MUIA_Application_Active ************
*
*   NAME
*       MUIA_Application_Active
*
*   FUNCTION
*       Store a normalized active flag, default TRUE. Activate or deactivate the
*       Commodities broker when one exists.
*
*   INPUTS
*       BOOL; initialize, set, get.
*
*   NOTES
*       This flag does not disable NewInput processing or put windows to sleep.
*       The ordinary setter forwards tags to Notify after updating broker state.
*
*/
/****** Application.mui/MUIA_Application_Broker ************
*
*   NAME
*       MUIA_Application_Broker
*
*   FUNCTION
*       Return the application-owned Commodities broker, or NULL if none was
*       created.
*
*   INPUTS
*       CxObj pointer; get only.
*
*   NOTES
*       Do not delete it. The application destroys it at disposal.
*       UseCommodities=TRUE does not guarantee a non-NULL result.
*
*/
/****** Application.mui/MUIA_Application_BrokerPort ************
*
*   NAME
*       MUIA_Application_BrokerPort
*
*   FUNCTION
*       Return the application-owned broker message port, or NULL.
*
*   INPUTS
*       MsgPort pointer; get only.
*
*   NOTES
*       Keep the application alive. NewInput consumes and replies to its
*       messages. Do not delete the port or compete with the event loop for its
*       messages. A port can exist even when broker creation failed.
*
*/
/****** Application.mui/MUIA_Application_BrokerPri ************
*
*   NAME
*       MUIA_Application_BrokerPri
*
*   FUNCTION
*       Convert the supplied value to BYTE and use it as the broker priority.
*       Default zero.
*
*   INPUTS
*       LONG in the header; initialize and get.
*
*   NOTES
*       The getter returns the stored BYTE value through IPTR. No setter or
*       range validation is provided.
*
*/
/****** Application.mui/MUIA_Application_BrokerHook ************
*
*   NAME
*       MUIA_Application_BrokerHook
*
*   FUNCTION
*       Store a borrowed hook, initially NULL, called for each broker message
*       after default command handling.
*
*   INPUTS
*       Hook pointer; initialize, set, get.
*
*   NOTES
*       CallHookPkt receives the application as object and the CxMsg pointer as
*       message. Its result is ignored. NewInput replies immediately afterward:
*       do not retain or reply to the message. Disable/Enable change Active;
*       Kill sets SIGBREAKF_CTRL_C. Appear/Disappear have no default action.
*
*/
/****** Application.mui/MUIA_Application_UseCommodities ************
*
*   NAME
*       MUIA_Application_UseCommodities
*
*   FUNCTION
*       Request a Commodities broker and message port when commodities.library
*       is available. Default TRUE.
*
*   INPUTS
*       BOOL; initialize only.
*
*   NOTES
*       Broker creation failure is tolerated. Inspect Broker if the application
*       requires it; this attribute has no getter or setter.
*
*/
/****** Application.mui/MUIA_Application_UseRexx ************
*
*   NAME
*       MUIA_Application_UseRexx
*
*   FUNCTION
*       Request a public Rexx message port named from the uppercased actual
*       Base. Default FALSE.
*
*   INPUTS
*       BOOL; initialize only.
*
*   NOTES
*       A Commands initialization tag also sets the request to TRUE, even for a
*       NULL table. Constructor tag order matters: a later UseRexx tag can
*       override it. Optional port creation failure does not fail the
*       application. Setting Commands later does not create a missing port.
*
*/
/****** Application.mui/MUIA_Application_Commands ************
*
*   NAME
*       MUIA_Application_Commands
*
*   FUNCTION
*       Store a borrowed command table terminated by an entry whose mc_Name is
*       NULL. Default NULL. The constructor also enables the Rexx-port request
*       when this tag occurs.
*
*   INPUTS
*       MUI_Command table pointer; initialize, set, get.
*
*   NOTES
*       Each entry supplies mc_Name, optional ReadArgs mc_Template,
*       mc_Parameters for the IPTR argument array, and a callable mc_Hook. Keep
*       the table, strings, and hooks alive during input processing. A matched
*       hook receives the application and the parsed array, or NULL without a
*       template. Its return value becomes rm_Result1. RexxMsg exposes the
*       current request only during the hook.
*       The dispatcher compares case-insensitively using the length of the
*       incoming command word, so prefixes can match an earlier longer table
*       name. Command tables must not assume exact-name matching.
*
*   BUGS
*       Argument-buffer allocation and ReadArgs failure are not reliably checked
*       before the hook is called. A non-NULL template requires a valid
*       parameter count and suitable storage. Do not assume that hook invocation
*       proves successful parsing or that parsed argument pointers remain valid
*       after the hook.
*
*/
/****** Application.mui/MUIA_Application_RexxHook ************
*
*   NAME
*       MUIA_Application_RexxHook
*
*   FUNCTION
*       Store a borrowed fallback hook for Rexx commands not matched by
*       Commands. Default NULL.
*
*   INPUTS
*       Hook pointer; initialize, set, get.
*
*   NOTES
*       The hook receives the application and the live RexxMsg; its return value
*       becomes rm_Result1. Without it, unmatched requests receive RC_ERROR. The
*       message is replied to after the callback. Keep the hook alive and do not
*       retain or reply to the message yourself.
*
*/
/****** Application.mui/MUIA_Application_RexxMsg ************
*
*   NAME
*       MUIA_Application_RexxMsg
*
*   FUNCTION
*       Return the current Rexx request during a command or fallback hook; NULL
*       outside those callbacks.
*
*   INPUTS
*       RexxMsg pointer; get only.
*
*   NOTES
*       The pointer is borrowed and valid only during the callback. NewInput
*       clears the field before replying to the request.
*
*/
/****** Application.mui/MUIA_Application_RexxString ************
*
*   NAME
*       MUIA_Application_RexxString
*
*   FUNCTION
*       Replace the application-owned copy of the Rexx result string. When
*       RXFF_RESULT is requested, NewInput creates an Argstring from the current
*       copy for the reply.
*
*   INPUTS
*       STRPTR; set only.
*
*   NOTES
*       The previous copy is freed before StrDup reads the supplied pointer; do
*       not pass storage that aliases it. Input must remain valid through the
*       setter. Copy failure leaves no stored string and is not reported by the
*       setter.
*
*   BUGS
*       The stored result string is not automatically reset for each incoming
*       command. Set it deliberately in each callback that supplies a result;
*       otherwise a prior result can be reused. This attribute does not execute
*       an ARexx command.
*
*/
/****** Application.mui/MUIA_Application_DiskObject ************
*
*   NAME
*       MUIA_Application_DiskObject
*
*   FUNCTION
*       Store a borrowed icon used when iconifying. Default NULL; a missing icon
*       triggers fallback lookup when iconification is requested.
*
*   INPUTS
*       DiskObject pointer; initialize, set, get.
*
*   NOTES
*       The application does not free a supplied DiskObject. Keep it alive while
*       an AppIcon uses it. Iconification writes do_CurrentX and do_CurrentY to
*       NO_ICON_POSITION in the supplied object. Replacing the attribute does
*       not rebuild an already installed AppIcon.
*
*/
/****** Application.mui/MUIA_Application_Iconified ************
*
*   NAME
*       MUIA_Application_Iconified
*
*   FUNCTION
*       Treat exactly 1 as iconify and any other value as restore. On a state
*       change, store the flag, propagate inverse ShowMe to windows, and add or
*       remove a Workbench AppIcon.
*
*   INPUTS
*       BOOL; set and get.
*
*   NOTES
*       Initially FALSE. Fallback icons are application-owned; supplied
*       DiskObject remains borrowed. A no-argument AppIcon activation handled by
*       NewInput restores the application.
*
*   BUGS
*       The flag and window visibility are changed before icon creation
*       succeeds. Failed fallback lookup or AddAppIcon does not roll back the
*       iconified state; TRUE is not proof of a visible AppIcon.
*
*/
/****** Application.mui/MUIA_Application_DoubleStart ************
*
*   NAME
*       MUIA_Application_DoubleStart
*
*   FUNCTION
*       A duplicate SingleTask construction queues a setter tag with TRUE on the
*       existing application, which can trigger Notify registrations.
*
*   INPUTS
*       BOOL event; declared get only.
*
*   NOTES
*       No persistent DoubleStart field is stored by Application.
*
*   BUGS
*       The getter returns TRUE without writing the supplied storage. It cannot
*       be used to read a defined Boolean value; use the notification event.
*
*/
/****** Application.mui/MUIA_Application_ForceQuit ************
*
*   NAME
*       MUIA_Application_ForceQuit
*
*   FUNCTION
*       Return the app_ForceQuit field. It is initially FALSE and is never set
*       by this implementation.
*
*   INPUTS
*       BOOL; get only.
*
*   NOTES
*       Commodities Kill sets SIGBREAKF_CTRL_C instead. Do not wait for this
*       attribute as an implemented quit event.
*
*/
/****** Application.mui/MUIA_Application_HelpFile ************
*
*   NAME
*       MUIA_Application_HelpFile
*
*   FUNCTION
*       Store a borrowed help-file pointer, initially NULL.
*
*   INPUTS
*       STRPTR; initialize, set, get.
*
*   NOTES
*       The class does not open it. ShowHelp is not implemented. Keep the string
*       alive while stored; it is not copied or freed.
*
*/
/****** Application.mui/MUIA_Application_MenuAction ************
*
*   NAME
*       MUIA_Application_MenuAction
*
*   FUNCTION
*       Store and return the last application menu-action value, initially zero.
*       The window menu-pick path sets it from the selected item's UserData.
*
*   INPUTS
*       ULONG; declared get only; also accepted by OM_SET.
*
*   NOTES
*       The setter forwards the tag to Notify, so repeated values can trigger
*       notifications. The same window path also queues ReturnID. Values are
*       converted to the ULONG field; this is not pointer-sized storage on
*       64-bit targets.
*
*/
/****** Application.mui/MUIA_Application_Menustrip ************
*
*   NAME
*       MUIA_Application_Menustrip
*
*   FUNCTION
*       Supply the application-wide menu tree, initially NULL. The constructor
*       connects it to this application, and application disposal disposes it.
*
*   INPUTS
*       Object pointer; initialize; getter also implemented.
*
*   NOTES
*       Ownership transfers when the constructor stores the tag. Do not attach
*       the same owned object elsewhere. There is no setter for replacement. The
*       getter returns a borrowed pointer.
*
*/
/****** Application.mui/MUIA_Application_Sleep ************
*
*   NAME
*       MUIA_Application_Sleep
*
*   FUNCTION
*       Set Window_Sleep to TRUE for each current window when the value is
*       nonzero, or FALSE when it is zero.
*
*   INPUTS
*       BOOL; set only.
*
*   NOTES
*       No application-level sleep flag is stored and no getter is implemented.
*       Newly added windows do not inherit a remembered application sleep state.
*       Callback-driven membership changes during this traversal are not
*       protected.
*
*/
/****** Application.mui/MUIA_Application_Window ************
*
*   NAME
*       MUIA_Application_Window
*
*   FUNCTION
*       Append a non-NULL Window-compatible child through OM_ADDMEMBER and
*       connect it to the application.
*
*   INPUTS
*       Object pointer; initialize only; repeatable.
*
*   NOTES
*       The application owns attached windows. A NULL Window tag marks
*       construction failure; after processing tags, disposal removes and
*       disposes attached windows. Earlier construction failure can occur before
*       child tags are processed. Runtime changes use OM_ADDMEMBER and
*       OM_REMMEMBER. Neither operation validates existing membership or
*       propagates child failure results.
*
*/
/****** Application.mui/MUIA_Application_WindowList ************
*
*   NAME
*       MUIA_Application_WindowList
*
*   FUNCTION
*       Return the borrowed child list of the internal window Family.
*
*   INPUTS
*       Pointer to an Exec list; get only.
*
*   NOTES
*       Use NextObject for traversal. Do not free or modify the links directly.
*       Keep the application and membership stable during iteration; the list
*       becomes invalid at disposal.
*
*/
/****** Application.mui/MUIA_Application_UsedClasses ************
*
*   NAME
*       MUIA_Application_UsedClasses
*
*   FUNCTION
*       For each class name, call MUI_GetClass and, if it succeeds,
*       MUI_FreeClass. A NULL array is ignored.
*
*   INPUTS
*       NULL-terminated STRPTR array; initialize only.
*
*   NOTES
*       No class list is retained or returned. Individual failures do not abort
*       construction. Keep the array and strings valid during construction; this
*       is not a guarantee that every named class remains loaded.
*
*/
/****** Application.mui/MUIA_Application_Configdata ************
*
*   NAME
*       MUIA_Application_Configdata
*
*   FUNCTION
*       Queue MUIM_Application_SetConfigdata on this application with the
*       supplied pointer. This setter does not immediately install the object.
*
*   INPUTS
*       Object pointer; set only.
*
*   NOTES
*       Supply a live, distinct Configdata-compatible object and keep it alive
*       until the queued method adopts it. Subsequent replacement and disposal
*       destroy the installed object. The old configuration remains active until
*       execution.
*
*   BUGS
*       Queue failure is ignored by OM_SET, and pending pushed methods are not
*       reclaimed by application disposal. Do not infer successful transfer of
*       ownership from the setter return value. Use the direct method when
*       controlling installation synchronously.
*
*/
/****** Application.mui/MUIA_Application_SearchWinId ************
*
*   NAME
*       MUIA_Application_SearchWinId
*
*   FUNCTION
*       Store the window identifier used by the next GetWinPos query. Initial
*       value zero.
*
*   INPUTS
*       ULONG; set only.
*
*   NOTES
*       This is mutable shared query state, not a parameter to GetWinPos.
*       Serialize the set/get pair. Identifier zero is never searched.
*
*/
/****** Application.mui/MUIA_Application_SetWinPos ************
*
*   NAME
*       MUIA_Application_SetWinPos
*
*   FUNCTION
*       Copy a position record into the first matching occupied slot or the
*       first empty slot. An occupied slot has nonzero w1.
*
*   INPUTS
*       Pointer to struct windowpos; set only.
*
*   NOTES
*       Supply a non-NULL readable record. The first 299 slots of the
*       MAXWINS=300 array are examined. A full examined range silently ignores a
*       new ID. A zero w1 marks the stored slot empty and can terminate later
*       scans. The record is copied, not retained.
*
*/
/****** Application.mui/MUIA_Application_GetWinPos ************
*
*   NAME
*       MUIA_Application_GetWinPos
*
*   FUNCTION
*       Find SearchWinId in occupied slots until the first empty slot and return
*       a borrowed record pointer, or NULL when absent or the query ID is zero.
*
*   INPUTS
*       Pointer to struct windowpos; get only.
*
*   NOTES
*       The getter returns TRUE even for NULL. Records live inside the
*       application and can change through SetWinPos or CopyWinPosToApp; do not
*       free them.
*
*/
/****** Application.mui/MUIA_Application_GetWinPosSize ************
*
*   NAME
*       MUIA_Application_GetWinPosSize
*
*   FUNCTION
*       Count occupied records until the first zero w1 in the first 299 slots.
*       Return zero for none, otherwise sizeof(LONG) plus the record bytes, and
*       store that size in the serialized header.
*
*   INPUTS
*       LONG byte count; get only.
*
*   NOTES
*       This getter mutates winposused. Call it before using GetWinPosAddr to
*       obtain a current serialized header; there is no lock around the pair.
*
*/
/****** Application.mui/MUIA_Application_GetWinPosAddr ************
*
*   NAME
*       MUIA_Application_GetWinPosAddr
*
*   FUNCTION
*       Return a borrowed pointer to the LONG winposused field immediately
*       preceding the internal position array.
*
*   INPUTS
*       APTR; get only.
*
*   NOTES
*       The payload begins with a 32-bit LONG size even on 64-bit systems,
*       followed by native struct windowpos records. Query GetWinPosSize first.
*       No copy or byte-order conversion is made. Keep the application alive and
*       prevent position changes while consuming the bytes.
*
*/
/****** Application.mui/MUIA_Application_CopyWinPosToApp ************
*
*   NAME
*       MUIA_Application_CopyWinPosToApp
*
*   FUNCTION
*       If non-NULL, read the first LONG as a byte count and CopyMem that many
*       bytes into the application beginning at winposused. NULL does nothing.
*
*   INPUTS
*       CONST_APTR; set only.
*
*   NOTES
*       The caller retains the source. Only use a trusted native-format block
*       with a valid nonnegative size, readable source extent, and size no
*       greater than the LONG header plus the 300-record destination array.
*
*   BUGS
*       There is no size, sign, format, or record validation. A malformed block
*       can overwrite instance data; this is not a checked import interface.
*
*/
/****** Application.mui/MUIA_Application_DropObject ************
*
*   NAME
*       MUIA_Application_DropObject
*
*   FUNCTION
*       This attribute is declared but has no Application implementation.
*
*   INPUTS
*       Object pointer; declared initialize and set.
*
*   NOTES
*       The constructor ignores it and the getter has no matching case. Setter
*       tags are forwarded to Notify without an Application state change. The
*       legacy Menu attribute is not an alias for Menustrip.
*
*/
/****** Application.mui/MUIA_Application_MenuHelp ************
*
*   NAME
*       MUIA_Application_MenuHelp
*
*   FUNCTION
*       This attribute is declared but has no Application implementation.
*
*   INPUTS
*       ULONG; declared get only.
*
*   NOTES
*       The constructor ignores it and the getter has no matching case. Setter
*       tags are forwarded to Notify without an Application state change. The
*       legacy Menu attribute is not an alias for Menustrip.
*
*/
/****** Application.mui/MUIA_Application_Menu ************
*
*   NAME
*       MUIA_Application_Menu
*
*   FUNCTION
*       This attribute is declared but has no Application implementation.
*
*   INPUTS
*       NewMenu pointer; declared initialize and get only when MUI_OBSOLETE is
*       enabled.
*
*   NOTES
*       The constructor ignores it and the getter has no matching case. Setter
*       tags are forwarded to Notify without an Application state change. The
*       legacy Menu attribute is not an alias for Menustrip.
*
*/

static const int __version = 1;
static const int __revision = 1;


/*
 * MethodQueueNode
 */
struct MQNode
{
    struct MinNode    mq_Node;
    Object           *mq_Dest;
    LONG              mq_Count;
    IPTR             *mq_Msg;
};

/*
 * FilePrefHeader
 */
struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

#define ID_MUIO MAKE_ID('M','U','I','O')

/*
 * Allocates an MethodQueue Method
 */
/* Allocate one cleared queue node plus count IPTR words and initialize the
 * embedded message pointer. Return owned storage or NULL. count must be
 * nonnegative and fit the allocation arithmetic; this helper does not
 * validate it.
 */
static struct MQNode *CreateMQNode(LONG count)
{
    struct MQNode *mq;

    mq = (struct MQNode *)mui_alloc(sizeof(struct MQNode) +
        (count * sizeof(IPTR)));
    if (!mq)
        return NULL;

    mq->mq_Count = count;
    mq->mq_Msg = (IPTR *) (((char *)mq) + sizeof(struct MQNode));
    return mq;
}

/*
 * Free an IQ Method got from CreateIQMethod()
 */
/* Free an owned queue-node allocation after it has been removed or
 * executed. Do not free its borrowed destination or referenced argument
 * storage. No result is returned.
 */
static void DeleteMQNode(struct MQNode *mq)
{
    mui_free(mq);
}


/*
 * Queue of Return IDs
 */
struct RIDNode
{
    struct MinNode  rid_Node;
    ULONG           rid_Value;
};


/* Allocate a cleared return-ID node, store the ULONG value, and return
 * owned storage or NULL. data is unused; insertion is the caller's
 * responsibility.
 */
static struct RIDNode *CreateRIDNode(struct MUI_ApplicationData *data,
    ULONG retid)
{
    struct RIDNode *rid;

    if ((rid = mui_alloc_struct(struct RIDNode)))
    {
        rid->rid_Value = retid;
    }
    return rid;
}


/* Free a removed owned return-ID node. data is unused. No result or
 * retained state is produced.
 */
static void DeleteRIDNode(struct MUI_ApplicationData *data,
    struct RIDNode *rid)
{
    mui_free(rid);
}


/**************************************************************************
 Process a pushed method.
**************************************************************************/
/* Remove one FIFO node while holding the method semaphore, release the
 * semaphore, execute the message on its borrowed destination, and free the
 * node. Return TRUE if work was executed, FALSE if the queue was empty. The
 * caller must ensure destination and application lifetime; method results
 * are discarded.
 */
static BOOL application_do_pushed_method(struct MUI_ApplicationData *data)
{
    struct MQNode *mq;

    ObtainSemaphore(&data->app_MethodSemaphore);

    if ((mq = (struct MQNode *)RemHead((struct List *)&data->app_MethodQueue)))
    {
        ReleaseSemaphore(&data->app_MethodSemaphore);

        DoMethodA(mq->mq_Dest, (Msg) mq->mq_Msg);
        DeleteMQNode(mq);
        return TRUE;
    }
    ReleaseSemaphore(&data->app_MethodSemaphore);
    return FALSE;
}

/* Search tracked applications under ZuneSemaphore by exact strcmp of non-
 * NULL Base strings. Return a borrowed matching application or NULL. base
 * must be readable and NUL-terminated. Releasing the semaphore does not
 * retain the returned object.
 */
static Object *find_application_by_base(struct IClass *cl, Object *obj,
    STRPTR base)
{
    struct MUIMasterBase_intern *intZuneBase = (struct MUIMasterBase_intern *)MUIMasterBase;
    struct TrackingNode *tn;
    Object *retval = NULL;

    ObtainSemaphore(&intZuneBase->ZuneSemaphore);
    ForeachNode(&intZuneBase->Applications, tn)
    {
        STRPTR tn_base = "";

        get(tn->tn_Application, MUIA_Application_Base, &tn_base);

        if (tn_base && (strcmp(base, tn_base)) == 0)
        {
            retval = tn->tn_Application;
            break;
        }
    }
    ReleaseSemaphore(&intZuneBase->ZuneSemaphore);

    return retval;
}

/**************************************************************************
 Config file notification

 ENV:zune/<appbase>.prefs is watched. Whenever it is written (e.g. by the
 MUI Settings window, sys:prefs/Zune) a fresh Configdata is built from it
 and handed to the application, so the new settings are applied
 immediately without restarting the application.
**************************************************************************/
/* Construct a Configdata bound to this application and queue its
 * installation through the Configdata attribute if allocation succeeds.
 * Return no status. A queue failure is not checked and can leave the new
 * configuration unowned.
 */
static void Application_ReloadConfigdata(struct IClass *cl, Object *obj)
{
    Object *cfg;

    cfg = MUI_NewObject(MUIC_Configdata,
        MUIA_Configdata_Application, obj, TAG_DONE);
    if (cfg)
        set(obj, MUIA_Application_Configdata, cfg);
}

/* Create owned port/name resources and start DOS message notification for
 * the bounded ENV:zune/<base>.prefs path. On local allocation or
 * StartNotify failure, release those resources and leave notification
 * disabled. There is no return status.
 */
static void Application_SetupConfigNotify(struct IClass *cl, Object *obj)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    char name[255];

    if (!data->app_Base)
        return;

    data->app_CfgNotifyName = NULL;
    data->app_CfgNotifyPort = CreateMsgPort();
    if (!data->app_CfgNotifyPort)
        return;

    snprintf(name, sizeof(name), "ENV:zune/%s.prefs", data->app_Base);
    data->app_CfgNotifyName = StrDup(name);
    if (!data->app_CfgNotifyName)
    {
        DeleteMsgPort(data->app_CfgNotifyPort);
        data->app_CfgNotifyPort = NULL;
        return;
    }

    memset(&data->app_CfgNotifyRequest, 0, sizeof(struct NotifyRequest));
    data->app_CfgNotifyRequest.nr_Name = data->app_CfgNotifyName;
    data->app_CfgNotifyRequest.nr_Flags = NRF_SEND_MESSAGE;
    data->app_CfgNotifyRequest.nr_stuff.nr_Msg.nr_Port =
        data->app_CfgNotifyPort;

    if (!StartNotify(&data->app_CfgNotifyRequest))
    {
        FreeVec(data->app_CfgNotifyName);
        data->app_CfgNotifyName = NULL;
        DeleteMsgPort(data->app_CfgNotifyPort);
        data->app_CfgNotifyPort = NULL;
    }
}

/**************************************************************************
 OM_NEW
**************************************************************************/
/* Construct Notify and application resources, parse initialization tags,
 * attach owned windows, and register the application. Return the object or
 * attempt cleanup and return zero on required failure. Optional broker,
 * Rexx port, watcher, and generated-version failures are tolerated. Early
 * cleanup assumes preferences exist whenever Base is non-NULL; see the
 * class BUGS contract.
 */
static IPTR Application__OM_NEW(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct MUI_ApplicationData *data;
    struct TagItem *tags, *tag;
    BOOL bad_childs = FALSE;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    /* Initial local instance data */
    data = INST_DATA(cl, obj);

    /* init input handler list */
    NewList((struct List *)&(data->app_IHList));

    /* init input queue */
    NewList((struct List *)&(data->app_MethodQueue));

    /* init return ids queue */
    NewList((struct List *)&(data->app_ReturnIDQueue));

    /* window list */
    data->app_WindowFamily = MUI_NewObjectA(MUIC_Family, NULL);
    if (!data->app_WindowFamily)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    data->app_GlobalInfo.mgi_ApplicationObject = obj;
    if (!(data->app_GlobalInfo.mgi_WindowsPort = CreateMsgPort()))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    data->app_Task = FindTask(NULL);

    /* Parse prefs */
    data->app_SingleTask =
        (BOOL) GetTagData(MUIA_Application_SingleTask, FALSE,
        msg->ops_AttrList);
    data->app_Base =
        (STRPTR) GetTagData(MUIA_Application_Base, (IPTR) "UNNAMED",
        msg->ops_AttrList);
    if (!data->app_Base || strpbrk(data->app_Base, ":/()#?*,"))
    {
        data->app_Base = NULL;  /* don't remove */
        CoerceMethod(cl, obj, OM_DISPOSE);

        return 0;
    }

    if (!data->app_SingleTask)
    {
        /* must append .1, .2, ... to the base name */
        LONG i;
        char portname[255];

        for (i = 1; i < 1000; i++)
        {
            snprintf(portname, 255, "%s.%d", data->app_Base, (int)i);
            if (!find_application_by_base(cl, obj, portname))
                break;
        }
        data->app_Base = StrDup(portname);
    }
    else
    {
        Object *other_app;

        ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
        if ((other_app = find_application_by_base(cl, obj, data->app_Base)))
        {
            // FIXME: Lifetime of other_app until the queued PushMethod
            // executes is not established here.
            DoMethod(other_app, MUIM_Application_PushMethod,
                (IPTR) other_app, 3, MUIM_Set, MUIA_Application_DoubleStart,
                TRUE);
            data->app_Base = NULL;
        }
        ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);

        data->app_Base = StrDup(data->app_Base);
    }

    if (!data->app_Base)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    data->app_GlobalInfo.mgi_Configdata =
        MUI_NewObject(MUIC_Configdata, MUIA_Configdata_Application, obj,
        TAG_DONE);
    if (!data->app_GlobalInfo.mgi_Configdata)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }
    get(data->app_GlobalInfo.mgi_Configdata, MUIA_Configdata_ZunePrefs,
        &data->app_GlobalInfo.mgi_Prefs);

    /* watch the prefs file so settings changes are applied live */
    Application_SetupConfigNotify(cl, obj);

//    D(bug("muimaster.library/application.c: Message Port created at 0x%p\n",
//    data->app_GlobalInfo.mgi_WindowPort));

    /* Setup timer stuff */
    if (!(data->app_TimerPort = CreateMsgPort()))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    if (!(data->app_TimerReq =
            (struct timerequest *)CreateIORequest(data->app_TimerPort,
                sizeof(struct timerequest))))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    if (OpenDevice(TIMERNAME, UNIT_VBLANK,
            (struct IORequest *)data->app_TimerReq, 0))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    InitSemaphore(&data->app_MethodSemaphore);

    muiNotifyData(obj)->mnd_GlobalInfo = &data->app_GlobalInfo;

    /* parse initial taglist */

    data->app_Active = 1;
    data->app_Title = "Unnamed";
    data->app_Version = "Unnamed 0.0";
    data->app_Description = "?";

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Application_Author:
            data->app_Author = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Base:
            /* moved before config parsing */
            break;

        case MUIA_Application_Copyright:
            data->app_Copyright = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Description:
            data->app_Description = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_HelpFile:
            data->app_HelpFile = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_SingleTask:
            /* moved before config parsing */
            break;

        case MUIA_Application_Title:
            data->app_Title = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Version:
            data->app_Version = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Version_Number:
            data->app_Version_Number = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Version_Date:
            data->app_Version_Date = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Version_Extra:
            data->app_Version_Extra = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Window:
            if (tag->ti_Data)
                DoMethod(obj, OM_ADDMEMBER, tag->ti_Data);
            else
                bad_childs = TRUE;
            break;

        case MUIA_Application_Menustrip:
            data->app_Menustrip = (Object *) tag->ti_Data;
            break;

        case MUIA_Application_BrokerPri:
            data->app_BrokerPri = (BYTE) tag->ti_Data;
            break;

        case MUIA_Application_BrokerHook:
            data->app_BrokerHook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_Application_Active:
            data->app_Active = tag->ti_Data ? TRUE : FALSE;
            break;

        case MUIA_Application_UsedClasses:
            {
                STRPTR *list = (STRPTR *) tag->ti_Data;
                if (!list)
                    break;
                while (*list)
                {
                    struct IClass *icl = MUI_GetClass(*list);
                    if (icl)
                        MUI_FreeClass(icl);
                    ++list;
                }
            }
            break;

        case MUIA_Application_UseRexx:
            data->app_UseRexx = tag->ti_Data ? TRUE : FALSE;
            break;

        case MUIA_Application_Commands:
            data->app_Commands = (struct MUI_Command *)tag->ti_Data;
            /* MUI by default enables Rexx port. Zune does not. Enable port at least
             * if the commands are passed */
            data->app_UseRexx = TRUE;
            break;

        case MUIA_Application_RexxHook:
            data->app_RexxHook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_Application_DiskObject:
            data->app_DiskObject = (struct DiskObject *)tag->ti_Data;
            break;

        }
    }

    /* create MUIA_Application_Version if NULL */
    if (data->app_Version == NULL
        && data->app_Title != NULL && data->app_Version_Number != NULL)
    {
        STRPTR result = NULL;
        ULONG length = 0;

        /* Calculate length */
        length = strlen("$VER: ") + strlen(data->app_Title) + 1 /* space */
            + strlen(data->app_Version_Number) + 1 /* NULL */ ;

        if (data->app_Version_Date != NULL)
        {
            length += 1 /* space */  + 1        /* ( */
                + strlen(data->app_Version_Date) + 1 /* ) */ ;
        }

        if (data->app_Version_Extra != NULL)
        {
            length += 1 /* space */  + 1        /* [ */
                + strlen(data->app_Version_Extra) + 1 /* ] */ ;
        }

        /* Allocate memory */
        result = AllocVec(length, MEMF_ANY);

        if (result != NULL)
        {
            result[0] = '\0';

            /* Format string */
            strlcat(result, "$VER: ", length);
            strlcat(result, data->app_Title, length);
            strlcat(result, " ", length);
            strlcat(result, data->app_Version_Number, length);

            if (data->app_Version_Date != NULL)
            {
                strlcat(result, " (", length);
                strlcat(result, data->app_Version_Date, length);
                strlcat(result, ")", length);
            }

            if (data->app_Version_Extra != NULL)
            {
                strlcat(result, " [", length);
                strlcat(result, data->app_Version_Extra, length);
                strlcat(result, "]", length);
            }

            data->app_Version = result;
            data->app_VersionAllocated = TRUE;
        }

    }

    if (bad_childs)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    if (CxBase
        && GetTagData(MUIA_Application_UseCommodities, TRUE,
            msg->ops_AttrList))
    {
        data->app_BrokerPort = CreateMsgPort();

        if (data->app_BrokerPort)
        {
            struct NewBroker nb;

            nb.nb_Version = NB_VERSION;
            nb.nb_Name =
                data->app_Title ? data->app_Title : (STRPTR) "Unnamed";
            nb.nb_Title =
                data->app_Version ? data->app_Version : (STRPTR) "Unnamed";
            nb.nb_Descr =
                data->app_Description ? data->
                app_Description : (STRPTR) "?";
            nb.nb_Unique = 0;
            nb.nb_Flags = COF_SHOW_HIDE;
            nb.nb_Pri = data->app_BrokerPri;
            nb.nb_Port = data->app_BrokerPort;
            nb.nb_ReservedChannel = 0;

            if (strncmp(nb.nb_Title, "$VER: ", 6) == 0)
                nb.nb_Title += 6;

            data->app_Broker = CxBroker(&nb, 0);

            if (data->app_Broker)
            {
                if (data->app_Active)
                    ActivateCxObj(data->app_Broker, 1);
            }
        }
    }

    if (data->app_UseRexx)
    {
        data->app_RexxPort = CreateMsgPort();
        if (data->app_RexxPort)
        {
            data->app_RexxPort->mp_Node.ln_Name = StrDup(data->app_Base);
            if (data->app_RexxPort->mp_Node.ln_Name != NULL)
            {
                D(bug("[MUI] %s is using REXX!\n", data->app_RexxPort->mp_Node.ln_Name));
                char *i;
                for (i = data->app_RexxPort->mp_Node.ln_Name; *i != '\0';
                    i++)
                {
                    *i = toupper(*i);
                }
                AddPort(data->app_RexxPort);
            }
            else
            {
                DeleteMsgPort(data->app_RexxPort);
                data->app_RexxPort = NULL;
            }
        }
    }

    if (data->app_Menustrip)
        DoMethod(data->app_Menustrip, MUIM_ConnectParent, (IPTR) obj);

    data->app_AppPort = CreateMsgPort();
    data->app_GlobalInfo.mgi_AppPort = data->app_AppPort;
    if (data->app_AppPort == NULL)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    data->app_TrackingNode.tn_Application = obj;
    AddTail((struct List *)&MUIMB(MUIMasterBase)->Applications,
        (struct Node *)&data->app_TrackingNode);
    data->app_is_TNode_in_list = TRUE;
    ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);

    return (IPTR) obj;
}



/**************************************************************************
 OM_DISPOSE
**************************************************************************/
/* After callbacks and external users have stopped, snapshot and optionally
 * save positions, remove and dispose child windows, and release owned
 * application resources before calling Notify disposal. Return the
 * superclass result. This can block on I/O/timer work; it neither frees
 * pending method-queue nodes nor unregisters each timer handler.
 */
static IPTR Application__OM_DISPOSE(struct IClass *cl, Object *obj,
    Msg msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct RIDNode *rid;

    long positionmode;
    if (data->app_Base)
    {
        char filename[255];
        positionmode = data->app_GlobalInfo.mgi_Prefs->window_position;
        if (positionmode >= 1)
        {
            /*
             * Capture the final window positions. Windows that are closed
             * by the application before disposing it (e.g. FryingPan) are
             * already snapshotted in UndisplayWindow, but apps that just
             * dispose the application with its windows still open (e.g.
             * iGame) would otherwise lose their position. MUIM_Window_
             * Snapshot stores the position in the app's winpos array,
             * which Configdata_SetWindowPos re-adds during Save.
             */
            {
                struct MinList *children = NULL;
                Object *cstate;
                Object *child;

                get(data->app_WindowFamily, MUIA_Family_List, &children);
                if (children)
                {
                    cstate = (Object *) children->mlh_Head;
                    while ((child = NextObject(&cstate)))
                        DoMethod(child, MUIM_Window_Snapshot, 1);
                }
            }

            snprintf(filename, 255, "ENV:zune/%s.prefs", data->app_Base);
            DoMethod(data->app_GlobalInfo.mgi_Configdata,
                MUIM_Configdata_Save, (IPTR) filename);
        }
        if (positionmode == 2)
        {
            /*
             * ENVARC holds the settings that survive a reboot, i.e. the
             * ones from the last "Save" in the config window. "Use" only
             * applies the settings for the current session, so do not
             * write the session settings here. Reload the persistent
             * ENVARC content and save it again, which updates only the
             * window positions (Configdata_SetWindowPos re-adds them
             * during Save).
             */
            snprintf(filename, 255, "ENVARC:zune/%s.prefs", data->app_Base);
            DoMethod(data->app_GlobalInfo.mgi_Configdata,
                MUIM_Configdata_Load, (IPTR) filename);
            DoMethod(data->app_GlobalInfo.mgi_Configdata,
                MUIM_Configdata_Save, (IPTR) filename);
        }
    }

    if (data->app_is_TNode_in_list)
    {
        ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
        Remove((struct Node *)&data->app_TrackingNode);
        ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    }

    if (data->app_WindowFamily)
    {
        struct MinList *children = NULL;
        Object *cstate;
        Object *child;

        /* special loop because the next object may have been removed/freed by
         *  the previous. so restart from listhead each time.
         */
        while (1)
        {
            get(data->app_WindowFamily, MUIA_Family_List, &children);
            if (children == NULL)
                break;

            cstate = (Object *) children->mlh_Head;
            if ((child = NextObject(&cstate)))
            {
                D(bug("Application_Dispose(%p) : OM_REMMEMBER(%p)\n", obj,
                        child));
                DoMethod(obj, OM_REMMEMBER, (IPTR) child);
                D(bug("Application_Dispose(%p) : MUI_DisposeObject(%p)\n",
                        obj, child));
                MUI_DisposeObject(child);

            }
            else
            {
                break;
            }
        }
        MUI_DisposeObject(data->app_WindowFamily);
    }

    if (data->app_Menustrip)
        MUI_DisposeObject(data->app_Menustrip);

    if (data->app_VersionAllocated && data->app_Version != NULL)
    {
        FreeVec(data->app_Version);
    }

    /* free commodities stuff */

    if (data->app_Broker)
    {
        DeleteCxObjAll(data->app_Broker);
    }

    if (data->app_BrokerPort)
    {
        struct Message *msg;

        while ((msg = GetMsg(data->app_BrokerPort)))
        {
            ReplyMsg(msg);
        }

        DeleteMsgPort(data->app_BrokerPort);
    }

    /* free timer stuff */
    if (data->app_TimerReq)
    {
        if (data->app_TimerReq->tr_node.io_Device)
        {
            while (data->app_TimerOutstanding)
            {
                if (Wait(1L << data->app_TimerPort->
                        mp_SigBit | 4096) & 4096)
                    break;
                data->app_TimerOutstanding--;
            }
            CloseDevice((struct IORequest *)data->app_TimerReq);
        }
        DeleteIORequest((struct IORequest *)data->app_TimerReq);
    }
    DeleteMsgPort(data->app_TimerPort);

    if (data->app_RexxPort)
    {
        struct Message *msg;
        while ((msg = GetMsg(data->app_RexxPort)))
        {
            ReplyMsg(msg);
        }
        RemPort(data->app_RexxPort);

        FreeVec(data->app_RexxPort->mp_Node.ln_Name);
        DeleteMsgPort(data->app_RexxPort);
    }

    if (data->app_RexxString)
        FreeVec(data->app_RexxString);

    if (data->app_AppIcon)
        RemoveAppIcon(data->app_AppIcon);

    if (data->app_DefaultDiskObject)
        FreeDiskObject(data->app_DefaultDiskObject);

    if (data->app_AppPort)
    {
        struct Message *msg;
        while ((msg = GetMsg(data->app_AppPort)))
            ReplyMsg(msg);

        DeleteMsgPort(data->app_AppPort);
    }

    if (data->app_CfgNotifyPort)
    {
        EndNotify(&data->app_CfgNotifyRequest);
        DeleteMsgPort(data->app_CfgNotifyPort);
    }

    if (data->app_CfgNotifyName)
        FreeVec(data->app_CfgNotifyName);

    if (data->app_GlobalInfo.mgi_Configdata)
        MUI_DisposeObject(data->app_GlobalInfo.mgi_Configdata);

    DeleteMsgPort(data->app_GlobalInfo.mgi_WindowsPort);

    FreeVec(data->app_Base);

    /* free returnid stuff */

    while ((rid =
            (struct RIDNode *)RemHead((struct List *)&data->
                app_ReturnIDQueue)))
    {
        DeleteRIDNode(data, rid);
    }

    return DoSuperMethodA(cl, obj, msg);
}


/**************************************************************************
 OM_SET
**************************************************************************/
/* Apply supported attribute operations in tag order, including copying
 * trusted position data, deferred configuration installation, iconification
 * and window broadcasts. Then forward the same tags to Notify and return
 * its result. No general failure status is collected; pointer ownership
 * follows each attribute contract.
 */
static IPTR Application__OM_SET(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    /* There are many ways to find out what tag items provided by set()
     ** we do know. The best way should be using NextTagItem() and simply
     ** browsing through the list.
     */
    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {

        case MUIA_Application_SearchWinId:
            data->searchwinid = tag->ti_Data;
            break;

        case MUIA_Application_CopyWinPosToApp:
            {
                CONST_APTR addr = (CONST_APTR)tag->ti_Data;
                if (addr) {
                    /*
                     * First element stores the size, and is a 32-bit
                     * integer even on 64-bit systems
                     */
                    LONG wpsize = *(LONG *)(addr);
                    D(bug("[MUI] Copying coords (%ld bytes @ 0x%p) ...\n", wpsize, addr));
                    CopyMem(addr, &data->winposused, wpsize);
                }
            }
            break;

        case MUIA_Application_SetWinPos:
            {
                struct windowpos *winp;
                winp = (struct windowpos *)tag->ti_Data;
                //kprintf("SetWinPos %d %d %d %d %d\n", winp->id, winp->x1,
                //    winp->y1, winp->w1, winp->h1);
                int i;
                for (i = 0; i < MAXWINS - 1; i++)
                {
                    if (data->winpos[i].w1)
                    {
                        if (winp->id == data->winpos[i].id)
                        {
                            //existing entry is overwritten
                            data->winpos[i].x1 = winp->x1;
                            data->winpos[i].y1 = winp->y1;
                            data->winpos[i].w1 = winp->w1;
                            data->winpos[i].h1 = winp->h1;
                            data->winpos[i].x2 = winp->x2;
                            data->winpos[i].y2 = winp->y2;
                            data->winpos[i].w2 = winp->w2;
                            data->winpos[i].h2 = winp->h2;
                            break;
                        }
                    }
                    else
                    {
                        // a new entry is added
                        data->winpos[i].id = winp->id;
                        data->winpos[i].x1 = winp->x1;
                        data->winpos[i].y1 = winp->y1;
                        data->winpos[i].w1 = winp->w1;
                        data->winpos[i].h1 = winp->h1;
                        data->winpos[i].x2 = winp->x2;
                        data->winpos[i].y2 = winp->y2;
                        data->winpos[i].w2 = winp->w2;
                        data->winpos[i].h2 = winp->h2;
                        break;
                    }
                }
            }
            break;

        case MUIA_Application_Configdata:
            DoMethod(obj, MUIM_Application_PushMethod, (IPTR) obj, 2,
                MUIM_Application_SetConfigdata, tag->ti_Data);
            break;

        case MUIA_Application_HelpFile:
            data->app_HelpFile = (STRPTR) tag->ti_Data;
            break;

        case MUIA_Application_Iconified:
            {
                BOOL do_iconify = tag->ti_Data == 1;
                if (data->app_Iconified != do_iconify)
                {
                    data->app_Iconified = do_iconify;

                    nnset(obj, MUIA_ShowMe, !data->app_Iconified);

                    /* Inform workbench.library */
                    if (data->app_Iconified)
                    {
                        STRPTR appname =
                            data->app_Title ? data->
                            app_Title : (STRPTR) "Unnamed";
                        struct DiskObject *dobj =
                            (struct DiskObject *)XGET(obj,
                            MUIA_Application_DiskObject);

                        if (dobj == NULL)
                        {
                            /* Get default AppIcon in ENV:SYS or ENVARC:SYS */
                            dobj = GetDefDiskObject(WBAPPICON);
                            if (dobj)
                                data->app_DefaultDiskObject = dobj;
                            else
                            {
                                /* First default: ENV:SYS/def_MUI.info */
                                dobj = GetDiskObject("ENV:SYS/def_MUI");
                                if (dobj)
                                    data->app_DefaultDiskObject = dobj;
                                else
                                {
                                    /* Second default: ENV:SYS/def_Zune.info */
                                    dobj =
                                        GetDiskObject("ENV:SYS/def_Zune");
                                    if (dobj)
                                        data->app_DefaultDiskObject = dobj;
                                    else
                                    {
                                        /* Third default: default tool icon */
                                        dobj = GetDefDiskObject(WBTOOL);
                                        if (dobj)
                                            data->app_DefaultDiskObject =
                                                dobj;
                                    }
                                }
                            }
                        }

                        if (dobj == NULL)
                            break;

                        dobj->do_CurrentX = NO_ICON_POSITION;
                        dobj->do_CurrentY = NO_ICON_POSITION;

                        data->app_AppIcon =
                            AddAppIconA(0L, 0L, appname, data->app_AppPort,
                            BNULL, dobj, NULL);
                    }
                    else
                    {
                        if (data->app_AppIcon)
                        {
                            RemoveAppIcon(data->app_AppIcon);
                            data->app_AppIcon = NULL;
                        }
                        if (data->app_DefaultDiskObject)
                        {
                            FreeDiskObject(data->app_DefaultDiskObject);
                            data->app_DefaultDiskObject = NULL;
                        }
                    }
                }
            }
            break;

        case MUIA_ShowMe:
            {
                /* Ok ok, you think this stinks? Well, think of it as
                   an attribute belonging to an interface which
                   MUIC_Application, together with MUIC_Area and a few
                   others implement. It makes sense now, yes? */
                struct List *wlist = NULL;
                APTR wstate;
                Object *curwin = NULL;
                Object *lastwin = NULL;

                /* MUIA_ShowMe can cause MUIM_Setup/MUIM_Cleanup to be issued.
                 * On the other hand it is allowed to add/remove other
                 * application windows in MUIM_Setup/MUIM_Cleanup.
                 * This means after processing a window from internal list,
                 * the list needs to be re-read and iteration started again,
                 * because wstate can become invalid.
                 * Note: The code below assumes that the window won't remove
                 * itself from the list.
                 */

                while (1)
                {
                    get(data->app_WindowFamily, MUIA_Family_List, &wlist);
                    wstate = (Object *) wlist->lh_Head;
                    while ((curwin = NextObject(&wstate)))
                    {
                        if (lastwin == NULL)
                            break;
                        if (curwin == lastwin)
                        {
                            curwin = NextObject(&wstate);
                            break;
                        }
                    }

                    /* This is the window to be processed */
                    if (curwin)
                    {
                        set(curwin, MUIA_ShowMe, tag->ti_Data);
                        lastwin = curwin;
                    }
                    else
                    {
                        /* No more windows */
                        break;
                    }
                }
            }
            break;

        case MUIA_Application_Sleep:
            {
                struct List *wlist = NULL;
                APTR wstate;
                Object *curwin;

                get(obj, MUIA_Application_WindowList, &wlist);
                if (wlist)
                {
                    wstate = wlist->lh_Head;
                    while ((curwin = NextObject(&wstate)))
                    {
                        set(curwin, MUIA_Window_Sleep, tag->ti_Data ? TRUE : FALSE);
                    }
                }
            }
            break;

        case MUIA_Application_MenuAction:
            data->app_MenuAction = tag->ti_Data;
            break;

        case MUIA_Application_BrokerHook:
            data->app_BrokerHook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_Application_Active:
            data->app_Active = tag->ti_Data ? TRUE : FALSE;
            if (data->app_Broker)
            {
                ActivateCxObj(data->app_Broker, data->app_Active);
            }
            break;

        case MUIA_Application_Commands:
            data->app_Commands = (struct MUI_Command *)tag->ti_Data;
            break;

        case MUIA_Application_RexxString:
            FreeVec(data->app_RexxString); /* MUI documentation say that "string in termporarily copied" */
            data->app_RexxString = StrDup((CONST_STRPTR)tag->ti_Data);
            break;

        case MUIA_Application_RexxHook:
            data->app_RexxHook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_Application_DiskObject:
            data->app_DiskObject = (struct DiskObject *)tag->ti_Data;
            break;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}


/*
 * OM_GET
 */
/* Write recognized application fields through writable opg_Storage, or
 * delegate WindowList to the internal Family and unknown IDs to Notify.
 * Pointer results are borrowed. DoubleStart returns TRUE without writing
 * storage; GetWinPosSize updates the serialized size header.
 */
static IPTR Application__OM_GET(struct IClass *cl, Object *obj,
    struct opGet *msg)
{
#define STORE *(msg->opg_Storage)

    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_Application_GetWinPosAddr:
        STORE = (IPTR) & data->winposused;
        return TRUE;

    case MUIA_Application_GetWinPosSize:
        {
            int i, sz = 0;
            for (i = 0; i < MAXWINS - 1; i++)
            {
                if (!data->winpos[i].w1)
                    break;
                sz += sizeof(struct windowpos);
            }
            if (sz > 0) {
                sz += sizeof(LONG);
            }
            data->winposused = sz;
            STORE = sz;
            return (TRUE);
        }

    case MUIA_Application_GetWinPos:
        {
            int i;
            if (data->searchwinid)
            {
                for (i = 0; i < MAXWINS - 1; i++)
                {
                    if (data->winpos[i].w1)
                    {
                        if (data->searchwinid == data->winpos[i].id)
                        {
                            STORE = (IPTR) & data->winpos[i].id;
                            return 1;
                        }
                    }
                    else
                        break;
                }
            }
            STORE = 0;
            return 1;
        }
        return TRUE;

    case MUIA_Version:
        STORE = __version;
        return TRUE;

    case MUIA_Revision:
        STORE = __revision;
        return TRUE;

    case MUIA_Application_Author:
        STORE = (IPTR) data->app_Author;
        return TRUE;

    case MUIA_Application_Base:
        STORE = (IPTR) data->app_Base;
        return TRUE;

    case MUIA_Application_Copyright:
        STORE = (IPTR) data->app_Copyright;
        return TRUE;

    case MUIA_Application_Description:
        STORE = (IPTR) data->app_Description;
        return TRUE;

    case MUIA_Application_DoubleStart:
        return TRUE;

    case MUIA_Application_ForceQuit:
        STORE = (IPTR) data->app_ForceQuit;
        return TRUE;

    case MUIA_Application_HelpFile:
        STORE = (IPTR) data->app_HelpFile;
        return TRUE;

    case MUIA_Application_Iconified:
        STORE = (IPTR) data->app_Iconified;
        return TRUE;

    case MUIA_Application_Title:
        STORE = (IPTR) data->app_Title;
        return TRUE;

    case MUIA_Application_Version:
        STORE = (IPTR) data->app_Version;
        return TRUE;

    case MUIA_Application_Version_Number:
        STORE = (IPTR) data->app_Version_Number;
        return TRUE;

    case MUIA_Application_Version_Date:
        STORE = (IPTR) data->app_Version_Date;
        return TRUE;

    case MUIA_Application_Version_Extra:
        STORE = (IPTR) data->app_Version_Extra;
        return TRUE;

    case MUIA_Application_WindowList:
        return GetAttr(MUIA_Family_List, data->app_WindowFamily,
            msg->opg_Storage);

    case MUIA_Application_Menustrip:
        STORE = (IPTR) data->app_Menustrip;
        return TRUE;

    case MUIA_Application_MenuAction:
        STORE = (IPTR) data->app_MenuAction;
        return TRUE;

    case MUIA_Application_BrokerPort:
        STORE = (IPTR) data->app_BrokerPort;
        return TRUE;

    case MUIA_Application_BrokerPri:
        STORE = (IPTR) data->app_BrokerPri;
        return TRUE;

    case MUIA_Application_BrokerHook:
        STORE = (IPTR) data->app_BrokerHook;
        return TRUE;

    case MUIA_Application_Broker:
        STORE = (IPTR) data->app_Broker;
        return TRUE;

    case MUIA_Application_Active:
        STORE = data->app_Active;
        return TRUE;

    case MUIA_Application_Commands:
        STORE = (IPTR) data->app_Commands;
        return TRUE;

    case MUIA_Application_RexxMsg:
        STORE = (IPTR) data->app_RexxMsg;
        return TRUE;

    case MUIA_Application_RexxHook:
        STORE = (IPTR) data->app_RexxHook;
        return TRUE;

    case MUIA_Application_DiskObject:
        STORE = (IPTR) data->app_DiskObject;
        return TRUE;
    }

    /* our handler didn't understand the attribute, we simply pass
     ** it to our superclass now
     */
    return (DoSuperMethodA(cl, obj, (Msg) msg));
#undef STORE
}


/**************************************************************************
 OM_ADDMEMBER
**************************************************************************/
/* Forward membership insertion to the internal Family, connect the supplied
 * live Window-compatible child to this application, and return TRUE
 * regardless of child results. The window becomes owned when attached;
 * membership and NULL input are not validated here.
 */
static IPTR Application__OM_ADDMEMBER(struct IClass *cl, Object *obj,
    struct opMember *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    D(bug("Application_AddMember: Adding 0x%p to window member list\n",
            msg->opam_Object));

    DoMethodA(data->app_WindowFamily, (Msg) msg);
    /* Application knows its GlobalInfo, so we can inform window */
    DoMethod(msg->opam_Object, MUIM_ConnectParent, (IPTR) obj);
    return TRUE;
}


/**************************************************************************
 OM_REMMEMBER
**************************************************************************/
/* Disconnect the live attached window, forward removal to the internal
 * Family, and return TRUE regardless of child results. The child is not
 * disposed; the caller must manage its later lifetime.
 */
static IPTR Application__OM_REMMEMBER(struct IClass *cl, Object *obj,
    struct opMember *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    D(bug("Application_RemMember: Removing 0x%p from window member list\n",
            msg->opam_Object));

    DoMethod(msg->opam_Object, MUIM_DisconnectParent);
    DoMethodA(data->app_WindowFamily, (Msg) msg);

    return TRUE;
}



/****** Application.mui/MUIM_Application_AddInputHandler ************
*
*   NAME
*       MUIM_Application_AddInputHandler
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_AddInputHandler,
*       (IPTR)ihnode);
*
*   FUNCTION
*       Register a borrowed MUI_InputHandlerNode for signal callbacks or start a
*       repeating timer when MUIIHNF_TIMER is set.
*
*   INPUTS
*       ihnode - Non-NULL, stable, writable node not already registered. Set
*       ihn_Object to a live target and ihn_Method to its no-argument method.
*       For signals set ihn_Signals; for timers set ihn_Millis (UWORD
*       milliseconds).
*
*   RESULT
*       Always TRUE, including timer-request allocation failure.
*
*   NOTES
*       Signal nodes are linked directly into the application list. Timer
*       registration stores an allocated timer-request pointer in
*       ihn_Node.mln_Pred; do not overwrite it. NewInput restarts a completed
*       timer before calling its target. Keep node and target alive until
*       removal has completed.
*
*   BUGS
*       Timer allocation failure leaves no valid registered timer request but
*       still reports TRUE. A later RemInputHandler cannot safely assume success
*       from this return value. No duplicate-registration or interval validation
*       is performed.
*
*/
static IPTR Application__MUIM_AddInputHandler(struct IClass *cl,
    Object *obj, struct MUIP_Application_AddInputHandler *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (msg->ihnode->ihn_Flags & MUIIHNF_TIMER)
    {
        struct timerequest_ext *time_ext =
            (struct timerequest_ext *)AllocVec(sizeof(struct
                timerequest_ext), MEMF_PUBLIC);
        if (time_ext)
        {
            /* Store the request inside the input handler, so that we can
             ** remove the inputhandler without problems */
            msg->ihnode->ihn_Node.mln_Pred = (struct MinNode *)time_ext;

            time_ext->treq = *data->app_TimerReq;
            time_ext->treq.tr_node.io_Command = TR_ADDREQUEST;
            time_ext->treq.tr_time.tv_secs = msg->ihnode->ihn_Millis / 1000;
            time_ext->treq.tr_time.tv_micro =
                (msg->ihnode->ihn_Millis % 1000) * 1000;
            time_ext->ihn = msg->ihnode;
            SendIO((struct IORequest *)time_ext);
        }
    }
    else
        AddTail((struct List *)&data->app_IHList,
            (struct Node *)msg->ihnode);
    return TRUE;
}


/****** Application.mui/MUIM_Application_RemInputHandler ************
*
*   NAME
*       MUIM_Application_RemInputHandler
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_RemInputHandler,
*       (IPTR)ihnode);
*
*   FUNCTION
*       Unlink a registered signal node, or abort if necessary, wait for, and
*       free its timer request.
*
*   INPUTS
*       ihnode - The same node successfully registered with AddInputHandler. Its
*       flags and internal timer pointer must remain intact.
*
*   RESULT
*       Always TRUE after removal.
*
*   NOTES
*       Timer removal can wait for I/O completion. The caller owns the node and
*       target. A timer callback can remove itself because NewInput resubmits
*       before invoking it.
*
*   BUGS
*       An unregistered node, repeated removal, or a failed timer registration
*       is not checked. Signal-list iteration does not protect a callback that
*       removes its current node and frees it before NewInput advances.
*
*/
static IPTR Application__MUIM_RemInputHandler(struct IClass *cl,
    Object *obj, struct MUIP_Application_RemInputHandler *msg)
{
    //struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    if (msg->ihnode->ihn_Flags & MUIIHNF_TIMER)
    {
        struct timerequest_ext *time_ext =
            (struct timerequest_ext *)msg->ihnode->ihn_Node.mln_Pred;
        if (!CheckIO((struct IORequest *)time_ext))
            AbortIO((struct IORequest *)time_ext);
        WaitIO((struct IORequest *)time_ext);
        FreeVec(time_ext);
    }
    else
        Remove((struct Node *)msg->ihnode);

    return TRUE;
}


/* Window event consumer defined in window.c. Pass a dequeued live
 * IntuiMessage; the callee processes and replies to it. Do not reuse it
 * after the call or reply a second time.
 */
void _zune_window_message(struct IntuiMessage *imsg);   /* from window.c */

/****** Application.mui/MUIM_Application_InputBuffered ************
*
*   NAME
*       MUIM_Application_InputBuffered
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_InputBuffered);
*
*   FUNCTION
*       Drain pushed methods, then process at most one queued window
*       IntuiMessage.
*
*   INPUTS
*       No arguments beyond MethodID.
*
*   RESULT
*       Always TRUE.
*
*   NOTES
*       The window handler replies to the consumed message. This method does not
*       drain all window events, process timer/Rexx/broker/AppMessage sources,
*       or return queued ReturnIDs. Continuously self-replenishing pushed
*       methods can prevent completion.
*
*/
static IPTR Application__MUIM_InputBuffered(struct IClass *cl, Object *obj,
    struct MUIP_Application_InputBuffered *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct IntuiMessage *imsg;

    /* process all pushed methods */
    while (application_do_pushed_method(data))
        ;

    imsg =
        (struct IntuiMessage *)GetMsg(data->app_GlobalInfo.mgi_WindowsPort);
    if (imsg != NULL)
    {
        /* Let window object process message */
        _zune_window_message(imsg);     /* will reply the message */
    }
    return TRUE;
}

/****** Application.mui/MUIM_Application_NewInput ************
*
*   NAME
*       MUIM_Application_NewInput
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_NewInput, (IPTR)signals);
*
*   FUNCTION
*       Process pending pushed methods, handle selected event sources, drain
*       newly pushed methods, and remove at most one ReturnID from its FIFO.
*
*   INPUTS
*       signals - Non-NULL readable and writable ULONG pointer. On entry supply
*       the signals returned by Wait, or zero for polling all current sources.
*       On return it contains the computed wait mask.
*       obj - Live application processed in its creating task.
*
*   RESULT
*       The next queued ULONG ReturnID, or zero when none is queued. A queued
*       zero is indistinguishable from no ID.
*
*   NOTES
*       This method itself does not call Wait. Callbacks and invoked operations
*       can still block. A zero input mask treats all registered signal sources
*       as signaled, including user handlers; it does not test which handler
*       signals actually occurred.
*       The wait mask is computed before event callbacks and the final queue
*       drain. Callback changes to registrations are not reflected until a later
*       call. Pending ReturnIDs do not themselves force a zero wait mask. A
*       pushed method can enqueue more work, extending the drain indefinitely.
*
*   BUGS
*       Do not dispose of this application while NewInput is active. Keep
*       handler and window traversals valid through callbacks. The command-table
*       path has unchecked allocation/parsing failures; see Commands.
*
*/
static IPTR Application__MUIM_NewInput(struct IClass *cl, Object *obj,
    struct MUIP_Application_NewInput *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct RIDNode *rid;
    ULONG retval = 0;
    ULONG signal, signalmask;
    ULONG handler_mask = 0;     /* the mask of the signal handlers */
    struct MinNode *mn;

    //struct MinNode ihn_Node;

    signal = *msg->signal;

    /* process all pushed methods */
    while (application_do_pushed_method(data))
        /* nothing */ ;

    /* query the signal for the handlers */
    for (mn = data->app_IHList.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
    {
        struct MUI_InputHandlerNode *ihn;
        ihn = (struct MUI_InputHandlerNode *)mn;
        handler_mask |= ihn->ihn_Signals;
    }

    signalmask = (1L << data->app_GlobalInfo.mgi_WindowsPort->mp_SigBit)
        | (1L << data->app_TimerPort->mp_SigBit) | handler_mask;

    if (data->app_Broker)
        signalmask |= (1L << data->app_BrokerPort->mp_SigBit);

    if (data->app_RexxPort)
        signalmask |= (1L << data->app_RexxPort->mp_SigBit);

    if (data->app_AppPort)
        signalmask |= (1L << data->app_AppPort->mp_SigBit);

    if (data->app_CfgNotifyPort)
        signalmask |= (1L << data->app_CfgNotifyPort->mp_SigBit);

    if (signal == 0)
    {
        /* Stupid app which (always) passes 0 in signals. It's impossible to
           know which signals were really set as the app will already have
           called Wait() which has cleared the task's tc_SigRecvd. So assume
           the window, timer, and broker signals all to be set. Also all of
           the inputhandler signals (MUI does that too). */

        signal = signalmask;
    }

    if (signal & signalmask)
    {
        if (signal & (1L << data->app_GlobalInfo.mgi_WindowsPort->
                mp_SigBit))
        {
            struct IntuiMessage *imsg;
            /* process all pushed methods */

            while ((imsg =
                    (struct IntuiMessage *)GetMsg(data->app_GlobalInfo.
                        mgi_WindowsPort)))
            {
                /* Let window object process message */
                _zune_window_message(imsg);     /* will reply the message */
            }
        }

        if (signal & (1L << data->app_TimerPort->mp_SigBit))
        {
            struct timerequest_ext *time_ext;
            struct Node *n;
            struct List list;
            NewList(&list);

            /* At first we fetch all messages from the message port and store
             ** them in a list, we use the node of the Message here */
            while ((time_ext =
                    (struct timerequest_ext *)GetMsg(data->app_TimerPort)))
                AddTail(&list, (struct Node *)time_ext);

            /* Now we proccess the list and resend the timer io, no loop can
             ** happen. We use RemHead() because the handler can remove it
             ** itself and so a FreeVec() could happen in
             ** MUIM_Application_RemInputHandler which would destroy the
             ** ln->Succ of course */
            while ((n = RemHead(&list)))
            {
                struct timerequest_ext *time_ext =
                    (struct timerequest_ext *)n;
                struct MUI_InputHandlerNode *ihn = time_ext->ihn;
                time_ext->treq.tr_time.tv_secs =
                    time_ext->ihn->ihn_Millis / 1000;
                time_ext->treq.tr_time.tv_micro =
                    (time_ext->ihn->ihn_Millis % 1000) * 1000;
                SendIO((struct IORequest *)&time_ext->treq);
                DoMethod(ihn->ihn_Object, ihn->ihn_Method);
            }
        }

        if (data->app_BrokerPort
            && (signal & (1L << data->app_BrokerPort->mp_SigBit)))
        {
            CxMsg *msg;

            while ((msg = (CxMsg *) GetMsg(data->app_BrokerPort)))
            {
                switch (CxMsgType(msg))
                {
                case CXM_COMMAND:
                    switch (CxMsgID(msg))
                    {
                    case CXCMD_DISABLE:
                        set(obj, MUIA_Application_Active, FALSE);
                        break;

                    case CXCMD_ENABLE:
                        set(obj, MUIA_Application_Active, TRUE);
                        break;

                    case CXCMD_APPEAR:
                    case CXCMD_DISAPPEAR:
                        /* No default handling - application needs to be in
                         * control of this */
                        break;

                    case CXCMD_KILL:
                        SetSignal(SIGBREAKF_CTRL_C, SIGBREAKF_CTRL_C);
                        break;
                    }
                    break;
                }

                if (data->app_BrokerHook)
                {
                    CallHookPkt(data->app_BrokerHook, obj, msg);
                }

                ReplyMsg((struct Message *)msg);
            }
        }

        if (data->app_RexxPort
            && (signal & (1L << data->app_RexxPort->mp_SigBit)))
        {

            D(bug("[MUI] Got Rexx message!\n"));
            struct RexxMsg *rmsg;
            while ((rmsg = (struct RexxMsg *)GetMsg(data->app_RexxPort)))
            {
                if (IsRexxMsg(rmsg))
                {
                    STRPTR commandstr = (STRPTR)rmsg->rm_Args[0];
                    struct MUI_Command *cmd = data->app_Commands;
                    BOOL matched = FALSE;

                    while (cmd != NULL && cmd->mc_Name)
                    {
                        LONG len = 0;
                        while ((commandstr[len] != ' ') && (commandstr[len] != '\0')) len++;

                        if (Strnicmp(commandstr, cmd->mc_Name, len) == 0)
                        {
                            IPTR args[cmd->mc_Parameters];
                            struct RDArgs *rdargs = NULL, *rdargsret = NULL;
                            if (cmd->mc_Template)
                            {
                                STRPTR tmp = commandstr + len;
                                STRPTR arguments;

                                while (*tmp == ' ') tmp++;
                                len = strlen(tmp);
                                arguments = AllocVec(len + 2, MEMF_ANY);
                                CopyMem(tmp, arguments, len);
                                arguments[len]      = '\n';
                                arguments[len + 1]  = '\0';

                                if ((rdargs = AllocDosObject(DOS_RDARGS, NULL)))
                                {
                                    rdargs->RDA_Source.CS_Buffer = arguments;
                                    rdargs->RDA_Source.CS_Length = strlen(arguments);
                                    rdargs->RDA_Source.CS_CurChr = 0;
                                    rdargs->RDA_DAList = 0;
                                    rdargs->RDA_Buffer = NULL;
                                    rdargs->RDA_BufSiz = 0;
                                    rdargs->RDA_ExtHelp = NULL;
                                    rdargs->RDA_Flags = 0;

                                    memset(args, 0, cmd->mc_Parameters * sizeof(IPTR));

                                    rdargsret = ReadArgs(cmd->mc_Template, args, rdargs);
                                }

                                FreeVec(arguments);
                            }

                            data->app_RexxMsg = rmsg;
                            IPTR res = CallHookPkt(cmd->mc_Hook, obj, cmd->mc_Template ? args : NULL);
                            data->app_RexxMsg = NULL;

                            if (rdargsret)
                                FreeArgs(rdargsret);

                            if (rdargs)
                                FreeDosObject(DOS_RDARGS, rdargs);

                            rmsg->rm_Result1 = (LONG)res;
                            if ((rmsg->rm_Action & RXFF_RESULT) && data->app_RexxString)
                            {
                                rmsg->rm_Result2 = (IPTR)CreateArgstring(data->app_RexxString, strlen(data->app_RexxString));
                            }

                            matched = TRUE;
                            break;
                        }
                        cmd++;
                    }

                    if (!matched)
                    {
                        if (data->app_RexxHook)
                        {
                            data->app_RexxMsg = rmsg;
                            rmsg->rm_Result1 = (LONG)CallHookPkt(data->app_RexxHook, obj, rmsg);
                            data->app_RexxMsg = NULL;
                            if ((rmsg->rm_Action & RXFF_RESULT) && data->app_RexxString)
                            {
                                rmsg->rm_Result2 = (IPTR)CreateArgstring(data->app_RexxString, strlen(data->app_RexxString));
                            }
                        }
                        else
                            rmsg->rm_Result1 = RC_ERROR;
                    }
                }

                ReplyMsg((struct Message *)rmsg);
            }
        }

        if (data->app_CfgNotifyPort
            && (signal & (1L << data->app_CfgNotifyPort->mp_SigBit)))
        {
            struct NotifyMessage *nmsg;

            /* only act if a notification really happened */
            if ((nmsg =
                    (struct NotifyMessage *)GetMsg(data->app_CfgNotifyPort)))
            {
                /* reply (and thereby free) all notification messages */
                do {
                    ReplyMsg((struct Message *)nmsg);
                } while ((nmsg =
                        (struct NotifyMessage *)GetMsg(data->app_CfgNotifyPort)));

                /* the prefs file has been rewritten (e.g. by the MUI Settings
                 * window) - rebuild the configdata from it and apply it */
                Application_ReloadConfigdata(cl, obj);
            }
        }

        if (data->app_AppPort
            && (signal & (1L << data->app_AppPort->mp_SigBit)))
        {
            struct AppMessage *appmsg;
            while ((appmsg =
                    (struct AppMessage *)GetMsg(data->app_AppPort)))
            {
                if ((appmsg->am_Type == AMTYPE_APPICON)
                    && (appmsg->am_NumArgs == 0)
                    && (appmsg->am_ArgList == NULL)
                    && (XGET(obj, MUIA_Application_Iconified) == TRUE))
                {
                    /* Reply before removing AppIcon */
                    ReplyMsg((struct Message *)appmsg);
                    set(obj, MUIA_Application_Iconified, FALSE);
                    continue;
                }
                else if (appmsg->am_Type == AMTYPE_APPWINDOW)
                {
                    set((Object *) appmsg->am_UserData, MUIA_AppMessage,
                        appmsg);
                }

                ReplyMsg((struct Message *)appmsg);
            }
        }

        if (signal & handler_mask)
        {
            for (mn = data->app_IHList.mlh_Head; mn->mln_Succ;
                mn = mn->mln_Succ)
            {
                struct MUI_InputHandlerNode *ihn;
                ihn = (struct MUI_InputHandlerNode *)mn;
                if (signal & ihn->ihn_Signals)
                    DoMethod(ihn->ihn_Object, ihn->ihn_Method);
            }
        }
    }

    /* process all pushed methods - again */
    while (application_do_pushed_method(data))
        /* nothing */ ;

    *msg->signal = signalmask;

    /* set return code */
    if ((rid =
            (struct RIDNode *)RemHead((struct List *)&data->
                app_ReturnIDQueue)))
    {
        retval = rid->rid_Value;
        DeleteRIDNode(data, rid);
        return retval;
    }
    return 0;
}

/****** Application.mui/MUIM_Application_Input ************
*
*   NAME
*       MUIM_Application_Input
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_Input, (IPTR)signals);
*
*   FUNCTION
*       Construct a polling mask and invoke NewInput. This is the legacy input
*       method.
*
*   INPUTS
*       signals - Writable ULONG pointer; the incoming value is replaced.
*
*   RESULT
*       The ReturnID result from NewInput; the location receives its wait mask.
*
*   NOTES
*       Use NewInput for signal-driven processing.
*
*   BUGS
*       This implementation combines handler flags rather than ihn_Signals and
*       omits the broker signal from its constructed mask. It does not
*       faithfully poll every registered source.
*
*/
static IPTR Application__MUIM_Input(struct IClass *cl, Object *obj,
    struct MUIP_Application_Input *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    ULONG signal = 0, handler_mask = 0;
    struct MinNode *mn;

    /* query the signal for the handlers */
    for (mn = data->app_IHList.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
    {
        struct MUI_InputHandlerNode *ihn;
        ihn = (struct MUI_InputHandlerNode *)mn;
        handler_mask |= ihn->ihn_Flags;
    }

    signal = (1L << data->app_GlobalInfo.mgi_WindowsPort->mp_SigBit)
        | (1L << data->app_TimerPort->mp_SigBit) | handler_mask;

    if (data->app_RexxPort)
        signal |= (1L << data->app_RexxPort->mp_SigBit);

    if (data->app_AppPort)
        signal |= (1L << data->app_AppPort->mp_SigBit);

    if (data->app_CfgNotifyPort)
        signal |= (1L << data->app_CfgNotifyPort->mp_SigBit);

    *msg->signal = signal;
    return Application__MUIM_NewInput(cl, obj, (APTR) msg);
}

/****** Application.mui/MUIM_Application_PushMethod ************
*
*   NAME
*       MUIM_Application_PushMethod
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_PushMethod, (IPTR)dest,
*       count, ...);
*
*   FUNCTION
*       Copy message words into a FIFO entry, enqueue it under the method
*       semaphore, and signal the creating task using the window-port signal.
*
*   INPUTS
*       dest - Live destination object to keep alive until execution or
*       cancellation.
*       count - LONG whose low four bits specify the number of following IPTR
*       words, including MethodID. Use 1 through 15.
*       ... - Destination MethodID followed by its message arguments.
*
*   RESULT
*       An opaque IPTR queue-entry token, or zero on allocation failure. The
*       token is valid only while that entry remains queued.
*
*   NOTES
*       Only words are copied; referenced storage and objects remain borrowed.
*       NewInput and InputBuffered execute queued messages outside the semaphore
*       and discard their results. There is no completion acknowledgement.
*       Cancel pending work before destroying its target; cancellation cannot
*       stop a method already dequeued.
*
*   BUGS
*       Count is masked with 0xf, not range-checked. Zero creates an entry
*       without a MethodID; 16 becomes zero. The source explicitly notes that
*       extracting words after count is broken on 64-bit big-endian systems.
*       Disposal does not free outstanding queue entries.
*
*/
static IPTR Application__MUIM_PushMethod(struct IClass *cl, Object *obj,
    struct MUIP_Application_PushMethod *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct MQNode *mq;
    LONG i;
    IPTR *m = (IPTR *) &msg->count; /* FIXME: breaks on 64-bit BigEndian systems */
    LONG count;

    count = msg->count & 0xf; /* MUI4 uses count to pass additional info */

    mq = CreateMQNode(count);
    if (!mq)
        return 0;
    mq->mq_Dest = msg->dest;

    /* fill msg */
    for (i = 0; i < count; i++)
        mq->mq_Msg[i] = *(m + 1 + i);

    /* enqueue method */
    ObtainSemaphore(&data->app_MethodSemaphore);
    AddTail((struct List *)&data->app_MethodQueue, (struct Node *)mq);
    ReleaseSemaphore(&data->app_MethodSemaphore);

    /* CHECKME: to wake task up as soon as possible! */
    Signal(data->app_Task,
        1L << data->app_GlobalInfo.mgi_WindowsPort->mp_SigBit);

    return (IPTR)mq;
}

/****** Application.mui/MUIM_Application_UnpushMethod ************
*
*   NAME
*       MUIM_Application_UnpushMethod
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_UnpushMethod, (IPTR)dest,
*       methodid, method);
*
*   FUNCTION
*       Remove and free every queued method matching all supplied filters.
*
*   INPUTS
*       dest - Destination pointer or NULL for any destination.
*       methodid - Queue-entry token from PushMethod, or zero for any token.
*       method - Destination MethodID, or zero for any method.
*
*   RESULT
*       Number of queued entries removed.
*
*   NOTES
*       The queue is locked while filtering. This neither waits for nor cancels
*       an entry already dequeued for execution. Freed tokens may be reused by
*       allocation; do not retain them as permanent identifiers.
*
*/
static IPTR Application__MUIM_UnpushMethod(struct IClass *cl, Object *obj,
    struct MUIP_Application_UnpushMethod *msg)
{
    D(bug("[Application__MUIM_UnpushMethod] dest %p id %p method %u\n",
        msg->dest, msg->methodid, msg->method));

    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    struct MQNode *current, *next;
    ULONG removed = 0;

    ObtainSemaphore(&data->app_MethodSemaphore);
    ForeachNodeSafe(&data->app_MethodQueue, current, next)
    {
        D(bug("[Application__MUIM_UnpushMethod] examine dest %p id %p "
            "method %u\n",
            current->mq_Dest, current, current->mq_Msg[0]));
        if (((msg->dest == NULL) || (msg->dest == current->mq_Dest))
            && ((msg->methodid == 0) || (msg->methodid == (IPTR)current))
            && ((msg->method == 0) || (msg->method == current->mq_Msg[0]))
        )
        {
            Remove((struct Node*)current);
            DeleteMQNode(current);
            removed++;
            D(bug("[Application__MUIM_UnpushMethod] current %p removed\n",
                current));
        }
    }
    ReleaseSemaphore(&data->app_MethodSemaphore);

    return removed;
}


/****** Application.mui/MUIM_Application_ReturnID ************
*
*   NAME
*       MUIM_Application_ReturnID
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_ReturnID, retid);
*
*   FUNCTION
*       Append a ULONG return value to the application ReturnID FIFO.
*
*   INPUTS
*       retid - ULONG value; MUIV_Application_ReturnID_Quit is the conventional
*       quit value.
*
*   RESULT
*       TRUE if allocation and insertion succeed; FALSE on allocation failure.
*
*   NOTES
*       NewInput removes one ID per call. This method neither signals the
*       application task nor locks the ReturnID queue. Use it in the application
*       task; external tasks can request it through PushMethod while separately
*       ensuring object lifetime.
*
*/
static IPTR Application__MUIM_ReturnID(struct IClass *cl, Object *obj,
    struct MUIP_Application_ReturnID *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct RIDNode *rid;

/*
    if (!data->app_RIDMemChunk)
    {
        data->app_RIDMemChunk =
            g_mem_chunk_create(struct RIDNode, 10, G_ALLOC_AND_FREE);
    }
*/
    rid = CreateRIDNode(data, msg->retid);
    if (!rid)
        return FALSE;
    AddTail((struct List *)&data->app_ReturnIDQueue, (struct Node *)rid);
    return TRUE;
}


/* Match the application UserData first, otherwise delegate to the internal
 * Family including its own UserData. Return a borrowed match pointer or
 * zero. The menustrip is not traversed by this override.
 */
static IPTR Application__MUIM_FindUData(struct IClass *cl, Object *obj,
    struct MUIP_FindUData *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
        return (IPTR) obj;

    return DoMethodA(data->app_WindowFamily, (Msg) msg);
}


/* On an application UserData match, get attr into writable storage and
 * return TRUE without checking the getter result; otherwise delegate to the
 * internal Family. The comparison field is ULONG and the menustrip is not
 * traversed.
 */
static IPTR Application__MUIM_GetUData(struct IClass *cl, Object *obj,
    struct MUIP_GetUData *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
    {
        get(obj, msg->attr, msg->storage);
        return TRUE;
    }
    return DoMethodA(data->app_WindowFamily, (Msg) msg);
}


/* Set the application on a UserData match and forward to the internal
 * Family, then return TRUE regardless of setter results or matches. Keep
 * traversal stable through synchronous notifications.
 */
static IPTR Application__MUIM_SetUData(struct IClass *cl, Object *obj,
    struct MUIP_SetUData *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
        set(obj, msg->attr, msg->val);

    DoMethodA(data->app_WindowFamily, (Msg) msg);
    return TRUE;
}


/* Set the application and return TRUE on a UserData match, ignoring setter
 * success; otherwise delegate to the internal Family and return its first-
 * match result. Keep traversal stable through callbacks.
 */
static IPTR Application__MUIM_SetUDataOnce(struct IClass *cl, Object *obj,
    struct MUIP_SetUDataOnce *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (muiNotifyData(obj)->mnd_UserData == msg->udata)
    {
        set(obj, msg->attr, msg->val);
        return TRUE;
    }
    return DoMethodA(data->app_WindowFamily, (Msg) msg);
}



/****** Application.mui/MUIM_Application_AboutMUI ************
*
*   NAME
*       MUIM_Application_AboutMUI
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_AboutMUI, (IPTR)refwindow);
*
*   FUNCTION
*       Create the Aboutmui window on first use, cache it, and request that it
*       open. Later calls reuse the same window.
*
*   INPUTS
*       refwindow - Optional live Window object used as the reference during
*       first construction; NULL omits the reference tag.
*
*   RESULT
*       Always zero, including construction or opening failure.
*
*   NOTES
*       The Aboutmui constructor attaches the window to this application, which
*       owns it. Later refwindow arguments do not reposition the cached window.
*       Keep a supplied reference window alive while the cached window uses it.
*       Do not dispose of the cached window separately and leave this
*       application alive.
*
*/
static IPTR Application__MUIM_AboutMUI(struct IClass *cl, Object *obj,
    struct MUIP_Application_AboutMUI *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    if (!data->app_AboutWin)
    {
        data->app_AboutWin = AboutmuiObject,
            msg->refwindow ? MUIA_Window_RefWindow : TAG_IGNORE,
            msg->refwindow, MUIA_Window_LeftEdge,
            MUIV_Window_LeftEdge_Centered, MUIA_Window_TopEdge,
            MUIV_Window_TopEdge_Centered, MUIA_Aboutmui_Application, obj,
            End;
    }

    if (data->app_AboutWin)
        set(data->app_AboutWin, MUIA_Window_Open, TRUE);

    return 0;
}


/* Close each currently open window and try to collect its borrowed pointer
 * in ln_Name of an owned AllocVec node/list. Return that list or NULL.
 * List/node allocation failure does not stop closing and can lose reopening
 * information. The eventual OpenWindows call consumes the allocations; keep
 * all referenced windows alive.
 */
static struct List *Application__CloseWindows(struct IClass *cl, Object *obj)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct MinList *children = NULL;
    struct List *windows = NULL;
    Object *cstate;
    Object *child;

    get(data->app_WindowFamily, MUIA_Family_List, &children);
    if (!children)
        return NULL;

    /* remember which windows were open, so they can be re-opened after a
     * configdata change. Windows that were already closed are left alone. */
    windows = AllocVec(sizeof(struct List), MEMF_CLEAR);
    if (windows)
        NewList(windows);

    cstate = (Object *) children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        if (XGET(child, MUIA_Window_Open))
        {
            if (windows)
            {
                struct Node *n = AllocVec(sizeof(struct Node), MEMF_CLEAR);

                if (n)
                {
                    n->ln_Name = (STRPTR)child;
                    AddTail(windows, n);
                }
            }
            D(bug("[MUI:App] %s: closing window %p\n", __func__, child));
            set(child, MUIA_Window_Open, FALSE);
        }
    }
    return windows;
}

/****** Application.mui/MUIM_Application_SetConfigdata ************
*
*   NAME
*       MUIM_Application_SetConfigdata
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_SetConfigdata,
*       (IPTR)configdata);
*
*   FUNCTION
*       Close currently open windows, dispose of the old configuration, install
*       the new object, read its ZunePrefs pointer, and queue reopening.
*
*   INPUTS
*       configdata - Live Configdata-compatible object, distinct from the
*       currently installed object. Ownership transfers to the application when
*       installed.
*
*   RESULT
*       Always zero, without an installation or reopening success indication.
*
*   NOTES
*       Previously borrowed configuration/prefs pointers become invalid. The
*       method closes windows immediately but reopens them through PushMethod.
*       The new configuration must provide valid ZunePrefs.
*
*   BUGS
*       No NULL, identity, or getter-success check is performed before adopting
*       the replacement. Passing the old object disposes it before reuse. If
*       saving the open-window list fails, NULL later means reopen all windows;
*       failed node allocation can omit individual windows. Queue failure can
*       leave windows closed and leak the saved list.
*
*/
static IPTR Application__MUIM_SetConfigdata(struct IClass *cl, Object *obj,
    struct MUIP_Application_SetConfigdata *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct List *windows;

    /* close all currently open windows and remember them, so they can be
     * re-opened with the new settings afterwards */
    windows = Application__CloseWindows(cl, obj);

    if (data->app_GlobalInfo.mgi_Configdata)
        MUI_DisposeObject(data->app_GlobalInfo.mgi_Configdata);
    data->app_GlobalInfo.mgi_Configdata = msg->configdata;
    get(data->app_GlobalInfo.mgi_Configdata, MUIA_Configdata_ZunePrefs,
        &data->app_GlobalInfo.mgi_Prefs);

    DoMethod(obj, MUIM_Application_PushMethod, (IPTR) obj, 2,
        MUIM_Application_OpenWindows, (IPTR) windows);
    return 0;
}

/****** Application.mui/MUIM_Application_SetConfigItem ************
*
*   NAME
*       MUIM_Application_SetConfigItem
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_SetConfigItem, item,
*       (IPTR)data);
*
*   FUNCTION
*       Set a supported string-valued configuration item with
*       MUIM_Configdata_SetString. Other item IDs do nothing.
*
*   INPUTS
*       item - One of the supported identifiers listed below.
*       data - Non-NULL readable NUL-terminated string.
*       Supported IDs: MUICFG_Keyboard_Press, MUICFG_Keyboard_Toggle,
*       MUICFG_Keyboard_Up, MUICFG_Keyboard_Down, MUICFG_Keyboard_PageUp,
*       MUICFG_Keyboard_PageDown, MUICFG_Keyboard_Top, MUICFG_Keyboard_Bottom,
*       MUICFG_Keyboard_Left, MUICFG_Keyboard_Right, MUICFG_Keyboard_WordLeft,
*       MUICFG_Keyboard_WordRight, MUICFG_Keyboard_LineStart,
*       MUICFG_Keyboard_LineEnd, MUICFG_Keyboard_NextGadget,
*       MUICFG_Keyboard_PrevGadget, MUICFG_Keyboard_GadgetOff,
*       MUICFG_Keyboard_CloseWindow, MUICFG_Keyboard_NextWindow,
*       MUICFG_Keyboard_PrevWindow, MUICFG_Keyboard_Help, MUICFG_Keyboard_Popup,
*       MUICFG_Drag_LMBModifier, MUICFG_Drag_MMBModifier,
*       MUICFG_Iconification_Hotkey, MUICFG_Font_Normal, MUICFG_Font_List,
*       MUICFG_Font_Tiny, MUICFG_Font_Fixed, MUICFG_Font_Title, MUICFG_Font_Big,
*       MUICFG_Font_Button, MUICFG_Font_Knob, MUICFG_String_Background,
*       MUICFG_String_Text, MUICFG_String_ActiveBackground,
*       MUICFG_String_ActiveText, MUICFG_String_Cursor,
*       MUICFG_String_MarkedBackground, MUICFG_String_MarkedText,
*       MUICFG_ActiveObject_Color, MUICFG_PublicScreen.
*
*   RESULT
*       Always zero, including unsupported IDs or ignored underlying failures.
*
*   NOTES
*       Keyboard bindings, drag modifiers, and the iconification hotkey are set
*       directly. Font IDs, String color/pen IDs, ActiveObject_Color, and
*       PublicScreen close currently open windows and queue reopening after the
*       change. Configdata copies the string or removes a value equal to its
*       default. This does not save preferences to disk.
*
*   BUGS
*       The close/reopen path shares SetConfigdata's incomplete-allocation and
*       queue-failure limitations; no rollback or success status is provided.
*
*/
static IPTR Application__MUIM_SetConfigItem(struct IClass *cl, Object *obj,
    struct MUIP_Application_SetConfigItem *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    if (data->app_GlobalInfo.mgi_Configdata) {
        switch (msg->item) {
            /*
            case MUICFG_Window_Spacing_Left:
            case MUICFG_Window_Spacing_Right:
            case MUICFG_Window_Spacing_Top:
            case MUICFG_Window_Buttons:
            case MUICFG_Window_Spacing_Bottom:
            case MUICFG_Window_Positions:
            case MUICFG_Window_Redraw:
            case MUICFG_Window_Refresh:
            case MUICFG_Radio_HSpacing:
            case MUICFG_Radio_VSpacing:
            case MUICFG_Group_HSpacing:
            case MUICFG_Group_VSpacing:
            case MUICFG_Cycle_MenuCtrl_Position:
            case MUICFG_Cycle_MenuCtrl_Level:
            case MUICFG_Cycle_MenuCtrl_Speed:
            case MUICFG_Cycle_Menu_Recessed:
            case MUICFG_Listview_Font_Leading:
            case MUICFG_Listview_Smoothed:
            case MUICFG_Listview_SmoothVal:
            case MUICFG_Listview_Refresh:
            case MUICFG_Listview_Multi:
            case MUICFG_GroupTitle_Position:
            case MUICFG_GroupTitle_Color:
            case MUICFG_Scrollbar_Type:
            case MUICFG_Scrollbar_Arrangement:
            case MUICFG_Balance_Look:
            case MUICFG_Dragndrop_Look:
            case MUICFG_Drag_Autostart:
            case MUICFG_Drag_Autostart_Length:
            case MUICFG_Drag_LeftButton:
            case MUICFG_Drag_MiddleButton:
            case MUICFG_Register_TruncateTitles:
            case MUICFG_Screen_Mode:
            case MUICFG_Screen_Mode_ID:
            case MUICFG_Screen_Width:
            case MUICFG_Screen_Height:
            case MUICFG_PublicScreen_PopToFront:
            case MUICFG_Iconification_ShowIcon:
            case MUICFG_Iconification_ShowMenu:
            case MUICFG_Iconification_OnStartup:
            case MUICFG_Interfaces_EnableARexx:
            case MUICFG_BubbleHelp_FirstDelay:
            case MUICFG_BubbleHelp_NextDelay: */
            case MUICFG_Keyboard_Press:
            case MUICFG_Keyboard_Toggle:
            case MUICFG_Keyboard_Up:
            case MUICFG_Keyboard_Down:
            case MUICFG_Keyboard_PageUp:
            case MUICFG_Keyboard_PageDown:
            case MUICFG_Keyboard_Top:
            case MUICFG_Keyboard_Bottom:
            case MUICFG_Keyboard_Left:
            case MUICFG_Keyboard_Right:
            case MUICFG_Keyboard_WordLeft:
            case MUICFG_Keyboard_WordRight:
            case MUICFG_Keyboard_LineStart:
            case MUICFG_Keyboard_LineEnd:
            case MUICFG_Keyboard_NextGadget:
            case MUICFG_Keyboard_PrevGadget:
            case MUICFG_Keyboard_GadgetOff:
            case MUICFG_Keyboard_CloseWindow:
            case MUICFG_Keyboard_NextWindow:
            case MUICFG_Keyboard_PrevWindow:
            case MUICFG_Keyboard_Help:
            case MUICFG_Keyboard_Popup:
            case MUICFG_Drag_LMBModifier:
            case MUICFG_Drag_MMBModifier:
            case MUICFG_Iconification_Hotkey:
                DoMethod(data->app_GlobalInfo.mgi_Configdata, MUIM_Configdata_SetString, msg->item, msg->data);
                break;
            case MUICFG_Font_Normal:
            case MUICFG_Font_List:
            case MUICFG_Font_Tiny:
            case MUICFG_Font_Fixed:
            case MUICFG_Font_Title:
            case MUICFG_Font_Big:
            case MUICFG_Font_Button:
            case MUICFG_Font_Knob:
            case MUICFG_String_Background:
            case MUICFG_String_Text:
            case MUICFG_String_ActiveBackground:
            case MUICFG_String_ActiveText:
            case MUICFG_String_Cursor:
            case MUICFG_String_MarkedBackground:
            case MUICFG_String_MarkedText:
            case MUICFG_ActiveObject_Color:
            case MUICFG_PublicScreen:
                {
                    struct List *windows;

                    windows = Application__CloseWindows(cl, obj);
                    DoMethod(data->app_GlobalInfo.mgi_Configdata, MUIM_Configdata_SetString, msg->item, msg->data);
                    DoMethod(obj, MUIM_Application_PushMethod, (IPTR) obj, 2,
                        MUIM_Application_OpenWindows, (IPTR) windows);
                }
                break;
        }
    }
    return 0;
}


/****** Application.mui/MUIM_Application_OpenWindows ************
*
*   NAME
*       MUIM_Application_OpenWindows
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_OpenWindows,
*       (IPTR)windows);
*
*   FUNCTION
*       Request opening of selected windows, or all current application windows
*       when windows is NULL.
*
*   INPUTS
*       windows - NULL, or an owned Exec List allocated with AllocVec whose
*       nodes were individually allocated with AllocVec and hold live window
*       pointers in ln_Name.
*
*   RESULT
*       Always zero; window setter results are ignored.
*
*   NOTES
*       A non-NULL list and every removed node are consumed and freed with
*       FreeVec. Do not pass the borrowed Application_WindowList or stack
*       storage. The referenced windows are not freed. For NULL, traverse the
*       internal Family without taking ownership of its list.
*
*/
static IPTR Application__MUIM_OpenWindows(struct IClass *cl, Object *obj,
    struct MUIP_Application_OpenWindows *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);

    /* a list of windows to open was passed (e.g. the ones closed by
     * Application__CloseWindows before a configdata change) - open exactly
     * those and free the list */
    if (msg->windows)
    {
        struct Node *n;

        while ((n = (struct Node *)RemHead(msg->windows)))
        {
            set((Object *)n->ln_Name, MUIA_Window_Open, TRUE);
            FreeVec(n);
        }
        FreeVec(msg->windows);
        return 0;
    }

    /* otherwise open all windows of the application */
    {
        struct MinList *children = NULL;
        Object *cstate;
        Object *child;

        get(data->app_WindowFamily, MUIA_Family_List, &children);
        if (!children)
            return 0;

        cstate = (Object *) children->mlh_Head;
        while ((child = NextObject(&cstate)))
        {
            set(child, MUIA_Window_Open, TRUE);
        }
    }
    return 0;
}


/****** Application.mui/MUIM_Application_OpenConfigWindow ************
*
*   NAME
*       MUIM_Application_OpenConfigWindow
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_OpenConfigWindow, flags);
*
*   FUNCTION
*       Launch sys:prefs/Zune asynchronously with the application base and
*       object address on its command line.
*
*   INPUTS
*       flags - ULONG declared in the message; ignored.
*
*   RESULT
*       0 if SystemTagList returns -1; otherwise 1. This does not report whether
*       the preferences program completed successfully.
*
*   NOTES
*       The command is built in a 255-byte buffer. Child standard input and
*       output are disabled; the stack size is AROS_STACKSIZE.
*
*   BUGS
*       The base is inserted without shell quoting and may be truncated along
*       with the command. Accepted Base characters are not a shell-escaping
*       guarantee. The method does not establish that the externally used
*       application object remains alive.
*
*/
static IPTR Application__MUIM_OpenConfigWindow(struct IClass *cl,
    Object *obj, struct MUIP_Application_OpenConfigWindow *msg)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct TagItem tags[] = {
        {SYS_Asynch, TRUE},
        {SYS_Input, 0},
        {SYS_Output, 0},
        {NP_StackSize, AROS_STACKSIZE},
        {TAG_DONE}
    };
    char cmd[255];

    snprintf(cmd, 255, "sys:prefs/Zune %s %ld",
        data->app_Base ? data->app_Base : (STRPTR) "", (long)obj);

    if (SystemTagList(cmd, tags) == -1)
    {
        return 0;
    }

    return 1;
}

/****** Application.mui/MUIM_Application_Execute ************
*
*   NAME
*       MUIM_Application_Execute
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_Execute);
*
*   FUNCTION
*       Run NewInput and Wait repeatedly until the quit ReturnID or
*       SIGBREAKF_CTRL_C is received.
*
*   INPUTS
*       No arguments beyond MethodID. Call in the application's creating task.
*
*   RESULT
*       Always zero on leaving the loop.
*
*   NOTES
*       The loop waits on the NewInput mask plus Ctrl-C when the mask is
*       nonzero. It ignores other ReturnIDs, does not dispose of the
*       application, and has no timeout.
*
*   BUGS
*       It uses an IPTR local for an interface that writes ULONG signals. The
*       mismatch is not portable to all 64-bit byte orders. Queued non-quit IDs
*       and callback changes to masks inherit NewInput's limitations.
*
*/
static IPTR Application__MUIM_Execute(Class *CLASS, Object *self,
    Msg message)
{
    IPTR signals = 0L;

    while
        (DoMethod(self, MUIM_Application_NewInput, (IPTR) & signals)
        != MUIV_Application_ReturnID_Quit)
    {
        if (signals)
        {
            signals = Wait(signals | SIGBREAKF_CTRL_C);
            if (signals & SIGBREAKF_CTRL_C)
                break;
        }
    }

    return 0;
}


/****** Application.mui/MUIM_Application_UpdateMenus ************
*
*   NAME
*       MUIM_Application_UpdateMenus
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_UpdateMenus);
*
*   FUNCTION
*       Send MUIM_Window_UpdateMenu synchronously to every current application
*       window.
*
*   INPUTS
*       No arguments beyond MethodID. Keep membership stable during traversal.
*
*   RESULT
*       Always zero; window results are ignored.
*
*   NOTES
*       Window menu updates can free native Menu and MenuItem structures before
*       rebuilding them. Do not retain their pointers across this method. This
*       is not a success report for menu allocation or attachment.
*
*/
static IPTR Application__MUIM_UpdateMenus(struct IClass *cl, Object *obj,
    Msg message)
{
    struct List *wlist = NULL;
    APTR wstate;
    Object *curwin;

    get(obj, MUIA_Application_WindowList, &wlist);

    if (wlist)
    {
        wstate = wlist->lh_Head;
        while ((curwin = NextObject(&wstate)))
        {
            DoMethod(curwin, MUIM_Window_UpdateMenu);
        }
    }

    return 0;
}

/****** Application.mui/MUIM_Application_Load ************
*
*   NAME
*       MUIM_Application_Load
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_Load, (IPTR)name);
*
*   FUNCTION
*       Read a PREF/MUIO chunk into a temporary Dataspace, then send MUIM_Import
*       to each application window.
*
*   INPUTS
*       name - Filename, or MUIV_Application_Load_ENV (NULL) for
*       ENV:Zune/<base>.cfg, or MUIV_Application_Load_ENVARC for
*       ENVARC:Zune/<base>.cfg. Explicit filenames must be NUL-terminated and
*       shorter than 1023 bytes.
*
*   RESULT
*       Always zero, including allocation, file, IFF, and child-method failures.
*
*   NOTES
*       Import is attempted even when the file could not be opened or parsed,
*       using an empty or partially populated dataspace. There is no rollback of
*       child state.
*       The temporary dataspace is disposed before return; child methods must
*       not retain it or borrowed payloads. I/O can block. These .cfg state
*       files are separate from the .prefs configuration file.
*
*   BUGS
*       Underlying results are ignored. The explicit-name strncpy path does not
*       append a terminator for an overlong name. The return value cannot
*       establish successful loading.
*
*/
static IPTR Application__MUIM_Load(struct IClass *cl, Object *obj,
    struct MUIP_Application_Load *message)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct IFFHandle *iff;
    char name[1024];
    BPTR fh;
    Object *dataspace;
    struct MinList *children = NULL;
    Object *cstate;
    Object *child;

    bug("[MUI:App] %s(0x%p)\n", __func__, obj);

    if (!data->app_Base)
        return 0;

    dataspace = MUI_NewObject(MUIC_Dataspace, TAG_DONE);
    if (!dataspace)
        return 0;

    if (message->name == MUIV_Application_Load_ENV)
        snprintf(name, sizeof(name), "ENV:Zune/%s.cfg", data->app_Base);
    else if (message->name == MUIV_Application_Load_ENVARC)
        snprintf(name, sizeof(name), "ENVARC:Zune/%s.cfg", data->app_Base);
    else
        strncpy(name, message->name, sizeof(name) - 1);

    bug("[MUI:App] %s: opening '%s'\n", __func__, name);

    fh = Open(name, MODE_OLDFILE);
    if (fh)
    {
        if ((iff = AllocIFF()))
        {
            iff->iff_Stream = (IPTR) fh;

            InitIFFasDOS(iff);

            if (!OpenIFF(iff, IFFF_READ))
            {
                if (!StopChunk(iff, ID_PREF, ID_MUIO))
                {
                    if (!ParseIFF(iff, IFFPARSE_SCAN))
                    {
                        DoMethod(dataspace, MUIM_Dataspace_ReadIFF, iff,
                            ID_PREF, ID_MUIO);
                    }
                }

                CloseIFF(iff);
            }
            FreeIFF(iff);
        }
        Close(fh);
    }

    get(data->app_WindowFamily, MUIA_Family_List, &children);
    cstate = (Object *) children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        DoMethod(child, MUIM_Import, dataspace);
    }

    MUI_DisposeObject(dataspace);

    return 0;
}

/****** Application.mui/MUIM_Application_Save ************
*
*   NAME
*       MUIM_Application_Save
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_Save, (IPTR)name);
*
*   FUNCTION
*       Export each window into a temporary Dataspace, then write an IFF PREF
*       file containing PRHD and MUIO chunks.
*
*   INPUTS
*       name - Filename, or MUIV_Application_Save_ENV (NULL) for
*       ENV:Zune/<base>.cfg, or MUIV_Application_Save_ENVARC for
*       ENVARC:Zune/<base>.cfg. Explicit filenames must be NUL-terminated and
*       shorter than 1023 bytes.
*
*   RESULT
*       Always zero, including allocation, file, IFF, and child-method failures.
*
*   NOTES
*       Opening with MODE_NEWFILE can replace existing content. Output is not
*       atomic and failures can leave a partial file. Exported state depends on
*       each child's implementation.
*       The temporary dataspace is disposed before return; child methods must
*       not retain it or borrowed payloads. I/O can block. These .cfg state
*       files are separate from the .prefs configuration file.
*
*   BUGS
*       Underlying results are ignored. The explicit-name strncpy path does not
*       append a terminator for an overlong name. The return value cannot
*       establish successful saving.
*
*/
static IPTR Application__MUIM_Save(struct IClass *cl, Object *obj,
    struct MUIP_Application_Save *message)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct IFFHandle *iff;
    char name[1024];
    BPTR fh;
    Object *dataspace;
    struct MinList *children = NULL;
    Object *cstate;
    Object *child;

    if (!data->app_Base)
        return 0;

    dataspace = MUI_NewObject(MUIC_Dataspace, TAG_DONE);
    if (!dataspace)
        return 0;

    get(data->app_WindowFamily, MUIA_Family_List, &children);
    cstate = (Object *) children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        DoMethod(child, MUIM_Export, dataspace);
    }

    if (message->name == MUIV_Application_Save_ENV)
        snprintf(name, sizeof(name), "ENV:Zune/%s.cfg", data->app_Base);
    else if (message->name == MUIV_Application_Save_ENVARC)
        snprintf(name, sizeof(name), "ENVARC:Zune/%s.cfg", data->app_Base);
    else
        strncpy(name, message->name, sizeof(name) - 1);

    fh = Open(name, MODE_NEWFILE);
    if (fh)
    {
        if ((iff = AllocIFF()))
        {
            iff->iff_Stream = (IPTR) fh;

            InitIFFasDOS(iff);

            if (!OpenIFF(iff, IFFF_WRITE))
            {
                if (!PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN))
                {
                    if (!PushChunk(iff, ID_PREF, ID_PRHD,
                            sizeof(struct FilePrefHeader)))
                    {
                        struct FilePrefHeader head;

                        head.ph_Version = PHV_CURRENT;
                        head.ph_Type = 0;
                        head.ph_Flags[0] =
                            head.ph_Flags[1] =
                            head.ph_Flags[2] = head.ph_Flags[3] = 0;

                        if (WriteChunkBytes(iff, &head,
                                sizeof(head)) == sizeof(head))
                        {
                            PopChunk(iff);
                            DoMethod(dataspace, MUIM_Dataspace_WriteIFF,
                                iff, ID_PREF, ID_MUIO);
                        }
                        else
                        {
                            PopChunk(iff);
                        }
                    }
                    PopChunk(iff);
                }
                CloseIFF(iff);
            }
            FreeIFF(iff);
        }
        Close(fh);
    }

    MUI_DisposeObject(dataspace);

    return 0;
}


/****** Application.mui/MUIM_Application_CheckRefresh ************
*
*   NAME
*       MUIM_Application_CheckRefresh
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_CheckRefresh);
*
*   FUNCTION
*       Send MUIM_Window_Refresh to every window in the internal family.
*
*   INPUTS
*       No arguments beyond MethodID. Keep the application and window list alive
*       throughout the call.
*
*   RESULT
*       Always zero; child results are ignored.
*
*   NOTES
*       This is the refresh entry point used from modal-requester IntuiMessage
*       hooks. It does not itself dequeue or reply to the hook message. Supply
*       any native-window requester tags only while that window is open; they
*       are not necessarily valid during construction.
*
*/
static IPTR Application__MUIM_CheckRefresh(struct IClass *cl, Object *obj,
    struct MUIP_Application_CheckRefresh *message)
{
    struct MUI_ApplicationData *data = INST_DATA(cl, obj);
    struct MinList *children = NULL;
    Object *cstate;
    Object *child;

    get(data->app_WindowFamily, MUIA_Family_List, &children);
    cstate = (Object *) children->mlh_Head;
    while ((child = NextObject(&cstate)))
        DoMethod(child, MUIM_Window_Refresh);

    return 0;
}

/*
 * The class dispatcher
 */
/****** Application.mui/MUIM_Application_GetMenuCheck ************
*
*   NAME
*       MUIM_Application_GetMenuCheck
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_GetMenuCheck, MenuID);
*
*   FUNCTION
*       Declared in application.h, but not handled by the Application
*       dispatcher. The message is forwarded through Notify to rootclass without
*       implementing this operation.
*
*   INPUTS
*       MenuID - ULONG menu identifier.
*
*   RESULT
*       The inherited unhandled-method result is zero. It is not a completion or
*       menu-state result.
*
*   NOTES
*       Use the implemented attributes and object interfaces only where their
*       documented behavior meets the need. A method name alone does not
*       establish compatibility.
*
*/

/****** Application.mui/MUIM_Application_GetMenuState ************
*
*   NAME
*       MUIM_Application_GetMenuState
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_GetMenuState, MenuID);
*
*   FUNCTION
*       Declared in application.h, but not handled by the Application
*       dispatcher. The message is forwarded through Notify to rootclass without
*       implementing this operation.
*
*   INPUTS
*       MenuID - ULONG menu identifier.
*
*   RESULT
*       The inherited unhandled-method result is zero. It is not a completion or
*       menu-state result.
*
*   NOTES
*       Use the implemented attributes and object interfaces only where their
*       documented behavior meets the need. A method name alone does not
*       establish compatibility.
*
*/

/****** Application.mui/MUIM_Application_SetMenuCheck ************
*
*   NAME
*       MUIM_Application_SetMenuCheck
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_SetMenuCheck, MenuID,
*       stat);
*
*   FUNCTION
*       Declared in application.h, but not handled by the Application
*       dispatcher. The message is forwarded through Notify to rootclass without
*       implementing this operation.
*
*   INPUTS
*       MenuID - ULONG menu identifier.
*       stat - LONG state.
*
*   RESULT
*       The inherited unhandled-method result is zero. It is not a completion or
*       menu-state result.
*
*   NOTES
*       Use the implemented attributes and object interfaces only where their
*       documented behavior meets the need. A method name alone does not
*       establish compatibility.
*
*/

/****** Application.mui/MUIM_Application_SetMenuState ************
*
*   NAME
*       MUIM_Application_SetMenuState
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_SetMenuState, MenuID,
*       stat);
*
*   FUNCTION
*       Declared in application.h, but not handled by the Application
*       dispatcher. The message is forwarded through Notify to rootclass without
*       implementing this operation.
*
*   INPUTS
*       MenuID - ULONG menu identifier.
*       stat - LONG state.
*
*   RESULT
*       The inherited unhandled-method result is zero. It is not a completion or
*       menu-state result.
*
*   NOTES
*       Use the implemented attributes and object interfaces only where their
*       documented behavior meets the need. A method name alone does not
*       establish compatibility.
*
*/

/****** Application.mui/MUIM_Application_ShowHelp ************
*
*   NAME
*       MUIM_Application_ShowHelp
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Application_ShowHelp, (IPTR)window,
*       (IPTR)name, (IPTR)node, line);
*
*   FUNCTION
*       Declared in application.h, but not handled by the Application
*       dispatcher. The message is forwarded through Notify to rootclass without
*       implementing this operation.
*
*   INPUTS
*       window - Object pointer.
*       name, node - String pointers.
*       line - LONG. These fields are declared but unused by Application.
*
*   RESULT
*       The inherited unhandled-method result is zero. It is not a completion or
*       menu-state result.
*
*   NOTES
*       Use the implemented attributes and object interfaces only where their
*       documented behavior meets the need. A method name alone does not
*       establish compatibility.
*
*/

/****** Application.mui/MUIM_Application_Iconify ************
*
*   NAME
*       MUIM_Application_Iconify
*
*   SYNOPSIS
*       DoMethodA(obj, message);
*
*   FUNCTION
*       Declared in application.h, but not handled by the Application
*       dispatcher. The message is forwarded through Notify to rootclass without
*       implementing this operation.
*
*   INPUTS
*       The header defines the MethodID but no corresponding method structure or
*       implemented argument contract.
*
*   RESULT
*       The inherited unhandled-method result is zero. It is not a completion or
*       menu-state result.
*
*   NOTES
*       Use the implemented attributes and object interfaces only where their
*       documented behavior meets the need. A method name alone does not
*       establish compatibility.
*
*/

/* Dispatch supported Application operations and inherited user-data/member
 * overrides, returning handler results. Forward unknown methods to Notify.
 * Header-only method IDs are not silently implemented here; messages must
 * contain the fields required by the selected handler.
 */
BOOPSI_DISPATCHER(IPTR, Application_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Application__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Application__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Application__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Application__OM_GET(cl, obj, (struct opGet *)msg);
    case OM_ADDMEMBER:
        return Application__OM_ADDMEMBER(cl, obj, (APTR) msg);
    case OM_REMMEMBER:
        return Application__OM_REMMEMBER(cl, obj, (APTR) msg);
    case MUIM_Application_AddInputHandler:
        return Application__MUIM_AddInputHandler(cl, obj, (APTR) msg);
    case MUIM_Application_RemInputHandler:
        return Application__MUIM_RemInputHandler(cl, obj, (APTR) msg);
    case MUIM_Application_Input:
        return Application__MUIM_Input(cl, obj, (APTR) msg);
    case MUIM_Application_InputBuffered:
        return Application__MUIM_InputBuffered(cl, obj, (APTR) msg);
    case MUIM_Application_NewInput:
        return Application__MUIM_NewInput(cl, obj, (APTR) msg);
    case MUIM_Application_PushMethod:
        return Application__MUIM_PushMethod(cl, obj, (APTR) msg);
    case MUIM_Application_UnpushMethod:
        return Application__MUIM_UnpushMethod(cl, obj, (APTR) msg);
    case MUIM_Application_ReturnID:
        return Application__MUIM_ReturnID(cl, obj, (APTR) msg);
    case MUIM_FindUData:
        return Application__MUIM_FindUData(cl, obj, (APTR) msg);
    case MUIM_GetUData:
        return Application__MUIM_GetUData(cl, obj, (APTR) msg);
    case MUIM_SetUData:
        return Application__MUIM_SetUData(cl, obj, (APTR) msg);
    case MUIM_SetUDataOnce:
        return Application__MUIM_SetUDataOnce(cl, obj, (APTR) msg);
    case MUIM_Application_AboutMUI:
        return Application__MUIM_AboutMUI(cl, obj, (APTR) msg);
    case MUIM_Application_SetConfigdata:
        return Application__MUIM_SetConfigdata(cl, obj, (APTR) msg);
    case MUIM_Application_SetConfigItem:
        return Application__MUIM_SetConfigItem(cl, obj, (APTR) msg);
    case MUIM_Application_OpenWindows:
        return Application__MUIM_OpenWindows(cl, obj, (APTR) msg);
    case MUIM_Application_OpenConfigWindow:
        return Application__MUIM_OpenConfigWindow(cl, obj, (APTR) msg);
    case MUIM_Application_Execute:
        return Application__MUIM_Execute(cl, obj, msg);
    case MUIM_Application_UpdateMenus:
        return Application__MUIM_UpdateMenus(cl, obj, msg);
    case MUIM_Application_Load:
        return Application__MUIM_Load(cl, obj, (APTR) msg);
    case MUIM_Application_Save:
        return Application__MUIM_Save(cl, obj, (APTR) msg);
    case MUIM_Application_CheckRefresh:
        return Application__MUIM_CheckRefresh(cl, obj, (APTR) msg);
    }

    return (DoSuperMethodA(cl, obj, msg));
}
BOOPSI_DISPATCHER_END
/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Application_desc =
{
    MUIC_Application,
    MUIC_Notify,
    sizeof(struct MUI_ApplicationData),
    (void *) Application_Dispatcher
};
