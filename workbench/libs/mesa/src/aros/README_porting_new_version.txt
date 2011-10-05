This is a easy, 29-step procedure for porting new versions of Mesa.


 1. Download archive MesaLib-Y.XX

 2. Unpack, Mesa-Y.XX

 3. Duplicate Mesa-Y.XX directory to Mesa-Y.XX-staging

 4. Compare Mesa-Y.XX-staging to previous Mesa-Y.TT-staging or to Mesa found in 
    AROS source tree. Remove from Mesa-Y.XX-staging all directories which are
    not present in current AROS Mesa source tree. Don't remove the new files,
    except for files in include/GL and in ./.
   
    diff -r Mesa-Y.TT-staging/ Mesa-Y.XX-staging/ | grep "Only in Mesa-Y.XX"
   
 5. Compare Mesa found in AROS source tree with Mesa-Y.XX-staging. Remove from
    Mesa found in AROS source tree any files and directories not found in
    Mesa-Y.XX-staging.
   
    diff -r -x*aros* -xmmakefile.src AROS/workbench/libs/mesa/ Mesa-X.YY-staging/ | grep "Only in AROS/workbench/libs/mesa/"

 6. Copy Mesa-Y.XX-staging content over to AROS/workbench/libs/mesa overwritting
    all files.

 7. Apply mesa.diff patch. Resolve all conflicts.

    patch -p1 < ./mesa/src/aros/mesa.diff

 8. Increase version in AROS/workbench/libs/mesa/src/gallium/include/pipe/p_aros_version.h

 9. Make galliumauxiliary.lib building. This might require updating list of galliumauxiliary.lib
    source files in mmakefile.src

    make linklibs-galliumauxiliary

10. Make gallium.hidd building

    make hidd-gallium

11. Make gallium.library building

    make workbench-libs-gallium

12. Regenerate api_exec_es1.c and api_exec_es2.c

    make workbench-libs-mesa-module-generated

13. Regenerate MAPI/GLAPI files

    cd AROS/workbench/libs/mesa/src/mapi/glapi/gen
    make clean
    make mesa

14. Regenerate mesa.library interface, stub and config files
    
    Use AROS/workbench/libs/mesa/src/aros/misc/mesastubsgenerator.cs. This is
    a C# program which will generate the needed files. You can run it from
    MonoDevelop. Be sure to configure the paths (in Main function).

    Copy & merged generate arosmesa.conf, arosmesa_mangle.h, arosmesa_library_api.c
    and arosmesaapim.h to AROS/workbench/libs/mesa/src/aros/arosmesa. Be sure 
    to check the diffs - changes should only consist of adding entries at the 
    end. Especially check arosmesa.conf file diff in LVO's has not changed.
    
15. Update mesa.library version in arosmesa.conf

    Increase the version part of version.revision version string and set 
    revision part to 0 if new functions have been added to the library LVO table.
    
    Increate only the revision part if no new functions have been added to
    library LVO table.

17. Regenerate glsl/builtin_functions.cpp

    a) Copy Mesa-Y.XX to Mesa-Y.XX-temp
    b) cd Mesa-Y.XX-temp/src/glsl
    c) Apply following changes to Makefile:
        comment out 'include $(TOP)/configs/current'
        rename $(APP_CXX) to (CXX)
    d) make builtin_function.cpp

    Copy builtin_function.cpp to AROS/workbench/libs/mesa/src/glsl.
    Discard Mesa-Y.XX-temp

16. Make mesa.library building

    make workbench-libs-mesa

17. Make softpipe.hidd building

    make hidd-softpipe

18. Regenerate glu.library interface, stub and config files
   
    The needed files have already been generated in step 14.
    
    Copy & merged generate glu.conf, glu_mangle.h, glu_library_api.c
    and gluapim.h to AROS/workbench/libs/mesa/src/aros/glu. Be sure 
    to check the diffs - changes should only consist of adding entries at the 
    end. Especially check glu.conf file diff in LVO's has not changed.

19. Update glu.library version in glu.conf

    Increase the version part of version.revision version string and set 
    revision part to 0 if new functions have been added to the library LVO table.
    
    Increate only the revision part if no new functions have been added to
    library LVO table.

20. Make glu.library building

    make workbench-libs-glu
   
21. Regenerate vega.library interface, stub and config files
   
    The needed files have already been generated in step 14.
    
    Copy & merged generate vega.conf, vg_mangle.h, vega_library_api.c
    and vgapim.h to AROS/workbench/libs/mesa/src/aros/vega. Be sure 
    to check the diffs - changes should only consist of adding entries at the 
    end. Especially check glu.conf file diff in LVO's has not changed.

22. Update vega.library version in vega.conf

    Increase the version part of version.revision version string and set 
    revision part to 0 if new functions have been added to the library LVO table.
    
    Increate only the revision part if no new functions have been added to
    library LVO table.

23. Make vega.library building

    make workbench-libs-vega

24. Regenerate egl.library interface, stub and config files
   
    The needed files have already been generated in step 14.
    
    Copy & merged generate egl.conf, egl_mangle.h, egl_library_api.c
    and eglapim.h to AROS/workbench/libs/mesa/src/aros/egl. Be sure 
    to check the diffs - changes should only consist of adding entries at the 
    end. Especially check glu.conf file diff in LVO's has not changed.

25. Update egl.library version in egl.conf

    Increase the version part of version.revision version string and set 
    revision part to 0 if new functions have been added to the library LVO table.
    
    Increate only the revision part if no new functions have been added to
    library LVO table.

26. Make egl.library building

    make workbench-libs-egl

27. Make nouveau.hidd building

    make hidd-nouveau

28. Update revision numbers of gallium.library, gallium.hidd, softpipe.hidd and nouveau.hidd

29. Generate the diff file:

    diff -ur --exclude-from=./mesa/src/aros/mesa.diff.excluded -I '^.*\bRevision.*Date.*\b.*$' Mesa-Y.XX-staging ./mesa/ | grep -v "Only in" > ./mesa/src/aros/mesa.diff
   
   
General remarks:

- If at any point you will have to bring back some of the files deleted in step
  4, be sure to add them to Mesa-Y.XX-staging directory as well. Mesa-Y.XX-staging
  should always be kept in synch with Mesa source codes in AROS tree.
  
  Mesa-Y.XX-staging + mesa.diff = Mesa source codes in AROS tree

- If nouveau.hidd requires some changes in order to build with new mesa, be sure
  to update the respective patches in hidd.nouveau and hidd.nouveau/drm directory.

- If mesa introduces new files which has the same name as nouveau drm files,
  rename the files in nouveau/drm. Generally try to keep changes to mesa to
  minimum.
   


