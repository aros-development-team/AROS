#ifndef DATATYPES_AMIGAGUIDECLASS_H
#define DATATYPES_AMIGAGUIDECLASS_H

/*
** $VER: amigaguideclass.h 1.2 (22.12.2011)
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


#define AMIGAGUIDEDTCLASS  "amigaguide.datatype"

#define AGDTA_Dummy      (DTA_Dummy + 700)
#define AGDTA_Secure     (AGDTA_Dummy + 1)
#define AGDTA_HelpGroup  (AGDTA_Dummy + 2)


#endif /* DATATYPES_AMIGAGUIDECLASS_H */


