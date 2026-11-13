#ifndef _IA_INSTALL_INTERN_H
#define _IA_INSTALL_INTERN_H

struct Install_DATA
{
    struct MUI_CustomClass *id_OptionClass;
    struct MUI_CustomClass *id_StageClass;
    struct List             id_Options;
    struct List             id_Stages;
};

BOOPSI_DISPATCHER_PROTO(IPTR, Install__Dispatcher, CLASS, self, message);

#endif /* _IA_INSTALL_INTERN_H */
