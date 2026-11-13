#include "defs.h"

Object *MyLLabel1 (const char *name)

{
return (TextObject,
          MUIA_Text_Contents, name,
          MUIA_Text_PreParse, MUIX_L,
/*          MUIA_Text_SetMax, TRUE, */
          MUIA_FramePhantomHoriz, TRUE,
          ButtonFrame, 
        End);
}

Object *MyLLabel2 (const char *name)

{
return (TextObject,
          MUIA_Text_Contents, name,
          MUIA_Text_PreParse, MUIX_L,
/*          MUIA_Text_SetMax, TRUE, */
          MUIA_FramePhantomHoriz, TRUE,
          StringFrame, 
        End);
}

Object *NumberString (ULONG len)

{
return (StringObject,
          MUIA_String_Accept, "0123456789",
          MUIA_String_MaxLen, len,
          MUIA_String_Format, MUIV_String_Format_Right,
          MUIA_String_Integer, 0,
          StringFrame,
        End);
}


Object *CreateMiscPage (void)

{
GR_StatParams = VGroup,
                  MUIA_Frame, MUIV_Frame_Group,
                  MUIA_HelpNode, "Misc_Settings",
                  MUIA_Weight, 200,
                  Child, 
                    HGroup,
                      Child, Label1 (_(msgZoomed)),
                      Child, CM_Zoomed = CheckMark (FALSE),
                    End,
                  Child, VSpace (0),
                  Child, Label (_(msgPosUnzoomed)),
                  Child,
                    HGroup,
                      Child, Label2 (_(msgLeftEdge)),
                      Child, ST_UnZLeft = NumberString (20),
                      Child, HSpace (0),
                      Child, Label2 (_(msgTopEdge)),
                      Child, ST_UnZTop = NumberString (20),
                    End,
                  Child, VSpace (0),
                  Child, Label (_(msgPosZoomed)),
                  Child,
                    HGroup,
                      Child, Label2 (_(msgLeftEdge)),
                      Child, ST_ZLeft = NumberString (20),
                      Child, HSpace (0),
                      Child, Label2 (_(msgTopEdge)),
                      Child, ST_ZTop = NumberString (20),
                    End,
                End;

CM_StatEnabled = CheckMark (TRUE);

DoMethod (CM_StatEnabled, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
          GR_StatParams,
          3,
          MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

GR_Statistics = VGroup,
                  Child, HVSpace,
                  Child, 
                    HGroup,
                      Child, Label1 (_(msgStatistics)),
                      Child, CM_StatEnabled,
                    End,
                  Child, HVSpace,
                  Child, GR_StatParams,
                End;

GR_Misc = ColGroup (2),
            MUIA_Group_SameWidth, TRUE, 
            Child, HVSpace,
            Child, HVSpace,
#if defined(PLATFORM_HASZ2)
            Child, MyLLabel1 (_(msgCacheZ2RAM)),
            Child, CM_CacheZ2RAM = CheckMark (TRUE),
#endif
            Child, MyLLabel1 (_(msgPatchWB)),
            Child, CM_WBPatch = CheckMark (FALSE),
            Child, MyLLabel1 (_(msgMemTracking)),
            Child, CM_MemTracking = CheckMark (FALSE),
#if defined(PLATFORM_HASFASTROM)
            Child, MyLLabel1 (_(msgFastROM)),
            Child, CM_FastROM = CheckMark (FALSE),
#endif
            Child, HVSpace,
            Child, HVSpace,
            Child, MyLLabel2 (_(msgMinVMAlloc)),
            Child, ST_MinVMAlloc = NumberString (10),
            Child, HVSpace,
            Child, HVSpace,
            Child, MyLLabel2 (_(msgEnableHotkey)),
            Child, ST_EnableHotkey = String (NULL, 40),
            Child, MyLLabel2 (_(msgDisableHotkey)),
            Child, ST_DisableHotkey = String (NULL, 40),
          End;

return (HGroup,
          Child, HCenter (GR_Statistics),
          Child, HSpace (0),
          Child, 
            RectangleObject,
              MUIA_Rectangle_VBar, TRUE,
              MUIA_Weight, 0,
            End,
          Child, HSpace (0),
          Child, GR_Misc,
        End);
}
