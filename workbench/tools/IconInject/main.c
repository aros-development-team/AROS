
#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <workbench/icon.h>
#include <stdio.h>
#include <string.h>

static const char version[] = "\0$VER: IconInject 0.1 (06.03.09) © The AROS Dev Team" ;

static
const UBYTE iitemplate[] =
"SOURCE,"
"TARGET,"
"IMAGE/S,"
"VERBOSE/S";

struct ArgList
{
    STRPTR      sourceIcon;
    STRPTR      targetIcon;
    IPTR        replaceimages;
    IPTR        verbose;
} arglist;

int main(int argc, char **argv)
{
    struct RDArgs *iireadargs;
    struct ArgList *IIArgList = &arglist;

    char *IISourceIcon = NULL;
    char *IITargetIcon = NULL;

    iireadargs = ReadArgs(iitemplate, (IPTR *)IIArgList, NULL);
    if (iireadargs)
    {
        if ((IIArgList->sourceIcon) && (IIArgList->targetIcon))
        {
            D(bug("[IconInject] Source @ %p, Dest @ %p\n", IIArgList->sourceIcon, IIArgList->targetIcon));
            D(bug("[IconInject] '%s' -> '%s'\n", IIArgList->sourceIcon, IIArgList->targetIcon));
            if ((strlen(IIArgList->sourceIcon) > 5) && (strncmp(IIArgList->sourceIcon + strlen(IIArgList->sourceIcon) - 5, ".info",5)))
            {
                if ((IISourceIcon = AllocVec(strlen(IIArgList->sourceIcon) - 4, MEMF_CLEAR)) != NULL)
                    CopyMem(IIArgList->sourceIcon, IISourceIcon, strlen(IIArgList->sourceIcon) - 5);
            }
            else
            {
#warning "TODO: Check if the name is a device and append disk.."
                if ((IISourceIcon = AllocVec(strlen(IIArgList->sourceIcon) + 1, MEMF_CLEAR)) != NULL)
                    CopyMem(IIArgList->sourceIcon, IISourceIcon, strlen(IIArgList->sourceIcon) - 5);
            }

            if ((strlen(IIArgList->targetIcon) > 5) && (strncmp(IIArgList->targetIcon + strlen(IIArgList->targetIcon) - 5, ".info",5)))
            {
                if ((IITargetIcon = AllocVec(strlen(IIArgList->targetIcon) - 4, MEMF_CLEAR)) != NULL)
                    CopyMem(IIArgList->targetIcon, IITargetIcon, strlen(IIArgList->targetIcon) - 5);
            }
            else
            {
#warning "TODO: Check if the name is a device and append disk.."
                if ((IITargetIcon = AllocVec(strlen(IIArgList->targetIcon) + 1, MEMF_CLEAR)) != NULL)
                    CopyMem(IIArgList->targetIcon, IITargetIcon, strlen(IIArgList->targetIcon) - 5);
            }

            if (IISourceIcon && IITargetIcon)
            {
                struct DiskObject       *sourceIconDOB = NULL;
                struct DiskObject       *targetIconDOB = NULL;
                IPTR                    tmpGadRender = NULL;
                IPTR                    tmpSelRender = NULL;

                D(bug("[IconInject] '%s' -> '%s'\n", IISourceIcon, IITargetIcon));

                sourceIconDOB = GetIconTags
                        (
                            IISourceIcon, 
                            ICONGETA_FailIfUnavailable,        FALSE,
                            TAG_DONE
                        );

                targetIconDOB = GetIconTags
                        (
                            IITargetIcon, 
                            ICONGETA_FailIfUnavailable,        FALSE,
                            TAG_DONE
                        );

                if (sourceIconDOB && targetIconDOB)
                {
                    if (IIArgList->replaceimages)
                    {
                        if (IIArgList->verbose)
                            printf("Replacing ICON IMAGERY in %s.info from %s.info\n", IITargetIcon, IISourceIcon);

                        tmpGadRender = targetIconDOB->do_Gadget.GadgetRender;
                        tmpSelRender = targetIconDOB->do_Gadget.SelectRender;

                        targetIconDOB->do_Gadget.GadgetRender = sourceIconDOB->do_Gadget.GadgetRender;
                        targetIconDOB->do_Gadget.SelectRender = sourceIconDOB->do_Gadget.SelectRender;
                    }

                    // Update the icon ..
                    PutIconTagList(IITargetIcon, targetIconDOB, NULL);

                    if (IIArgList->replaceimages)
                    {
                        targetIconDOB->do_Gadget.GadgetRender = tmpGadRender;
                        targetIconDOB->do_Gadget.SelectRender = tmpSelRender;
                    }
                }
                {
                    // Couldnt obtain one of the Icons ..
                }
            }
            else
            {
                //Failed to allocate Icon name buffer(s) ..
            }
        }
        else
        {
            //Missing arguments.. display a gui?
            printf("no args ..\n");
        }
    }
    if (IISourceIcon) FreeVec(IISourceIcon);
    if (IITargetIcon) FreeVec(IITargetIcon);
    return RETURN_OK;
}
