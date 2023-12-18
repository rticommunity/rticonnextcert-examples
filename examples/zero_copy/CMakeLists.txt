# (c) 2023 Copyright, Real-Time Innovations, Inc.  All rights reserved.
#
# RTI grants Licensee a license to use, modify, compile, and create derivative
# works of the Software.  Licensee has the right to distribute object form
# only for use with RTI products.  The Software is provided "as is", with no
# warranty of any type, including any warranty for fitness for any purpose.
# RTI is under no obligation to maintain or support the Software.  RTI shall
# not be liable for any incidental or consequential damages arising out of the
# use or inability to use the software.

cmake_minimum_required(VERSION 3.1)

PROJECT(HelloWorld C)
set(SOURCE_EXTENSION_C c)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    SET(PLATFORM_LIBS pthread rt)
    add_definitions(-DRTI_UNIX -DRTI_LINUX)
endif()

set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/objs/${RTIME_TARGET_NAME})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/objs/${RTIME_TARGET_NAME})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/objs/${RTIME_TARGET_NAME})

set(RTI_LIB_PREFIX rti_me)
set(RTI_LIB_SUFFIX)

if (${CMAKE_BUILD_TYPE} MATCHES "[Rr]elease")
    set(RTI_LIB_SUFFIX "z")
    set(RTI_DEBUG_BUILD FALSE)
    add_definitions(-DNDEBUG)
else()
    message(FATAL_ERROR "Connext Cert only supports the 'Release' build type")
endif()

set(IDL_GEN_C
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorld.${SOURCE_EXTENSION_C}
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorldPlugin.${SOURCE_EXTENSION_C}
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorldSupport.${SOURCE_EXTENSION_C}
)
set(IDL_GEN_H
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorld.h
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorldPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorldSupport.h
)

SET(CERT_C_LIBS ${RTI_LIB_PREFIX}${RTI_LIB_SUFFIX})
SET(CERT_ZERO_COPY_LIBS ${RTI_LIB_PREFIX}_netiozcopy${RTI_LIB_SUFFIX})
ADD_DEFINITIONS(-DRTI_CERT)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    $ENV{RTIMEHOME}/include/rti_me
)
link_directories($ENV{RTIMEHOME}/lib/${RTIME_TARGET_NAME})

################################################################################
# HelloWorld_subscriber
################################################################################
add_executable(HelloWorld_subscriber 
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorld_subscriber.${SOURCE_EXTENSION_C}
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorldApplication.${SOURCE_EXTENSION_C}
    ${IDL_GEN_C}
    ${IDL_GEN_H}
)

target_link_libraries(HelloWorld_subscriber ${CERT_ZERO_COPY_LIBS} ${CERT_C_LIBS} ${PLATFORM_LIBS})

################################################################################
# HelloWorld_publisher
################################################################################
add_executable(HelloWorld_publisher
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorld_publisher.${SOURCE_EXTENSION_C}
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorldApplication.${SOURCE_EXTENSION_C}
    ${IDL_GEN_C}
    ${IDL_GEN_H}
)

target_link_libraries(HelloWorld_publisher ${CERT_ZERO_COPY_LIBS} ${CERT_C_LIBS} ${PLATFORM_LIBS})


