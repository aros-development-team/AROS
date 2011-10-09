boot/AROSBootstrap ROM boot/aros.hunk.gz

MakeDir RAM:ENV
MakeDir RAM:T
Assign  ENV:     RAM:ENV
Assign  T:       RAM:T

Mount >NIL: DEVS:DOSDrivers/~(#?.info)

LoadWB
EndCLI
