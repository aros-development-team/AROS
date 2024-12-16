#ifndef IA_VOLUME_INTERN_H
#define IA_VOLUME_INTERN_H

struct Volume_Data
{
    struct Hook                 vd_ActivateHook;
    struct MUI_EventHandlerNode vd_EHNode;
    Object                      *vd_PopObj;
    char                        *vd_PopPadTxt;
    Object                      *vd_PopParentObj;

    Object                      *vd_CreateObj;
    Object                      *vd_NameObj;
    Object                      *vd_FSObj;
    Object                      *vd_SizeObj;
};

BOOPSI_DISPATCHER_PROTO(IPTR, Volume__Dispatcher, CLASS, self, message);

#endif /* IA_VOLUME_INTERN_H */
