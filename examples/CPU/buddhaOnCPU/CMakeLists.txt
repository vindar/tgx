################################################
# CMakeLists for project: buddhaOnCPU
################################################
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
add_executable("${PROJECT_NAME}" ${project_SRC} ${tgx_SRC})

# override default behaviour and use R/G/B order to match CImg's color channels ordering. 
add_compile_definitions(TGX_RGB32_ORDER_BGR=0)

# set the project as the default startup project in visual studio.
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")

# move CMake specific project inside filter "CMakePredefinedTargets".
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PREDEFINED_TARGETS_FOLDER "CustomTargets")

#end of file

