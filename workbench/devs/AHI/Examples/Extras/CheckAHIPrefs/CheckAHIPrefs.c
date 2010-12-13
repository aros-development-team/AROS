
/* Just a small example how to read the settings file. */

#include <devices/ahi.h>
#include <dos/dos.h>
#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>

#include <proto/dos.h>
#include <proto/iffparse.h>

int main(int argc, char *argv[])
{
  struct IFFHandle *iff;
  struct StoredProperty *ahig;
  struct CollectionItem *ci;
  LONG unit = 0;
  int rc = RETURN_OK;

  if(argc != 3) {
    Printf("Usage: %s FILE UNIT\n", argv[0]);
    return RETURN_FAIL;
  }

  StrToLong(argv[2], &unit);

  if(iff = AllocIFF())
  {
    iff->iff_Stream = Open(argv[1], MODE_OLDFILE);
    if(iff->iff_Stream)
    {
      InitIFFasDOS(iff);
      if(!OpenIFF(iff, IFFF_READ))
      {
        if(!(PropChunk(iff,ID_PREF,ID_AHIG)
          || CollectionChunk(iff,ID_PREF,ID_AHIU)
          || StopOnExit(iff,ID_PREF,ID_FORM)))
        {
          if(ParseIFF(iff, IFFPARSE_SCAN) == IFFERR_EOC)
          {

            ahig = FindProp(iff,ID_PREF,ID_AHIG);
            if(ahig)
            {
              struct AHIGlobalPrefs *globalprefs;
              globalprefs = (struct AHIGlobalPrefs *)ahig->sp_Data;

              if(globalprefs->ahigp_DebugLevel != AHI_DEBUG_NONE)
              {
                Printf("Debugging is turned on.\n");
                rc = RETURN_WARN;
              }
            }

            ci = FindCollection(iff,ID_PREF,ID_AHIU);
            while(ci)
            {
              struct AHIUnitPrefs *unitprefs;
              unitprefs = (struct AHIUnitPrefs *)ci->ci_Data;

              if(unitprefs->ahiup_Unit == unit)
              {
                if(unitprefs->ahiup_Channels < 2)
                {
                  Printf("There are less than 2 channels selected for unit %ld.\n", unit);
                  rc = RETURN_WARN;
                }
              }
              ci=ci->ci_Next;
            }

          }
        }
        CloseIFF(iff);
      }
      Close(iff->iff_Stream);
    }
    FreeIFF(iff);
  }

  return rc;
}
