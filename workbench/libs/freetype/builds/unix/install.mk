#
# FreeType 2 installation instructions for Unix systems
#


# Copyright 1996-2000, 2002 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


.PHONY: install uninstall check

# Unix installation and deinstallation targets.
install: $(PROJECT_LIBRARY)
	$(MKINSTALLDIRS) $(libdir)                                 \
                         $(includedir)/freetype2/freetype/config   \
                         $(includedir)/freetype2/freetype/internal \
                         $(includedir)/freetype2/freetype/cache    \
                         $(bindir)                                 \
                         $(prefix)/share/aclocal
	$(LIBTOOL) --mode=install $(INSTALL) $(PROJECT_LIBRARY) $(libdir)
	-for P in $(PUBLIC_H) ; do                               \
          $(INSTALL_DATA) $$P $(includedir)/freetype2/freetype ; \
        done
	-for P in $(BASE_H) ; do                                          \
          $(INSTALL_DATA) $$P $(includedir)/freetype2/freetype/internal ; \
        done
	-for P in $(CONFIG_H) ; do                                      \
          $(INSTALL_DATA) $$P $(includedir)/freetype2/freetype/config ; \
        done
	-for P in $(CACHE_H) ; do                                      \
          $(INSTALL_DATA) $$P $(includedir)/freetype2/freetype/cache ; \
        done
	$(INSTALL_DATA) $(BUILD)/ft2unix.h $(includedir)/ft2build.h
	$(INSTALL_SCRIPT) -m 755 $(OBJ_BUILD)/freetype-config \
          $(bindir)/freetype-config
	$(INSTALL_SCRIPT) -m 644 $(BUILD)/freetype2.m4 \
          $(prefix)/share/aclocal/freetype2.m4


uninstall:
	-$(LIBTOOL) --mode=uninstall $(RM) $(libdir)/$(LIBRARY).$A
	-$(DELETE) $(includedir)/freetype2/freetype/cache/*
	-$(DELDIR) $(includedir)/freetype2/freetype/cache
	-$(DELETE) $(includedir)/freetype2/freetype/config/*
	-$(DELDIR) $(includedir)/freetype2/freetype/config
	-$(DELETE) $(includedir)/freetype2/freetype/internal/*
	-$(DELDIR) $(includedir)/freetype2/freetype/internal
	-$(DELETE) $(includedir)/freetype2/freetype/*
	-$(DELDIR) $(includedir)/freetype2/freetype
	-$(DELDIR) $(includedir)/freetype2
	-$(DELETE) $(includedir)/ft2build.h
	-$(DELETE) $(bindir)/freetype-config
	-$(DELETE) $(prefix)/share/aclocal/freetype2.m4


check:
	@echo There is no validation suite for this package.


.PHONY: clean_project_unix distclean_project_unix

# Unix cleaning and distclean rules.
#
clean_project_unix:
	-$(DELETE) $(BASE_OBJECTS) $(OBJ_M) $(OBJ_S)
	-$(DELETE) $(patsubst %.$O,%.$(SO),$(BASE_OBJECTS) $(OBJ_M) $(OBJ_S)) \
                   $(CLEAN)

distclean_project_unix: clean_project_unix
	-$(DELETE) $(PROJECT_LIBRARY)
	-$(DELETE) $(OBJ_DIR)/.libs/*
	-$(DELDIR) $(OBJ_DIR)/.libs
	-$(DELETE) *.orig *~ core *.core $(DISTCLEAN)

# EOF
