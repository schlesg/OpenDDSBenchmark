# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.
#

project(OpenDDS_Messenger1_cmake CXX)
cmake_minimum_required(VERSION 3.8.2)

# set(CMAKE_CXX_STANDARD 11) //Not supported
SET(GCC_COVERAGE_LINK_FLAGS    "-lboost_ program_options -lboost_timer -lboost_chrono -lrt")

find_package(OpenDDS)
find_package(Boost REQUIRED)
FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)
find_package(Boost REQUIRED filesystem program_options timer chrono)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

set(src "${CMAKE_CURRENT_SOURCE_DIR}")
set(dst ${CMAKE_CURRENT_BINARY_DIR})
set(all_targets publisher subscriber)

# Publisher
add_executable(publisher
    ${src}/publisher.cpp
)
OPENDDS_TARGET_SOURCES(publisher
    ${src}/Messenger.idl
)

# Subscriber
add_executable(subscriber
    ${src}/subscriber.cpp
)
OPENDDS_TARGET_SOURCES(subscriber
    ${src}/DataReaderListener.cpp
    ${src}/DataReaderListener.h
    ${src}/Messenger.idl
)

foreach(t ${all_targets})
  target_link_libraries(${t} OpenDDS::OpenDDS ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_LIBRARIES})
endforeach()

# Copy configs/scripts into build-output directory
if (EXISTS ${DDS_ROOT}/tests/DCPS/Messenger)
  file(GLOB ini "${DDS_ROOT}/tests/DCPS/Messenger/*.ini")
  file(GLOB pl "${DDS_ROOT}/tests/DCPS/Messenger/*.pl")
  file(GLOB xml "${DDS_ROOT}/tests/DCPS/Messenger/*.xml")
  file(GLOB p7s "${DDS_ROOT}/tests/DCPS/Messenger/*.p7s")

  add_custom_target(Copy_ini_and_scripts
    ALL
    COMMAND_EXPAND_LISTS
    VERBATIM
    COMMENT "Copying configs/scripts into build-output directory"
    COMMAND ${CMAKE_COMMAND} -E copy ${ini} ${pl} ${xml} ${p7s} ${dst}/$<CONFIG>
  )

  add_dependencies(Copy_ini_and_scripts ${all_targets})
endif()
