#ifndef DATATYPES_SOUNDCLASSEXT_H
#define DATATYPES_SOUNDCLASSEXT_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Extended Includes for soundclass (V41 sounddt by Stephan Rupprecht)
    Lang: English
*/

#ifndef DATATYPES_SOUNDCLASS_H
#   include <datatypes/soundclass.h>
#endif

#define SDTA_SampleType	    (SDTA_Dummy + 30)
#define SDTA_Panning	    (SDTA_Dummy + 31)
#define SDTA_Frequency	    (SDTA_Dummy + 32)

#define SDTST_M8S   	    0
#define SDTST_S8S   	    1
#define SDTST_M16S  	    2
#define SDTST_S16S  	    3

#define SDTM_ISSTEREO(sampletype)   ((sampletype) & 1)
#define SDTM_CHANNELS(sampletype)   (1 + SDTM_ISSTEREO(sampletype))
#define SDTM_BYTESPERSAMPLE(x)	    (((x) >= SDTST_M16S ) ? 2 : 1)
#define SDTM_BYTESPERPOINT(x)	    (SDTM_CHANNELS(x) * SDTM_BYTESPERSAMPLE(x))

#endif  /* DATATYPES_SOUNDCLASSEXT_H */
