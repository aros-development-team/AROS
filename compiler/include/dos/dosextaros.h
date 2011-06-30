#ifndef DOS_DOSEXTAROS_H
#define DOS_DOSEXTAROS_H

#define AROS_DOSDEVNAME(dn) AROS_BSTR_ADDR(((struct DosList*)(dn))->dol_Name)

#endif /* DOS_DOSEXTAROS_H */
