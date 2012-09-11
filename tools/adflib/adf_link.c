/*
 * ADF Library
 *
 * adf_link.c
 *
 */

#include<string.h>

#include"adf_defs.h"
#include"adf_str.h"
#include"adf_link.h"
#include"adf_dir.h"

extern struct Env adfEnv;

/*
 *
 *
 */
char* path(struct Volume *vol, SECTNUM parent)
{
    struct bEntryBlock entryBlk;
    char *tmpPath;
    int len;

    tmpPath = NULL;
    adfReadEntryBlock(vol, parent, &entryBlk);
    len = min(entryBlk.nameLen, MAXNAMELEN);
    memcpy(tmpPath,entryBlk.name,len);
    tmpPath[len]='\0';
/*    if (entryBlk.parent!=vol->rootBlock) {
        return(strcat(path(vol,entryBlk.parent), tmpPath));
    }
    else
   */     return(tmpPath);
}


/*
 *
 *
 */
RETCODE adfBlockPtr2EntryName(struct Volume *vol, SECTNUM nSect, SECTNUM lPar, 
	char **name, ULONG *size)
{
    struct bEntryBlock entryBlk;
    struct Entry entry;

    if (*name==0) {
        adfReadEntryBlock(vol, nSect, &entryBlk);
        *size = entryBlk.byteSize;
return RC_OK;
        adfEntBlock2Entry(&entryBlk, &entry);	//error
/*        if (entryBlk.secType!=ST_ROOT && entry.parent!=lPar)
            printf("path=%s\n",path(vol,entry.parent));
*/
       *name = strdup("");
        if (*name==NULL)
            return RC_MALLOC;
        return RC_OK;
    }
    else

    return RC_OK;
}

/*##################################################################################*/
