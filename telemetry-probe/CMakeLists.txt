cmake_minimum_required(VERSION 3.21)
project(AutonomousTruckerTelemetryProbe LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "The current capture plugin targets Windows builds of ATS and ETS2.")
endif()

if(NOT DEFINED SCS_SDK_INCLUDE_DIR OR NOT EXISTS "${SCS_SDK_INCLUDE_DIR}/scssdk_telemetry.h")
    message(FATAL_ERROR "Set SCS_SDK_INCLUDE_DIR to the SCS Telemetry SDK include directory.")
endif()

add_library(autonomous_trucker_probe SHARED
    src/telemetry_probe.cpp
)

target_include_directories(autonomous_trucker_probe PRIVATE
    "${SCS_SDK_INCLUDE_DIR}"
)

target_compile_features(autonomous_trucker_probe PRIVATE cxx_std_17)
target_compile_definitions(autonomous_trucker_probe PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
set_target_properties(autonomous_trucker_probe PROPERTIES
    OUTPUT_NAME "autonomous_trucker_probe"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)
