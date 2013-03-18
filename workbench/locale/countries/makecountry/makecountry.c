/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Program that makes country files
    Lang: english
*/

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>

#define D(x)

struct CountryEntry
{
    char *ca_Name;
    struct CountryPrefs *ca_Data;
};

extern struct CountryPrefs
    andorraPrefs,
    angolaPrefs,
    argentinaPrefs,
    australiaPrefs,
    belgiePrefs,
    belgiquePrefs,
    bosna_i_hercegovinaPrefs,
    bulgarijaPrefs,
    brasilPrefs,
    cabo_verdePrefs,
    canadaPrefs,
    canada_francaisPrefs,
    costa_ricaPrefs,
    czech_republicPrefs,
    danmarkPrefs,
    deutschPrefs,
    eestiPrefs,
    eirePrefs,
    espanaPrefs,
    francePrefs,
    greatBritainPrefs,
    haitiPrefs,
    hangukPrefs,
    hellasPrefs,
    hrvatskaPrefs,
    indonesiaPrefs,
    iranPrefs,
    irelandPrefs,
    islandPrefs,
    italiaPrefs,
    jugoslavijaPrefs,
    letzebuergPrefs,
    liechtensteinPrefs,
    lietuvaPrefs,
    magyarorszagPrefs,
    maltaPrefs,
    moldovaPrefs,
    monacoPrefs,
    new_zealandPrefs,
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
    slovenskoPrefs,
    slovenijaPrefs,
    suissePrefs,
    suomiPrefs,
    sverigePrefs,
    svizzeraPrefs,
    timor_lestePrefs,
    turkiyePrefs,
    ukrajinaPrefs,
    united_kingdomPrefs,
    united_statesPrefs,
    vaticanoPrefs,
    zhonghuaPrefs;

/* Please keep this in alphabetical order, ie the order of Latin 1.
   Character set of this file is also Latin 1 */

struct CountryEntry CountryArray[] =
{
    { "andorra"     	 , &andorraPrefs     	},
    { "angola"     	 , &angolaPrefs     	},
    { "argentina"     	 , &argentinaPrefs     	},
    { "australia"   	 , &australiaPrefs 	},
    { "belgië"	    	 , &belgiePrefs    	},
    { "belgique"    	 , &belgiquePrefs  	},
    { "bosna_i_hercegovina", &bosna_i_hercegovinaPrefs},
    { "brasil"	         , &brasilPrefs         },
    { "bulgarija"	 , &bulgarijaPrefs	},
    { "cabo_verde"	 , &cabo_verdePrefs  	},
    { "canada"	    	 , &canadaPrefs    	},
    { "canada_français"	 , &canada_francaisPrefs},
    { "costa_rica"	 , &costa_ricaPrefs	},
    { "czech_republic" 	 , &czech_republicPrefs },
    { "danmark"     	 , &danmarkPrefs   	},
    { "deutschland" 	 , &deutschPrefs   	},
    { "eesti"		 , &eestiPrefs		},
    { "éire"		 , &eirePrefs		},
    { "españa"	    	 , &espanaPrefs    	},
    { "france"	    	 , &francePrefs    	},
    { "great_britain"	 , &greatBritainPrefs   },
    { "haïti"		 , &haitiPrefs		},
    { "hanguk"	         , &hangukPrefs         },
    { "hellas"	         , &hellasPrefs         },
    { "hrvatska"    	 , &hrvatskaPrefs    	},
    { "indonesia"	 , &indonesiaPrefs	},
    { "iran"		 , &iranPrefs		},
    { "ireland"		 , &irelandPrefs	},
    { "ísland"		 , &islandPrefs		},
    { "italia"	    	 , &italiaPrefs    	},
    { "jugoslavija" 	 , &jugoslavijaPrefs 	},
    { "liechtenstein"	 , &liechtensteinPrefs	},
    { "lëtzebuerg"	 , &letzebuergPrefs	},
    { "lietuva"		 , &lietuvaPrefs	},
    { "magyarország" 	 , &magyarorszagPrefs	},
    { "malta"		 , &maltaPrefs		},
    { "moldova"		 , &moldovaPrefs	},
    { "monaco"	    	 , &monacoPrefs	    	},
    { "new_zealand"    	 , &new_zealandPrefs	},
    { "nihon"	    	 , &nihonPrefs	    	},
    { "nederland"   	 , &nederlandPrefs 	},
    { "norge"	    	 , &norgePrefs     	},
    { "österreich"    	 , &osterreichPrefs 	},
    { "polska"	    	 , &polskaPrefs    	},
    { "portugal"    	 , &portugalPrefs  	},
    { "românia"		 , &romaniaPrefs	},
    { "rossija"		 , &rossijaPrefs	},
    { "san_marino"  	 , &san_marinoPrefs  	},
    { "schweiz"     	 , &schweizPrefs   	},
    { "slovensko"	 , &slovenskoPrefs	},
    { "slovenija"   	 , &slovenijaPrefs   	},
    { "suisse"	    	 , &suissePrefs    	},
    { "suomi"	    	 , &suomiPrefs     	},
    { "sverige"     	 , &sverigePrefs   	},
    { "svizzera"    	 , &svizzeraPrefs    	},
    { "timor-leste"    	 , &timor_lestePrefs    },
    { "türkiye"     	 , &turkiyePrefs     	},
    { "ukrajina"	 , &ukrajinaPrefs	},
    { "united_kingdom"	 , &united_kingdomPrefs	},
    { "united_states"	 , &united_statesPrefs	},
    { "vaticano"	 , &vaticanoPrefs	},
    { "zhonghua"	 , &zhonghuaPrefs	},
    { NULL		 , NULL			}
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

void convertEndianness(struct CountryPrefs *cp);
unsigned long getCountryPrefsSize(void);

int doCountry(struct CountryPrefs *cp, char *progname, char *filename)
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

    convertEndianness(cp);

    if(fwrite(cp, getCountryPrefsSize(), 1, fp) < 1)
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
    int i,j,res = 0;
    int do_all;
    char buffer[64];
    char *name;
    iconv_t cd = (iconv_t)-1;

    if(argc < 3)
    {
	printf("%s: Wrong number of arguments\n", argv[0]);
	return(20);
    }

    do_all = strcmp("--all", argv[2]);

    /* Check host OS locale. If it's UTF-8, use UTF-8 file names. Otherwise use Latin-1 names. */
    name = setlocale(LC_CTYPE, "");
    D(printf("System locale: %s\n", name));

    /* Use strstr() because for example on Linux this will be "en_US.UTF-8" while on MacOS it's just "UTF-8" */
    if ((strstr(name, "UTF-8")) || (strstr(name, "utf8")))
    {
	cd = iconv_open("UTF-8", "ISO-8859-1");
	if (cd == (iconv_t)(-1))
	{
	    printf("%s: Error converting character sets\n", argv[0]);
	    return(20);
	}
    }

    for(j=0; CountryArray[j].ca_Name != NULL; j++)
    {
	name = CountryArray[j].ca_Name;

    	if (cd != (iconv_t)-1)
    	{
    	    char *out = buffer;
    	    size_t inbytes, outbytes;

	    /* Convert country name to local character set */
	    inbytes = strlen(name) + 1;
	    outbytes = sizeof(buffer);
	    iconv(cd, &name, &inbytes, &out, &outbytes);
	    name = buffer;
	}

	res = do_all;
	if (res)
	{
	    for(i=2; i < argc; i++)
    	    {
		res = strcmp(name, argv[i]);
		if (!res)
		    break;
	    }
	}

	if (res == 0)
	{
	    char path[1024];

	    printf("Generating %s.country\n", name);

	    strcpy(path, argv[1]);
	    strcat(path, name);
	    strcat(path, ".country");
	    doCountry(CountryArray[j].ca_Data, argv[0], path);
	}
    }

    return 0;
}
