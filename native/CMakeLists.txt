cmake_minimum_required(VERSION 3.16)
project(dustpress_native LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(dustpress_native STATIC
  ../src/AirEQ.cpp
  ../src/CurveBank.cpp
  ../src/EnvelopeFollower.cpp
  ../src/LimiterLookahead.cpp
  ../src/ParamSmoother.cpp
  ../src/SoftSaturation.cpp
  ../src/TiltEQ.cpp
  src/NativeDustPress.cpp
  src/WavFile.cpp
)

target_include_directories(dustpress_native
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_compile_features(dustpress_native PUBLIC cxx_std_17)

add_executable(dustpress_cli src/main.cpp)
target_link_libraries(dustpress_cli PRIVATE dustpress_native)

target_include_directories(dustpress_cli PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

add_executable(dustpress_probe src/probe.cpp)
target_link_libraries(dustpress_probe PRIVATE dustpress_native)
target_include_directories(dustpress_probe PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

