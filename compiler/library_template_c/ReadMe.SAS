SAS/C V6.5x note:
-----------------
[Andreas Kleinert <Andreas_Kleinert@t-online.de>]


Of course you should now how to handle SAS/C before trying to build
such a thing like a shared library with it, but basically you just
need to type "smake" in the "source/lib_source" directory.

Relevant settings are in:  - smakefile      (SAS/C makefile)
                           - smakefile.wth  (to be processed by SLink)
                           - SCOPTIONS      (SAS/C compiler settings)

