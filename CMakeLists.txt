cmake_minimum_required(VERSION 2.8.11)
PROJECT(areafix C)
set(areafix_VERSION 1.9.0)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Build shared libs" ON)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  ADD_DEFINITIONS(-g -DDEBUG -D_DEBUG)
  set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE STRING "My multi config types" FORCE)
else(CMAKE_BUILD_TYPE MATCHES Debug)
  ADD_DEFINITIONS(-DNDEBUG)
  set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "My multi config types" FORCE)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

if (MSVC)
  if (BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-D_DLL -D_MAKE_DLL)
  else()
    set(CompilerFlags
      CMAKE_CXX_FLAGS
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_C_FLAGS
      CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_RELEASE
    )
    foreach(CompilerFlag ${CompilerFlags})
      string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
  endif()
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../out/lib")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../out/bin")
  ADD_DEFINITIONS(-DWIN32 -D_WINDOWS -D_CONSOLE -D_CRT_SECURE_NO_WARNINGS)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/areafix;${CMAKE_CURRENT_SOURCE_DIR}/../huskylib;${CMAKE_CURRENT_SOURCE_DIR}/../fidoconf;;${CMAKE_CURRENT_SOURCE_DIR}/../smapi")
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    list( APPEND CMAKE_FIND_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../out/lib/Debug" )
  else(CMAKE_BUILD_TYPE MATCHES Debug)
    list( APPEND CMAKE_FIND_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../out/lib/Release" )
  endif(CMAKE_BUILD_TYPE MATCHES Debug)
else ()
  include_directories("areafix")
  ADD_DEFINITIONS(-Wall -DUNIX)
endif (MSVC)

find_library(husky_LIB NAMES husky PATHS "${CMAKE_FIND_ROOT_PATH}")
find_library(fidoconfig_LIB NAMES fidoconfig PATHS "${CMAKE_FIND_ROOT_PATH}")
find_library(smapi_LIB NAMES smapi PATHS "${CMAKE_FIND_ROOT_PATH}")

message("-------------------------")
message("System            : ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message("Compiler          : ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message("Prefix            : ${CMAKE_INSTALL_PREFIX} (run \"cmake . -DCMAKE_INSTALL_PREFIX:PATH=/other\" for other prefix)")
message("libhusky          : ${husky_LIB}")
message("libfidoconfig     : ${fidoconfig_LIB}")
message("libsmapi          : ${smapi_LIB}")
message("Build shared      : ${BUILD_SHARED_LIBS}")
message("Build type        : ${CMAKE_CONFIGURATION_TYPES}")
message("-------------------------")

SET(areafix_SOURCES 
  src/afglobal.c
  src/areafix.c
  src/callback.c
  src/query.c
  src/version.c
)
SET(areafix_HEADERS
  areafix/afglobal.h
  areafix/areafix.h
  areafix/callback.h
  areafix/query.h
  areafix/version.h
)

ADD_LIBRARY(areafix ${areafix_SOURCES})
set_target_properties(areafix PROPERTIES VERSION ${areafix_VERSION})
set_target_properties(areafix PROPERTIES PUBLIC_HEADER "${areafix_HEADERS}")
target_link_libraries(areafix ${husky_LIB} ${fidoconfig_LIB} ${smapi_LIB})
if (NOT MSVC)
INSTALL(TARGETS areafix
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION include/areafix
)
  set(CPACK_GENERATOR ${CPACK_GENERATOR} TGZ)
endif (NOT MSVC)
set (CPACK_PACKAGE_VERSION_MAJOR "1")
set (CPACK_PACKAGE_VERSION_MINOR "9")
file(READ cvsdate.h CPACK_PACKAGE_VERSION_PATCH LIMIT 10 OFFSET 17)
string(SUBSTRING ${CPACK_PACKAGE_VERSION_PATCH} 0 10 CPACK_PACKAGE_VERSION_PATCH)
string(REGEX REPLACE "-" "" CPACK_PACKAGE_VERSION_PATCH ${CPACK_PACKAGE_VERSION_PATCH})
INCLUDE(CPack)
