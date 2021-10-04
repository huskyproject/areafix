# areafix/Makefile
#
# This file is part of areafix, part of the Husky fidonet software project
# Use with GNU make v.3.82 or later
# Requires: husky enviroment
#

# Version
areafix_g1:=$(GREP) -Po 'define\s+AF_VER_MAJOR\s+\K\d+'
areafix_g2:=$(GREP) -Po 'define\s+AF_VER_MINOR\s+\K\d+'
areafix_g3:=$(GREP) -Po 'define\s+AF_VER_PATCH\s+\K\d+'
areafix_g4:=$(GREP) -Po 'char\s+cvs_date\[\]\s*=\s*"\K\d+-\d+-\d+'
areafix_VERMAJOR := $(shell $(areafix_g1) $(areafix_ROOTDIR)$(areafix_H_DIR)version.h)
areafix_VERMINOR := $(shell $(areafix_g2) $(areafix_ROOTDIR)$(areafix_H_DIR)version.h)
areafix_VERPATCH := $(shell $(areafix_g3) $(areafix_ROOTDIR)$(areafix_H_DIR)version.h)
areafix_VERH     := $(areafix_VERMAJOR).$(areafix_VERMINOR)
areafix_cvsdate := $(shell $(areafix_g4) $(areafix_ROOTDIR)cvsdate.h)
areafix_reldate := $(subst -,,$(areafix_cvsdate))
areafix_VER      := $(areafix_VERH).$(areafix_reldate)

# Object files: library (please sort list to easy check by human)
areafix_OBJFILES = $(O)afglobal$(_OBJ) $(O)areafix$(_OBJ) $(O)callback$(_OBJ) \
	$(O)query$(_OBJ) $(O)version$(_OBJ)

# Object files: programs
areafix_PROG_OBJ =

# Executable file(s) to build from sources
areafix_PROGS =

# Static and dynamic target libraries
areafix_TARGETLIB = $(L)$(LIBPREFIX)$(areafix_LIBNAME)$(LIBSUFFIX)$(_LIB)
areafix_TARGETDLL = $(B)$(DLLPREFIX)$(areafix_LIBNAME)$(DLLSUFFIX)$(_DLL)

ifeq ($(DYNLIBS), 1)
    areafix_TARGET = $(areafix_TARGETDLL).$(areafix_VER)
else
    areafix_TARGET = $(areafix_TARGETLIB)
endif

areafix_TARGET_BLD = $(areafix_BUILDDIR)$(areafix_TARGET)

# List of HUSKY libraries required to build binary file(s)
areafix_LIBS =

areafix_OBJS := $(addprefix $(areafix_OBJDIR),$(areafix_OBJFILES))

areafix_DEPS := $(areafix_OBJFILES)
ifdef O
    areafix_DEPS := $(areafix_DEPS:$(O)=)
endif
ifdef _OBJ
    areafix_DEPS := $(areafix_DEPS:$(_OBJ)=$(_DEP))
else
    areafix_DEPS := $(addsuffix $(_DEP),$(areafix_DEPS))
endif
areafix_DEPS := $(addprefix $(areafix_DEPDIR),$(areafix_DEPS))

areafix_CDEFS := $(CDEFS) -I$(areafix_ROOTDIR) \
                          -I$(areafix_ROOTDIR)$(areafix_H_DIR) \
                          -I$(fidoconf_ROOTDIR) \
                          -I$(smapi_ROOTDIR) \
                          -I$(huskylib_ROOTDIR)

.PHONY: areafix_all areafix_install areafix_install-dynlib \
    areafix_uninstall areafix_clean areafix_distclean areafix_depend \
    areafix_rm_OBJS areafix_rm_BLD areafix_rm_DEP areafix_rm_DEPS

areafix_all: $(areafix_TARGET_BLD)

ifneq ($(MAKECMDGOALS), depend)
ifneq ($(MAKECMDGOALS), distclean)
ifneq ($(MAKECMDGOALS), uninstall)
    include $(areafix_DEPS)
endif
endif
endif

# Make a hard link of the library in $(areafix_BUILDDIR)
$(areafix_TARGET_BLD): $(areafix_OBJDIR)$(areafix_TARGET)
	$(LN) $(LNHOPT) $< $(areafix_BUILDDIR)

# Build the static library
$(areafix_OBJDIR)$(areafix_TARGETLIB): $(areafix_OBJS) | do_not_run_make_as_root
	cd $(areafix_OBJDIR); $(AR) $(AR_R) $(areafix_TARGETLIB) $(^F)
ifdef RANLIB
	cd $(areafix_OBJDIR); $(RANLIB) $(areafix_TARGETLIB)
endif

# Build the dynamic library
$(areafix_OBJDIR)$(areafix_TARGETDLL).$(areafix_VER): $(areafix_OBJS) | do_not_run_make_as_root
ifeq (~$(MKSHARED)~,~ld~)
	$(LD) $(LFLAGS) -o $(areafix_OBJDIR)$(areafix_TARGETDLL).$(areafix_VER) $(areafix_OBJS)
else
	$(CC) $(LFLAGS) -shared -Wl,-soname,$(areafix_TARGETDLL).$(areafix_VER) \
	-o $(areafix_OBJDIR)$(areafix_TARGETDLL).$(areafix_VER) $(areafix_OBJS)
endif

# Compile .c files
$(areafix_OBJS): $(areafix_OBJDIR)%$(_OBJ): $(areafix_SRCDIR)%.c | $(areafix_OBJDIR)
	$(CC) $(CFLAGS) $(areafix_CDEFS) -o $(areafix_OBJDIR)$*$(_OBJ) $(areafix_SRCDIR)$*.c

$(areafix_OBJDIR): | $(areafix_BUILDDIR) do_not_run_make_as_root
	[ -d $@ ] || $(MKDIR) $(MKDIROPT) $@


# Install
areafix_install: areafix_install-dynlib

ifneq ($(DYNLIBS), 1)
areafix_install-dynlib: ;
else
    ifneq ($(strip $(LDCONFIG)),)
        areafix_install-dynlib: \
        $(DESTDIR)$(LIBDIR)$(DIRSEP)$(areafix_TARGETDLL).$(areafix_VER)
		-@$(LDCONFIG) >& /dev/null || true
    else
        areafix_install-dynlib: \
        $(DESTDIR)$(LIBDIR)$(DIRSEP)$(areafix_TARGETDLL).$(areafix_VER) ;
    endif

    $(DESTDIR)$(LIBDIR)$(DIRSEP)$(areafix_TARGETDLL).$(areafix_VER): \
        $(areafix_BUILDDIR)$(areafix_TARGETDLL).$(areafix_VER) | \
        $(DESTDIR)$(LIBDIR)
		$(INSTALL) $(ILOPT) $< $(DESTDIR)$(LIBDIR); \
		cd $(DESTDIR)$(LIBDIR); \
		$(TOUCH) $(areafix_TARGETDLL).$(areafix_VER); \
		$(LN) $(LNOPT) $(areafix_TARGETDLL).$(areafix_VER) $(areafix_TARGETDLL).$(areafix_VERH); \
		$(LN) $(LNOPT) $(areafix_TARGETDLL).$(areafix_VER) $(areafix_TARGETDLL)
endif


# Clean
areafix_clean: areafix_rm_OBJS
	-[ -d "$(areafix_OBJDIR)" ] && $(RMDIR) $(areafix_OBJDIR) || true

areafix_rm_OBJS:
	-$(RM) $(RMOPT) $(areafix_OBJDIR)*


# Distclean
areafix_distclean: areafix_clean areafix_rm_BLD
	-[ -d "$(areafix_BUILDDIR)" ] && $(RMDIR) $(areafix_BUILDDIR) || true

areafix_rm_BLD: areafix_rm_DEP
	-$(RM) $(RMOPT) $(areafix_BUILDDIR)$(areafix_TARGET)

areafix_rm_DEP: areafix_rm_DEPS
	-[ -d "$(areafix_DEPDIR)" ] && $(RMDIR) $(areafix_DEPDIR) || true

areafix_rm_DEPS:
	-$(RM) $(RMOPT) $(areafix_DEPDIR)*


# Uninstall
ifeq ($(DYNLIBS), 1)
    areafix_uninstall:
		-$(RM) $(RMOPT) $(DESTDIR)$(LIBDIR)$(DIRSEP)$(areafix_TARGETDLL)*
else
    areafix_uninstall: ;
endif

# Depend
ifeq ($(MAKECMDGOALS),depend)
areafix_depend: $(areafix_DEPS) ;

# Build dependency makefiles for every source file
$(areafix_DEPS): $(areafix_DEPDIR)%$(_DEP): $(areafix_SRCDIR)%.c | $(areafix_DEPDIR)
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $(areafix_CDEFS) $< > $@.$$$$; \
	sed 's,\($*\)$(_OBJ)[ :]*,$(areafix_OBJDIR)\1$(_OBJ) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(areafix_DEPDIR): | $(areafix_BUILDDIR) do_not_run_depend_as_root
	[ -d $@ ] || $(MKDIR) $(MKDIROPT) $@
endif

$(areafix_BUILDDIR):
	[ -d $@ ] || $(MKDIR) $(MKDIROPT) $@
