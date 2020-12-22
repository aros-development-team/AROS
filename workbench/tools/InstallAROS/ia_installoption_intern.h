#ifndef INSTALLOPTION_INTERN_H
#define INSTALLOPTION_INTERN_H

struct InstallOption_Data
{
    Object      *io_Object;
    char        *io_ID;
};

BOOPSI_DISPATCHER_PROTO(IPTR, InstallOption__Dispatcher, CLASS, self, message);

#endif /* INSTALLOPTION_INTERN_H */