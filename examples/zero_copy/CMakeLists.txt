# (c) 2023-2024 Copyright, Real-Time Innovations, Inc.  All rights reserved.
#
# RTI grants Licensee a license to use, modify, compile, and create derivative
# works of the Software.  Licensee has the right to distribute object form
# only for use with RTI products.  The Software is provided "as is", with no
# warranty of any type, including any warranty for fitness for any purpose.
# RTI is under no obligation to maintain or support the Software.  RTI shall
# not be liable for any incidental or consequential damages arising out of the
# use or inability to use the software.

cmake_minimum_required(VERSION 3.1)

project(HelloWorld C)
set(SOURCE_EXTENSION_C c)

add_compile_definitions(RTI_CERT)
string(REGEX MATCH "Linux" IS_LINUX ${RTIME_TARGET_NAME})
string(REGEX MATCH "QOS" IS_QOS ${RTIME_TARGET_NAME})
if (IS_LINUX OR IS_QOS)
    add_compile_definitions(_POSIX_C_SOURCE=199309L OSAPI_INCLUDE_POSIX)
endif()

set_property(GLOBAL PROPERTY RULE_MESSAGES OFF)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/objs/${RTIME_TARGET_NAME})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/objs/${RTIME_TARGET_NAME})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/objs/${RTIME_TARGET_NAME})

# the following assumes a target name that is an architecture and OS separated
# by a '-'
string(REGEX MATCH ^[^-]* RTIMEARCH ${RTIME_TARGET_NAME})

set(RTI_LIB_PREFIX "rti_me")
set(RTI_LIB_SUFFIX_PSL "_psl")
set(RTI_LIB_SUFFIX)

if (${CMAKE_BUILD_TYPE} MATCHES "[Rr]elease")
    set(RTI_LIB_SUFFIX "z")
    set(RTI_DEBUG_BUILD FALSE)
    add_definitions(-DNDEBUG)
else()
    set(RTI_LIB_SUFFIX "zd")
    set(RTI_DEBUG_BUILD TRUE)
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

set(CERT_LIB_PIL ${RTI_LIB_PREFIX}${RTI_LIB_SUFFIX})
set(CERT_LIB_PSL ${RTI_LIB_PREFIX}${RTI_LIB_SUFFIX_PSL}${RTI_LIB_SUFFIX})
set(CERT_ZERO_COPY_LIBS ${RTI_LIB_PREFIX}_netiozcopy${RTI_LIB_SUFFIX})

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    $ENV{RTIMEHOME}/include/rti_me/rti_me_psl
    $ENV{RTIMEHOME}/include/rti_me
)
link_directories(
    $ENV{RTIMEHOME}/lib/${RTIMEARCH}
    $ENV{RTIMEHOME}/lib/${RTIME_TARGET_NAME}
)

################################################################################
# HelloWorld_subscriber
################################################################################
add_executable(HelloWorld_subscriber
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorld_subscriber.${SOURCE_EXTENSION_C}
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorldApplication.${SOURCE_EXTENSION_C}
    ${IDL_GEN_C}
    ${IDL_GEN_H}
)

target_link_libraries(
    HelloWorld_subscriber
    ${CERT_LIB_PSL}
    ${PLATFORM_LIBS}
    ${CERT_LIB_PIL}
    ${CERT_ZERO_COPY_LIBS}
    ${CERT_LIB_PSL}
)

################################################################################
# HelloWorld_publisher
################################################################################
add_executable(HelloWorld_publisher
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorld_publisher.${SOURCE_EXTENSION_C}
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorldApplication.${SOURCE_EXTENSION_C}
    ${IDL_GEN_C}
    ${IDL_GEN_H}
)

target_link_libraries(
    HelloWorld_publisher
    ${CERT_LIB_PSL}
    ${PLATFORM_LIBS}
    ${CERT_LIB_PIL}
    ${CERT_ZERO_COPY_LIBS}
    ${CERT_LIB_PSL}
)

