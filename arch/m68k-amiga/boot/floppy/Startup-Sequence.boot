FailAt 21
boot/AROSBootstrap ROM boot/aros.elf.gz

MakeDir RAM:ENV
MakeDir RAM:T
Assign  ENV:     RAM:ENV
Assign  T:       RAM:T

Mount >NIL: DEVS:DOSDrivers/~(#?.info)

LoadWB
EndCLI
