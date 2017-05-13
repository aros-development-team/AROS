boot/amiga/AROSBootstrap ROM boot/amiga/aros.hunk.gz

MakeDir RAM:ENV
MakeDir RAM:T
MakeDir RAM:Clipboards
Assign  ENV:     RAM:ENV
Assign  T:       RAM:T
Assign  CLIPS:   RAM:Clipboards

Mount >NIL: DEVS:DOSDrivers/~(#?.info)

If EXISTS "AROS Live CD:"
  Assign "SYS:" "AROS Live CD:"
  If EXISTS "SYS:S"
    Assign "S:" "SYS:S"
  EndIf
  If EXISTS "SYS:L"
    Assign "L:" "SYS:L"
  EndIf
  If EXISTS "SYS:C"
    Assign "C:" "SYS:C"
  EndIf
  If EXISTS "SYS:Libs"
    Assign "LIBS:" "SYS:Libs"
  EndIf
  If EXISTS "SYS:L"
    Assign "FONTS:" "SYS:Fonts"
  EndIf
  If EXISTS "SYS:Devs"
    Assign "DEVS:" "SYS:Devs"
    If EXISTS "DEVS:Driver"
        Assign "DRIVERS:" "DEVS:Drivers"
        Assign "LIBS:" "DEVS:Drivers" ADD
    EndIf
  EndIf
  If EXISTS "C:Avail"
    C:Avail TOTAL >ENV:AvailMem
  Endif
  If VAL "$AvailMem" GE "8388608"
    If EXISTS "S:Startup-Sequence"
      If EXISTS "ENV:AvailMem"
        C:Delete QUIET ENV:AvailMem
      Endif
      Execute S:Startup-Sequence
      EndCLI
    EndIf
  EndIf
  If EXISTS "ENV:AvailMem"
    C:Delete QUIET ENV:AvailMem
  Endif
  Assign LIBS: SYS:Classes ADD
  Assign IMAGES: SYS:System/Images DEFER
  Assign LOCALE: SYS:Locale
  Assign HELP:   LOCALE:Help DEFER
EndIf

LoadWB
EndCLI
