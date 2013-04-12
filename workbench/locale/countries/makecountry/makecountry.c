/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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
    struct IntCountryPrefs *ca_Data;
};

extern struct IntCountryPrefs
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
char iffheader[] =
{
    "FORM" "\x00\x00\x00\x00" "PREF"

	"PRHD" "\x00\x00\x00\x06"
	    "\x00\x00\x00\x00\x00\x00"
};

char ctryheader[] =
{
    "CTRY" "\x00\x00\x00\x00"
};

char versheader[] =
{
    "FVER" "\x00\x00\x00\x00"
};

char flagheader[] =
{
    "FLAG" "\x00\x00\x00\x00"
};


char iffpad[] = "\x00";

void convertEndianness(struct CountryPrefs *cp);
unsigned long getCountryPrefsSize(void);
unsigned long getCountryPrefsVers(struct IntCountryPrefs *cp);
unsigned long getCountryPrefsFlag(struct IntCountryPrefs *cp);

int writeChunk(FILE *fp, char *header, void *buffer, int len,
                char *progname, char *filename)
{
    header[4] = (len & (0xFF000000)) >> 24;
    header[5] = (len & (0x00FF0000)) >> 16;
    header[6] = (len & (0x0000FF00)) >> 8;
    header[7] = (len & (0x000000FF));

    /* Write out the IFF header... */
    if(fwrite(header, 8, 1, fp) < 1)
    {
	printf("%s: Error writing chunk header for %s.\n", progname, filename);
	fclose(fp);
	return(20);
    }

    if(fwrite(buffer, len, 1, fp) < 1)
    {
	printf("%s: Error writing chunk data for %s.\n", progname, filename);
	fclose(fp);
	return(20);
    }
    if ((len & 1) && (fwrite(iffpad, 1, 1, fp) < 1))
    {
        printf("%s: Error padding chunk for %s.\n", progname, filename);
        fclose(fp);
        return(20);
    }
    return 0;
}

int doCountry(struct IntCountryPrefs *cp, char *progname, char *filename)
{
    FILE *fp;
    int size = 530;
    char *cpVers, *cpFlag;

    fp = fopen(filename, "w");
    if(fp == NULL)
    {
	printf("%s: Could not open file %s\n", progname, filename);
	return (20);
    }

    /* Adjust the size of the IFF file if necessary ... */
    if ((cpVers = getCountryPrefsVers(cp)) != NULL)
    {
        size += strlen(cpVers);
        size += 8;
    }
    if ((cpFlag = getCountryPrefsFlag(cp)) != NULL)
    {
        size += strlen(cpFlag);
        size += 8;
    }

    iffheader[4] = (size & (0xFF000000)) >> 24;
    iffheader[5] = (size & (0x00FF0000)) >> 16;
    iffheader[6] = (size & (0x0000FF00)) >> 8;
    iffheader[7] = (size & (0x000000FF));

    /* Write out the IFF header... */
    if(fwrite(iffheader, 26, 1, fp) < 1)
    {
	printf("%s: Error writing IFF header for %s.\n", progname, filename);
	fclose(fp);
	return(20);
    }

    /* Write out the main Country Prefs Chunk ... */
    convertEndianness((struct CountryPrefs *)cp);

    if(writeChunk(fp, ctryheader, cp, getCountryPrefsSize(), progname, filename))
    {
	printf("%s: Error writing country data chunk %s.\n", progname, filename);
	return(20);
    }

    /* Write out the Version String Chunk if appropriate ... */
    if (cpVers)
    {
        if(writeChunk(fp, versheader, cpVers, strlen(cpVers) + 1, progname, filename))
        {
            printf("%s: Error writing country version string chunk %s.\n", progname, filename);
            return(20);
        }
    }

    /* Write out the Flag Chunk if appropriate ... */
    if (cpFlag)
    {
        if(writeChunk(fp, flagheader, cpFlag, strlen(cpFlag) + 1, progname, filename))
        {
            printf("%s: Error writing country flag string chunk %s.\n", progname, filename);
            return(20);
        }
    }

    fclose(fp);
    return 0;
}

int main(int argc, char **argv)
{
    int i, j, skipCountry = 0;
    int do_specific;
    char buffer[64];
    char *name;
    iconv_t cd = (iconv_t)-1;

    if(argc < 3)
    {
	printf("%s: Wrong number of arguments\n", argv[0]);
	return(20);
    }

    do_specific = strcmp("--all", argv[2]);

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

	if (do_specific)
	{
            /* locate the specified country */
	    for(i=2; i < argc; i++)
    	    {
		skipCountry = strcmp(name, argv[i]);
		if (!skipCountry)
		    break;
	    }
	}

	if (!skipCountry)
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
