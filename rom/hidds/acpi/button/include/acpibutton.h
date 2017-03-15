#ifndef HIDD_ACPIBUTTON_H
#define HIDD_ACPIBUTTON_H

#define CLID_Hidd_ACPIButton "hidd.acpibutton"
#define IID_Hidd_ACPIButton CLID_Hidd_ACPIButton

#define HiddACPIButtonAB __abHidd_ACPIButton

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddACPIButtonAB;
#endif

/* Attrs */

enum {

   aoHidd_ACPIButton_Type,
   aoHidd_ACPIButton_Handle,
   aoHidd_ACPIButton_Hook,

   num_Hidd_ACPIButton_Attrs
};

#define aHidd_ACPIButton_Type          (aoHidd_ACPIButton_Type     + HiddACPIButtonAB)
#define aHidd_ACPIButton_Handle       (aoHidd_ACPIButton_Handle     + HiddACPIButtonAB)
#define aHidd_ACPIButton_Hook          (aoHidd_ACPIButton_Hook     + HiddACPIButtonAB)

#define IS_HIDDACPIBUTTON_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HiddACPIButtonAB, num_Hidd_ACPIButton_Attrs)

/* Button types */
enum {
   vHidd_ACPIButton_Power,
   vHidd_ACPIButton_PowerF,
   vHidd_ACPIButton_Sleep,
   vHidd_ACPIButton_SleepF,
   vHidd_ACPIButton_Lid
};


#endif /* !HIDD_ACPIBUTTON_H */
