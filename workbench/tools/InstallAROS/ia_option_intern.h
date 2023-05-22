#ifndef _IA_OPTION_INTERN_H
#define _IA_OPTION_INTERN_H

struct InstallOption_Data
{
    struct Node iod_Node;
    Object      *iod_Object;
    char        *iod_ID;
    IPTR        iod_OptionTag;
    IPTR        iod_OptionVal;
};

BOOPSI_DISPATCHER_PROTO(IPTR, InstallOption__Dispatcher, CLASS, self, message);

#endif /* _IA_OPTION_INTERN_H */
