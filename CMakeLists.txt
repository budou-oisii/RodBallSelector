cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(RodBallSelector VERSION 1.0.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK. Set GEODE_SDK to your Geode SDK folder.")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})

# Create a versioned package name for GitHub Releases.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/mod.json" ROD_BALL_MOD_JSON)
string(JSON ROD_BALL_MOD_ID GET "${ROD_BALL_MOD_JSON}" "id")
string(JSON ROD_BALL_MOD_VERSION GET "${ROD_BALL_MOD_JSON}" "version")
add_custom_target(${PROJECT_NAME}_VERSIONED_PACKAGE ALL
    DEPENDS ${PROJECT_NAME}_PACKAGE
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/${ROD_BALL_MOD_ID}.geode"
        "${CMAKE_CURRENT_BINARY_DIR}/${ROD_BALL_MOD_ID}_${ROD_BALL_MOD_VERSION}.geode"
    COMMENT "Creating ${ROD_BALL_MOD_ID}_${ROD_BALL_MOD_VERSION}.geode"
    VERBATIM
)
