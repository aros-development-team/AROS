info_TEXINFOS = grub.texi multiboot.texi
grub_TEXINFOS = tutorial.texi user-ref.texi prog-ref.texi \
	appendices.texi
EXAMPLES = boot.S kernel.c multiboot.h
multiboot_TEXINFOS = boot.S.texi kernel.c.texi multiboot.h.texi
man_MANS = grub.8 mbchk.1 grub-install.8
HELP2MAN = help2man
SRC2TEXI = src2texi
noinst_SCRIPTS = $(HELP2MAN) $(SRC2TEXI)

EXTRA_DIST = menu.lst $(man_MANS) $(noinst_SCRIPTS) \
	$(EXAMPLES) $(multiboot_TEXINFOS)

# Cancel the rule %.texi -> %. This rule may confuse make to determine
# the dependecies.
.texi:

%.c.texi: %.c $(srcdir)/$(SRC2TEXI)
	$(SHELL) $(srcdir)/$(SRC2TEXI) $(srcdir) $< $@

%.h.texi: %.h $(srcdir)/$(SRC2TEXI)
	$(SHELL) $(srcdir)/$(SRC2TEXI) $(srcdir) $< $@

%.S.texi: %.S $(srcdir)/$(SRC2TEXI)
	$(SHELL) $(srcdir)/$(SRC2TEXI) $(srcdir) $< $@

if MAINTAINER_MODE
$(srcdir)/grub.8: ../grub/grub $(srcdir)/$(HELP2MAN)
	$(PERL) $(srcdir)/$(HELP2MAN) --name="the grub shell" \
		--section=8 --output=$@ $<

$(srcdir)/grub-install.8: ../util/grub-install $(srcdir)/$(HELP2MAN)
	chmod 755 $<
	$(PERL) $(srcdir)/$(HELP2MAN) --name="install GRUB on your drive" \
		--section=8 --output=$@ $<

$(srcdir)/mbchk.1: ../util/mbchk $(srcdir)/$(HELP2MAN)
	$(PERL) $(srcdir)/$(HELP2MAN) \
		--name="check the format of a Multiboot kernel" \
		--section=1 --output=$@ $<
endif
