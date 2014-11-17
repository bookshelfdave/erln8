cmake_minimum_required (VERSION 2.6)
project (erln8)
set (Erln8_VERSION_MAJOR 1)
set (Erln8_VERSION_MINOR 0)
SET(CMAKE_CXX_FLAGS "-std=c++11")

configure_file (
    "${PROJECT_SOURCE_DIR}/version.h.in"
    "${PROJECT_BINARY_DIR}/version.h"
)

SET(Boost_USE_STATIC_LIBS ON)
FIND_PACKAGE( Boost 1.56 COMPONENTS system thread program_options log_setup log filesystem REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

add_executable(erln8 erln8.cpp config.cpp builder.cpp builder.h config.h repo.cpp repo.h)
TARGET_LINK_LIBRARIES( erln8 ${Boost_LIBRARIES} )
