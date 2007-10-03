
#ifndef DESKTOPCONTAINER_OBSERVER_H
#    define DESKTOPCONTAINER_OBSERVER_H

#    define ICOA_Directory            TAG_USER+1
#    define ICOA_InTree               TAG_USER+2

#    define ICOM_AddIcons             TAG_USER+12

#    define DO_Dummy                    TAG_USER+2700
#    define DOA_DefaultWindowClass      DO_Dummy+1
#    define DOA_DefaultWindowArguments  DO_Dummy+2

struct DesktopObserverClassData
{
    Class          *defaultWindow;
    struct TagItem *defaultWindowArgs;
};

struct doDeleteWindow
{
    STACKED Msg             MethodID;
    STACKED Object         *icObs;
};

// struct icoAddIcon
// {
// Msg methodID;
// ULONG wsr_Results;
// struct SingleResult *wsr_ResultsArray;
// };

#endif
