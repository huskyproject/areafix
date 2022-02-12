# areafix/Makefile
#
# This file is part of areafix, part of the Husky fidonet software project
# Use with GNU make v.3.82 or later
# Requires: husky enviroment
#

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
    ifeq ($(findstring Windows,$(OS)),)
        areafix_TARGET = $(areafix_TARGETDLL).$(areafix_VER)
    else
        areafix_TARGET = $(areafix_TARGETDLL)
    endif
else
    areafix_TARGET = $(areafix_TARGETLIB)
endif

areafix_TARGET_BLD = $(areafix_BUILDDIR)$(areafix_TARGET)

# List of HUSKY libraries required to build binary file(s)
areafix_LIBS := $(fidoconf_TARGET_BLD) $(smapi_TARGET_BLD) \
                $(huskylib_TARGET_BLD)

areafix_CDEFS := $(CDEFS) -I$(areafix_ROOTDIR) \
                          -I$(areafix_ROOTDIR)$(areafix_H_DIR) \
                          -I$(fidoconf_ROOTDIR) \
                          -I$(smapi_ROOTDIR) \
                          -I$(huskylib_ROOTDIR)

.PHONY: areafix_build areafix_install areafix_install-dynlib \
    areafix_uninstall areafix_clean areafix_distclean areafix_depend \
    areafix_rm_OBJS areafix_rm_BLD areafix_rm_DEP areafix_rm_DEPS

areafix_build: $(areafix_TARGET_BLD)

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
$(areafix_OBJDIR)$(areafix_TARGETLIB): $(areafix_ALL_OBJS) | do_not_run_make_as_root
	cd $(areafix_OBJDIR); $(AR) $(AR_R) $(areafix_TARGETLIB) $(^F)
ifdef RANLIB
	cd $(areafix_OBJDIR); $(RANLIB) $(areafix_TARGETLIB)
endif

# Build the dynamic library
ifeq ($(DYNLIBS),1)
$(areafix_OBJDIR)$(areafix_TARGET): \
    $(areafix_ALL_OBJS) $(areafix_LIBS) | do_not_run_make_as_root
    ifeq ($(filter gcc clang,$(MKSHARED)),)
		$(LD) $(LFLAGS) -o $@ $^
    else
		$(CC) $(LFLAGS) -shared -Wl,-soname,$(areafix_TARGET) -o $@ $^
    endif
endif

# Compile .c files
$(areafix_ALL_OBJS): $(areafix_OBJDIR)%$(_OBJ): $(areafix_SRCDIR)%.c | $(areafix_OBJDIR)
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
	-$(RM) $(RMOPT) $(areafix_BUILDDIR)$(areafix_TARGETLIB)
	-$(RM) $(RMOPT) $(areafix_BUILDDIR)$(areafix_TARGETDLL)*

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
