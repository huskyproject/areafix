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
# _SRC_DIR  sources dir

# Version
VERMAJOR= 1
VERMINOR= 9
VERPATCH= 0
VERH	= $(VERMAJOR).$(VERMINOR)
VER	= $(VERH).$(VERPATCH)

# File name
LIBNAME = areafix
TARGETLIB = $(LIBPREFIX)$(LIBNAME)$(LIBSUFFIX)$(_LIB)
TARGETDLL = $(DLLPREFIX)$(LIBNAME)$(DLLSUFFIX)$(_DLL)

# Object files: programs
OBJS    = areafix$(_OBJ) query$(_OBJ) afglobal$(_OBJ) callback$(_OBJ) version$(_OBJ)

# Header files
HEADERS	= areafix.h query.h callback.h afglobal.h version.h

# Sources directory
_SRC_DIR= ../src/

# Headers directory
H_DIR	= ../areafix/
