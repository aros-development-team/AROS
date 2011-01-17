
/****************************************************************
   This file was created automatically by `FlexCat 2.4'
   from "/cygdrive/e/Private/Projects/AROS-Win32/contrib/necessary/AHI/Device/ahi.cd".

   Do NOT edit by hand!
****************************************************************/

#include "ahi_def.h"

#include <libraries/locale.h>
#include <proto/locale.h>

struct FC_Type
{   LONG   ID;
    STRPTR Str;
};

const struct FC_Type _msgDefault = { 0, "Default" };
const struct FC_Type _msgMenuControl = { 1, "\000\000Control" };
const struct FC_Type _msgMenuLastMode = { 2, "L\000Last Mode" };
const struct FC_Type _msgMenuNextMode = { 3, "N\000Next Mode" };
const struct FC_Type _msgMenuPropertyList = { 4, "?\000Property List..." };
const struct FC_Type _msgMenuRestore = { 5, "R\000Restore" };
const struct FC_Type _msgMenuOK = { 6, "O\000OK" };
const struct FC_Type _msgMenuCancel = { 7, "C\000Cancel" };
const struct FC_Type _msgUnknown = { 8, "UNKNOWN:Audio ID 0x%08lx" };
const struct FC_Type _msgReqOK = { 9, "OK" };
const struct FC_Type _msgReqCancel = { 10, "Cancel" };
const struct FC_Type _msgReqFrequency = { 11, "Frequency" };
const struct FC_Type _msgDefaultMode = { 12, "Default audio mode" };
const struct FC_Type _msgReqInfoTitle = { 13, "Mode Properties" };
const struct FC_Type _msgReqInfoAudioID = { 14, "Audio mode ID: 0x%08lx" };
const struct FC_Type _msgReqInfoResolution = { 15, "Resolution: %ld bit %s" };
const struct FC_Type _msgReqInfoMono = { 16, "mono" };
const struct FC_Type _msgReqInfoStereo = { 17, "stereo" };
const struct FC_Type _msgReqInfoStereoPan = { 18, "stereo with panning" };
const struct FC_Type _msgReqInfoChannels = { 19, "Channels: %ld" };
const struct FC_Type _msgReqInfoMixrate = { 20, "Mixing rates: %ld-%ld Hz" };
const struct FC_Type _msgReqInfoHiFi = { 21, "HiFi mixing" };
const struct FC_Type _msgReqInfoRecordHalf = { 22, "Record in half duplex" };
const struct FC_Type _msgReqInfoRecordFull = { 23, "Record in full duplex" };
const struct FC_Type _msgReqInfoMultiChannel = { 24, "7.1 multichannel" };
const struct FC_Type _msgFreqFmt = { 25, "%lu Hz" };

static struct Catalog *ahi_Catalog = NULL;

static const struct TagItem ahi_tags[] = {
  { OC_BuiltInLanguage, (IPTR)"english" },
  { OC_Version,         4 },
  { TAG_DONE,           0  }
};

void OpenahiCatalog(struct Locale *loc, STRPTR language)
{
  if(LocaleBase != NULL  &&  ahi_Catalog == NULL)
  {
    ahi_Catalog = OpenCatalog(loc, (STRPTR) "ahi.catalog",
        language ? OC_Language : TAG_IGNORE, (IPTR)language,
        TAG_MORE, ahi_tags);
  }
}

struct Catalog *ExtOpenCatalog(struct Locale *loc, STRPTR language)
{
  if(LocaleBase != NULL)
  {
    return OpenCatalog(loc, (STRPTR) "ahi.catalog",
        language ? OC_Language : TAG_IGNORE, (IPTR)language,
        TAG_MORE, ahi_tags);
  }
  return NULL;
}

void CloseahiCatalog(void)
{
  if (LocaleBase != NULL)
  {
    CloseCatalog(ahi_Catalog);
  }
  ahi_Catalog = NULL;
}

void ExtCloseCatalog(struct Catalog *catalog)
{
  if (LocaleBase != NULL)
  {
    CloseCatalog(catalog);
  }
}

STRPTR GetahiString(APTR fcstr)
{
  STRPTR defaultstr = ((struct FC_Type *)fcstr)->Str;
  if (ahi_Catalog)
    return (STRPTR) GetCatalogStr(ahi_Catalog, ((struct FC_Type *)fcstr)->ID, defaultstr);
  return defaultstr;
}

STRPTR GetString(APTR fcstr, struct Catalog *catalog)
{
  STRPTR defaultstr = ((struct FC_Type *)fcstr)->Str;
  if (catalog)
    return (STRPTR) GetCatalogStr(catalog, ((struct FC_Type *)fcstr)->ID, defaultstr);
  return defaultstr;
}
