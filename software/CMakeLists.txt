cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi_scopetemp CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

SET(CMAKE_CXX_FLAGS "-Wall -O0 -g" )
SET(CMAKE_CXX_FLAGS_DEBUG "-Werror" )

Include (CheckCSourceCompiles)
include (CheckIncludeFiles)

find_package(USB10 REQUIRED)
find_package(INDI REQUIRED)

include_directories( ${CMAKE_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${CFITSIO_INCLUDE_DIR})

########### scopetemp ###########
set(indi_scopetemp_SRCS
  ${CMAKE_SOURCE_DIR}/scopetemp.cc
  )

add_executable(indi_scopetemp ${indi_scopetemp_SRCS})

target_link_libraries(indi_scopetemp
  ${INDI_LIBRARIES}
  ${INDI_DRIVER_LIBRARIES}
  ${LIBUSB10_LIBRARIES}
  )

install(TARGETS indi_scopetemp RUNTIME DESTINATION bin )
