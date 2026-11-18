#!/bin/sh
# Compiler launcher: -fPIC last, so it wins over CMake's -fPIE for the port's executable
# objects and over projects (Torch) that ignore CMAKE_*_FLAGS. Everything ends up in .so files.
exec "$@" -fPIC
