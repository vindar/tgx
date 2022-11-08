###############################################################################
###############################################################################
#
#                    CMakeList for project: buddhaOnCPU 
# 
# Building the example (Windows/Linux/MacOS).
# For Windows/Linuc/MacOS.
#
# Building the example:
#
# 1. Install CMake (version 3.10 later). 
#
# 2. Open a shell/terminal inside the directory that contains this file. 
#
# 3. Install CImg (only for Linux/MacOS)
#    - If on Linux/Debian, run: "sudo apt install cimg-dev"
#    - If on MacOS/homebrew, run: "brew install cimg"
#
# 4. run the following commands:
#       mkdir build
#       cd build
#       cmake ..
#
# 4. This will create the project files into the /build directory which can now 
#    be used to build the example. .
#
#    -> On Windows. Open the Visual Studio solution file "buddhaOnCPU.sln" and 
#       build the example from within the IDE. 
# 
#    -> On Linux/MacOS. Run "make" to build the example. 
###############################################################################
###############################################################################
cmake_minimum_required(VERSION 3.10)

project(buddhaOnCPU)

# release is the default build type
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif ()

# add TGX
include_directories("../../../src/")
file(GLOB tgx_SRC "../../../src/*.cpp") 

# add the project files.
file(GLOB project_SRC "buddha.h" "buddhaOnCPU.cpp")

# build an executable
add_executable(${PROJECT_NAME} ${project_SRC} ${tgx_SRC})

# CImg needs X11 on Linux/MacOS
if (UNIX)
    find_package(X11)
    if (X11_FOUND)
        target_link_libraries(${PROJECT_NAME} PUBLIC ${X11_LIBRARIES})
        target_include_directories(${PROJECT_NAME} PUBLIC ${X11_INCLUDE_DIR})
    else()
        if (APPLE)
            message(FATAL_ERROR "CImg requires X11/XQuartz on MacOS. Please install it with 'brew install cimg' or 'brew cask install xquartz' ...")
        else()
            message(FATAL_ERROR "CImg requires X11 on Linux. Please install it with something like 'sudo apt install cimg-dev' or 'sudo apt install libx11-dev'...")
        endif()
    endif ()
endif ()

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# override default behaviour and use R/G/B order to match CImg's color channels ordering. 
add_compile_definitions(TGX_RGB32_ORDER_BGR=0)

# set the project as the default startup project in visual studio.
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")

# move CMake specific project inside filter "CMakePredefinedTargets".
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PREDEFINED_TARGETS_FOLDER "CustomTargets")

###############################################################################
#end of file
###############################################################################


