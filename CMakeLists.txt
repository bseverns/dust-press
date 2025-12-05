cmake_minimum_required(VERSION 3.16)
project(dustpress LANGUAGES C CXX)

# Keep the CMake entry point lean: the actual sources live under native/.
# This exists so preset-driven builds that start at repo root still find a CMake project.
add_subdirectory(native)
