# $Id$
#
# Common makefiles part (include-file) for libareafix

# Use defines before including this file:
# _OBJ	object files suffix begins with point (".o", ".obj", ... )
# _LIB	library files suffix begins with point (".a", ".lib", ... )
# _DLL	dinamic linking library files suffix begins with point (".dll", ".so", ...)
# LIBPREFIX  library filename prefix ("lib", "", ...)
# LIBSUFFIX  library filename suffix (compiler ID, three characters, somewhere empty)
# DLLPREFIX  dinamic linking library prefix (usually empty)
# DLLSUFFIX  dinamic linking library suffix (compiler ID or empty)
# O, B, L, H prefixes to use with names of objects, binaries, libraries and headers respectively

# Library name
LIBNAME = areafix

# Version
VERMAJOR= 1
VERMINOR= 9
VERPATCH= 0
VERH	= $(VERMAJOR).$(VERMINOR)
VER	= $(VERH).$(VERPATCH)

# Sources directory
_SRC_DIR= ../src/
# Headers directory
H_DIR	= ../areafix/

# Header files (please sort list to easy check by human)
HEADERS	= $(H)afglobal.h $(H)areafix.h $(H)callback.h $(H)query.h $(H)version.h

# Object files: library (please sort list to easy check by human)
OBJS    = $(O)afglobal$(_OBJ) $(O)areafix$(_OBJ) $(O)callback$(_OBJ) \
	$(O)query$(_OBJ) $(O)version$(_OBJ)

# Object files: programs
PRGOBJS	=

# Binary file(s) to build from sources
PROGRAMS=

# Usually compose from LIBPREFIX, LIBSUFFIX and LIB.
TARGETLIB = $(L)$(LIBPREFIX)$(LIBNAME)$(LIBSUFFIX)$(_LIB)
TARGETDLL = $(B)$(DLLPREFIX)$(LIBNAME)$(DLLSUFFIX)$(_DLL)

# List of HUSKY libraries required to build binary file(s)
LIBS	=
