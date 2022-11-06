################################################
# CMakeLists for project: buddhaOnCPU
################################################
cmake_minimum_required(VERSION 3.10)

project(buddhaOnCPU)

# release is the default build type
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif ()

# add TGX...
include_directories("../../../src/")
file(GLOB tgx_SRC "../../../src/*.cpp") 

# ...and the project files...
file(GLOB project_SRC "buddha.h" "buddhaOnCPU.cpp")

# to the build. 
add_executable("${PROJECT_NAME}" ${project_SRC} ${tgx_SRC})

# set the project as the default startup project in visual studio.
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")

# move CMake specific project inside filter "CMakePredefinedTargets".
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PREDEFINED_TARGETS_FOLDER "CustomTargets")

#end of file

