# CMakeLists.txt -- Build system for the XRF-Maps
#
#Copyright (c) 2019, UChicago Argonne, LLC. All rights reserved.
#
#Copyright 2019. UChicago Argonne, LLC. This software was produced
#under U.S. Government contract DE-AC02-06CH11357 for Argonne National
#Laboratory (ANL), which is operated by UChicago Argonne, LLC for the
#U.S. Department of Energy. The U.S. Government has rights to use,
#reproduce, and distribute this software.  NEITHER THE GOVERNMENT NOR
#UChicago Argonne, LLC MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
#ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  If software is
#modified to produce derivative works, such modified software should
#be clearly marked, so as not to confuse it with the version available
#from ANL.
#
#Additionally, redistribution and use in source and binary forms, with
#or without modification, are permitted provided that the following
#conditions are met:
#
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#
#    * Neither the name of UChicago Argonne, LLC, Argonne National
#      Laboratory, ANL, the U.S. Government, nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY UChicago Argonne, LLC AND CONTRIBUTORS
#"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
#FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL UChicago
#Argonne, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
#ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#POSSIBILITY OF SUCH DAMAGE.


cmake_minimum_required(VERSION 3.5)

project(uProbeX)

#include(${PROJECT_SOURCE_DIR}/../XRF-Maps/build/XRF_MapsConfig.cmake)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/" ${PROJECT_SOURCE_DIR}/cmake ${PROJECT_SOURCE_DIR}/../XRF-Maps/build)
#set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PROJECT_SOURCE_DIR}/../XRF-Maps/build )

option(BUILD_WITH_TIRPC "Build tripc library for libxrpc (RedHat 8)" OFF)

IF(UNIX)
  IF(BUILD_WITH_TIRPC)
    set(GCC_INCLUDE_DIR "/usr/include/tirpc/" CACHE PATH "include folder for xrpc (red hat)") 
    include_directories(${GCC_INCLUDE_DIR})
  ENDIF()
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/linux)
ENDIF()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# If compiled on some intel mahcines this causes crashes so let user set it for compile
option(AVX2 "Compule with arch AVX2 on MSVC" OFF)
option(AVX "Compule with arch AVX on MSVC" OFF)
option(SSE2 "Compule with arch SSE2 on MSVC" OFF)
option(MSVC_DEBUG_INFO_REL "Compule with debug info in Release build for MSVC" OFF)
option(BUILD_WITH_OPENCV_SUPPORT "Compule with opencv support for image segmentation" ON)

IF (UNIX)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/linux)
ENDIF()

find_package(Threads)
find_package(hdf5 CONFIG REQUIRED)
find_package(netCDF CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(cppzmq CONFIG REQUIRED)
find_package(TIFF REQUIRED)
find_package(XRF_Maps CONFIG REQUIRED COMPONENTS libxrf_io libxrf_fit)
find_package(NLopt CONFIG REQUIRED)

IF (BUILD_WITH_OPENCV_SUPPORT)
  add_definitions(-D_BUILD_WITH_OPENCV)
  find_package(OpenCV CONFIG REQUIRED)
ENDIF()

if (libxrf_fit_FOUND)
  message("Found libxrf_fit")
endif()

if (libxrf_io_FOUND)
  message("Found libxrf_io")
endif()

add_definitions(-D_BUILD_WITH_ZMQ)
add_definitions(-DZMQ_STATIC)

# Opitons for getting YouCompleteMe plugin working with this project
SET( CMAKE_EXPORT_COMPILE_COMMANDS ON )

IF( EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json" )
  EXECUTE_PROCESS( COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
  )
ENDIF()

IF(NOT DEFINED CMAKE_PREFIX_PATH)
  set(CMAKE_PREFIX_PATH CACHE PATH "")
  MESSAGE( "IF qt6-config.cmake is not found, set CMAKE_PREFIX_PATH = Qt root directory")
ENDIF()
add_definitions(-D_BUILD_WITH_QT)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 COMPONENTS Core Charts REQUIRED)
include_directories(${Qt6Charts_INCLUDES})
set(CMAKE_CXX_FLAGS "${Qt6Charts_EXECUTABLE_COMPILE_FLAGS}")

# Set a default build configuration if none is specified. 'Release' produces the smallest binaries
IF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
ENDIF()

include(CheckCXXCompilerFlag)

IF (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
  IF (NOT APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
  ELSE()
  set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -D__APPLE__")
  ENDIF()
  # Enable C++11 mode on C++ / Clang
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -D__Unix__  -fPIC -g")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__Unix__  -fPIC")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:$ORIGIN")
ENDIF()

  IF (AVX2)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:AVX2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
  ELSE()
    IF (AVX)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:AVX")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX")
    ELSE()
      IF(SSE2)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
      ENDIF()
    ENDIF()
  ENDIF()

  IF(MSVC_DEBUG_INFO_REL)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi ")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /ASSEMBLYDEBUG /DEBUG:FULL")
  ENDIF()

# Compile with compiler warnings turned on
IF(MSVC)
  IF(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  ELSE()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  ENDIF()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DHDF5CPP_USEDLL /D_USE_MATH_DEFINES /DH5_BUILT_AS_DYNAMIC_LIB /D_HDF5USEDLL_ /DBUILDING_DLL /DXDR_HACK /DXDR_LE  /DNOMINMAX /openmp")
ELSEIF ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Waddress -Warray-bounds=1 -Wbool-compare -Wbool-operation -Wc++11-compat -Wc++14-compat -Wcatch-value -Wchar-subscripts -Wcomment -Wduplicate-decl-specifier -Wenum-compare -Wformat -Wimplicit -Wimplicit-int -Wimplicit-function-declaration -Winit-self -Wlogical-not-parentheses -Wmain -Wmaybe-uninitialized -Wmemset-elt-size -Wmemset-transposed-args -Wmisleading-indentation -Wmissing-attributes -Wmissing-braces -Wmultistatement-macros -Wnarrowing -Wnonnull -Wnonnull-compare -Wopenmp-simd -Wparentheses -Wpointer-sign -Wreorder -Wrestrict -Wreturn-type -Wsequence-point -Wsign-compare -Wsizeof-pointer-div -Wsizeof-pointer-memaccess -Wstrict-aliasing -Wstrict-overflow=1 -Wswitch -Wtautological-compare -Wtrigraphs -Wuninitialized -Wunknown-pragmas -Wunused-function -Wunused-label -Wunused-value -Wunused-variable -Wvolatile-register-var")
ENDIF()

#zeromq include
include_directories(${ZeroMQ_INCLUDE_DIR})

#hdf5 include
include_directories(${HDF5_INCLUDE_DIRS})

# Include Eigen 
include_directories(${EIGEN3_INCLUDES})

# Include NetCDF 
include_directories(${NETCDF_INCLUDE_DIRS})

# Include XRF_Maps
include_directories(${XRF_Maps_DIR}/../src)
include_directories(${XRF_Maps_DIR}/../src/support/eigen-git-mirror)

# Include header files
include_directories(src)

file(GLOB_RECURSE PROJ_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable( ${PROJECT_NAME} ${PROJ_SRC} uProbeX.qrc )

set(CompilerFlags
  LIBRARY_OUTPUT_DIRECTORY LIBRARY_OUTPUT_DIRECTORY_RELEASE LIBRARY_OUTPUT_DIRECTORY_DEBUG
  RUNTIME_OUTPUT_DIRECTORY RUNTIME_OUTPUT_DIRECTORY_RELEASE RUNTIME_OUTPUT_DIRECTORY_DEBUG)

foreach(CompilerFlag ${CompilerFlags})
  set_target_properties(uProbeX PROPERTIES ${CompilerFlag} ${PROJECT_SOURCE_DIR}/bin)
endforeach()


#qt_use_modules(uProbeX Core Widgets Gui)
IF (MSVC)
  set_target_properties(uProbeX PROPERTIES COMPILE_FLAGS "/D_WINSOCKAPI_")
  set_target_properties(uProbeX PROPERTIES COMPILE_FLAGS_RELEASE "/O2 /Qvec-report:2 /GL /MP /bigobj /Qansi-alias")
  set_target_properties(uProbeX PROPERTIES LINK_FLAGS "/LTCG")
  IF (BUILD_WITH_OPENCV_SUPPORT)
    target_link_libraries (uProbeX PRIVATE 
                        yaml-cpp::yaml-cpp
                        libzmq-static
            						ws2_32.lib
            						rpcrt4.lib
            						iphlpapi.lib
                        ${TIFF_LIBRARIES}
                        ${Qt6Core_LIBRARIES}
                        ${Qt6Charts_LIBRARIES}
                        opencv_core
                        opencv_imgcodecs
                        opencv_ml
                        opencv_dnn
                        opencv_flann
                        netCDF::netcdf
                        hdf5::hdf5-shared
                        cppzmq cppzmq-static
                        NLopt::nlopt
                        libxrf_fit
						            libxrf_io
                        ${CMAKE_THREAD_LIBS_INIT}
                        ws2_32.lib )
  ELSE()
    target_link_libraries (uProbeX PRIVATE 
                          yaml-cpp::yaml-cpp
                          libzmq-static
                          ws2_32.lib
                          rpcrt4.lib
                          iphlpapi.lib
                          ${TIFF_LIBRARIES}
                          ${Qt6Core_LIBRARIES}
                          ${Qt6Charts_LIBRARIES}
                          netCDF::netcdf
                          hdf5::hdf5-shared
                          cppzmq cppzmq-static
                          NLopt::nlopt
                          libxrf_fit
                          libxrf_io
                          ${CMAKE_THREAD_LIBS_INIT}
                          ws2_32.lib )
  ENDIF() 

ELSEIF (UNIX)
  #IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  #  set_target_properties(uProbeX PROPERTIES MACOSX_BUNDLE TRUE)
  #ENDIF()
  set_target_properties(uProbeX PROPERTIES COMPILE_FLAGS "-O2")
  IF (BUILD_WITH_OPENCV_SUPPORT)
    target_link_libraries (uProbeX PRIVATE
                        libzmq
                        yaml-cpp::yaml-cpp
                        ${TIFF_LIBRARIES}
                        ${Qt6Core_LIBRARIES}
                        ${Qt6Charts_LIBRARIES}
                        opencv_core
                        opencv_imgcodecs
                        opencv_ml
                        opencv_dnn
                        opencv_flann
                        netCDF::netcdf
                        cppzmq cppzmq-static
                        NLopt::nlopt
                        libxrf_fit
                        libxrf_io
                        ${CMAKE_THREAD_LIBS_INIT} )
  ELSE()
    target_link_libraries (uProbeX PRIVATE
                        libzmq
                        yaml-cpp::yaml-cpp
                        ${TIFF_LIBRARIES}
                        ${Qt6Core_LIBRARIES}
                        ${Qt6Charts_LIBRARIES}
                        netCDF::netcdf
                        cppzmq cppzmq-static
                        NLopt::nlopt
                        libxrf_fit
                        libxrf_io
                        ${CMAKE_THREAD_LIBS_INIT} )
  ENDIF()
ENDIF()
