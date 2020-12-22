#ifndef INSTALLOPTION_INTERN_H
#define INSTALLOPTION_INTERN_H

struct InstallOption_Data
{
    Object      *iod_Object;
    char        *iod_ID;
    IPTR        *iod_OptionTag;
    IPTR        iod_OptionVal;
};

BOOPSI_DISPATCHER_PROTO(IPTR, InstallOption__Dispatcher, CLASS, self, message);

#endif /* INSTALLOPTION_INTERN_H */