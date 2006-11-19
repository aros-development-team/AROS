/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Program that makes country files
    Lang: english
*/

#include <exec/types.h>
#include <libraries/locale.h>
#include <prefs/locale.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aros/system.h>
#include <iconv.h>

#define EC(x)\
{\
    (x) =   (((x) & 0xFF000000) >> 24)\
	  | (((x) & 0x00FF0000) >> 8)\
	  | (((x) & 0x0000FF00) << 8)\
	  | (((x) & 0x000000FF) << 24);\
}

struct CountryEntry
{
    STRPTR ca_Name;
    struct CountryPrefs *ca_Data;
};

extern struct CountryPrefs
    andorraPrefs,
    australiaPrefs,
    belgiePrefs,
    belgiquePrefs,
    bosna_i_hercegovinaPrefs,
    bulgarijaPrefs,
    brasilPrefs,
    canadaPrefs,
    canada_francaisPrefs,
    ceskoPrefs,
    danmarkPrefs,
    deutschPrefs,
    eirePrefs,
    espanaPrefs,
    francePrefs,
    greatBritainPrefs,
    hrvatskaPrefs,
    iranPrefs,
    irelandPrefs,
    islandPrefs,
    italiaPrefs,
    jugoslavijaPrefs,
    letzebuergPrefs,
    liechtensteinPrefs,
    lietuvaPrefs,
    magyarorszagPrefs,
    monacoPrefs,
    nihonPrefs,
    nederlandPrefs,
    norgePrefs,
    osterreichPrefs,
    polskaPrefs,
    portugalPrefs,
    romaniaPrefs,
    rossijaPrefs,
    san_marinoPrefs,
    schweizPrefs,
    slovakiaPrefs,
    slovenijaPrefs,
    suissePrefs,
    suomiPrefs,
    sverigePrefs,
    svizzeraPrefs,
    turkiyePrefs,
    ukrajinaPrefs,
    united_kingdomPrefs,
    united_statesPrefs,
    vaticanoPrefs;

/* Please keep this in alphabetical order, ie the order of Latin 1 */

struct CountryEntry CountryArray[] =
{
    { "andorra"     	, &andorraPrefs     	},
    { "australia"   	, &australiaPrefs 	},
    { "belgië"	    	, &belgiePrefs    	},
    { "belgique"    	, &belgiquePrefs  	},
    { "bosna_i_hercegovina", &bosna_i_hercegovinaPrefs},
    { "brasil"          , &brasilPrefs          },
    { "bulgarija"	, &bulgarijaPrefs	},
    { "canada"	    	, &canadaPrefs    	},
    { "canada_français" , &canada_francaisPrefs },
    { "cesko"	    	, &ceskoPrefs	    	},
    { "danmark"     	, &danmarkPrefs   	},
    { "deutschland" 	, &deutschPrefs   	},
    { "éire"		, &eirePrefs		},
    { "españa"	    	, &espanaPrefs    	},
    { "france"	    	, &francePrefs    	},
    { "great_britain"	, &greatBritainPrefs    },
    { "hrvatska"    	, &hrvatskaPrefs    	},
    { "iran"		, &iranPrefs		},
    { "ireland"		, &irelandPrefs		},
    { "ísland"		, &islandPrefs		},
    { "italia"	    	, &italiaPrefs    	},
    { "jugoslavija" 	, &jugoslavijaPrefs 	},
    { "liechtenstein"	, &liechtensteinPrefs	},
    { "lëtzebuerg"	, &letzebuergPrefs	},
    { "lietuva"		, &lietuvaPrefs		},
    { "magyarország" 	, &magyarorszagPrefs	},
    { "monaco"	    	, &monacoPrefs	    	},
    { "nihon"	    	, &nihonPrefs	    	},
    { "nederland"   	, &nederlandPrefs 	},
    { "norge"	    	, &norgePrefs     	},
    { "österreich"    	, &osterreichPrefs 	},
    { "polska"	    	, &polskaPrefs    	},
    { "portugal"    	, &portugalPrefs  	},
    { "românia"		, &romaniaPrefs		},
    { "rossija"		, &rossijaPrefs		},
    { "san_marino"  	, &san_marinoPrefs  	},
    { "schweiz"     	, &schweizPrefs   	},
    { "slovakia"    	, &slovakiaPrefs    	},
    { "slovenija"   	, &slovenijaPrefs   	},
    { "suisse"	    	, &suissePrefs    	},
    { "suomi"	    	, &suomiPrefs     	},
    { "sverige"     	, &sverigePrefs   	},
    { "svizzera"    	, &svizzeraPrefs    	},
    { "türkiye"     	, &turkiyePrefs     	},
    { "ukrajina"	, &ukrajinaPrefs	},
    { "united_kingdom"	, &united_kingdomPrefs	},
    { "united_states"	, &united_statesPrefs	},
    { "vaticano"	, &vaticanoPrefs	},
    { NULL  	    	, NULL    	    	}
};

/* This is equivalent to the start of the catalog file.
   It is a series of strings, so that the endianness is
   correct either way
*/
char preamble[] =
{
    "FORM" "\x00\x00\x02\x12" "PREF"

	"PRHD" "\x00\x00\x00\x06"
	    "\x00\x00\x00\x00\x00\x00"

	"CTRY" "\x00\x00\x01\xF8"
};


int doCountry(struct CountryPrefs *cp, STRPTR progname, STRPTR filename)
{
    FILE *fp;

    fp = fopen(filename, "w");
    if(fp == NULL)
    {
	printf("%s: Could not open file %s\n", progname, filename);
	return (20);
    }

    /* Write the preamble...
	FORM 0x00000212 PREF
	    PRHD 0x00000006

	    CTRY 0x000001F8
    */
    if(fwrite(preamble, 34, 1, fp) < 1)
    {
	printf("%s: Write error during preable of %s.\n", progname, filename);
	fclose(fp);
	return(20);
    }

#if (AROS_BIG_ENDIAN == 0)
    /* We have to convert the endianness of this data,
       thankfully there are only two fields which this applies
       to.
    */
    EC(cp->cp_CountryCode);
    EC(cp->cp_TelephoneCode);
#endif

    if(fwrite(cp, sizeof(struct CountryPrefs), 1, fp) < 1)
    {
	printf("%s: Write error during data for %s.\n", progname, filename);
	fclose(fp);
	return(20);
    }

    fclose(fp);
    return 0;
}

int main(int argc, char **argv)
{
    int i,j,res;
    char buffer[1024];
    const char *inpos;
    char *outpos;
    size_t inbytes, outbytes;
    iconv_t cd;

    if(argc < 3)
    {
	printf("%s: Wrong number of arguments\n", argv[0]);
	return(20);
    }

    cd = iconv_open("", "ISO-8859-1");
    if(cd == (iconv_t)(-1))
    {
	printf("%s: Error converting character sets\n", argv[0]);
	return(20);
    }

    for(i=2; i < argc; i++)
    {
	for(j=0; CountryArray[j].ca_Name != NULL; j++)
	{
	    /* Convert country name to local character set */
	    inpos = CountryArray[j].ca_Name;
	    inbytes = 1024;
	    outpos = buffer;
	    outbytes = 1024;
	    iconv(cd, &inpos, &inbytes, &outpos, &outbytes);

	    res = strcmp(buffer, argv[i]);
	    if(res == 0)
	    {
		strcpy(buffer, argv[1]);
		strcat(buffer, argv[i]);
		strcat(buffer, ".country");
		doCountry(CountryArray[j].ca_Data, argv[0], buffer);
		break;
	    }
#if 0
/* stegerg: does not work because of 'ö' in österreich */
	    /* If countryArray < argv[] don't bother searching */
	    else if(res > 0)
		break;
#endif

	}
	if(res != 0)
	{
	    printf("Unknown country %s\n", argv[i]);
	    //return(20);
	}
    }
    return 0;
}
