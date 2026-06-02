cmake_minimum_required(VERSION 3.16)

set(IDF_TARGET esp32p4)
set(EXTRA_COMPONENT_DIRS "../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(tgx_p4_buddha)
