###############################################################################
###############################################################################
#
#                       CMakeList of the TGX library. 
# 
# How to build the library (Windows/Linux/MacOS).
#
# 1. Install CMake 3.10 (or later). 
#
# 2. Open a terminal/shell at the library root folder /tgx and type:
#      mkdir build
#      cd build
#      cmake ..
#
# 3. Build the library using the generated project files.
#    - On Windows: open the Visual Studio solution file "tgx.sln" and build the 
#                  library from within the IDE.  
#    - On Linux/MacOS: use the 'make' command to build the library. 
#
###############################################################################
###############################################################################
cmake_minimum_required(VERSION 3.10)

project(tgx)

file(GLOB_RECURSE _tgx_src_files ./src/*.cpp ./src/*.c)
file(GLOB_RECURSE _tgx_hdr_files ./src/*.hpp ./src/*.h ./src/*.inl)

add_library(tgx STATIC ${_tgx_src_files} ${_tgx_hdr_files})
target_include_directories(tgx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
target_compile_features(tgx PUBLIC cxx_std_17)

# set the project as the default startup project in visual studio.
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")

# move CMake specific project inside filter "CMakePredefinedTargets".
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PREDEFINED_TARGETS_FOLDER "CustomTargets")

# build only for debug and release conf. 
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

###############################################################################
#end of file
###############################################################################


