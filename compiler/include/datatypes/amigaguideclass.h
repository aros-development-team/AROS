#ifndef DATATYPES_AMIGAGUIDECLASS_H
#define DATATYPES_AMIGAGUIDECLASS_H

/*
** $VER: amigaguideclass.h 1.1 (04.09.03)
*/

#ifndef UTILITY_TAGITEM_H
# include <utility/tagitem.h>
#endif

#ifndef DATATYPES_DATATYPESCLASS_H
# include <datatypes/datatypesclass.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
# include <libraries/iffparse.h>
#endif

#if defined(__MORPHOS__) && defined(__GNUC__)
# pragma pack(2)
#endif


#define AMIGAGUIDEDTCASS  "amigaguide.datatype"

#define AGDTA_Dummy      (DTA_Dummy + 700)
#define AGDTA_Secure     (AGDTA_Dummy + 1)
#define AGDTA_HelpGroup  (AGDTA_Dummy + 2)


#if defined(__MORPHOS__) && defined(__GNUC__)
# pragma pack()
#endif

#endif /* DATATYPES_AMIGAGUIDECLASS_H */


