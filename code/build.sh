#!/bin/bash

if [[ ! -d "build" ]];
then
    mkdir "build"
fi

pushd "build" 2> /dev/null > /dev/null

COMPILER_FLAGS="-O0 -g -ggdb -DLUDUM_DEBUG=1"
LINKER_FLAGS="-lSDL2 -ldl"

# OpenGL renderer .so
#
g++ $COMPILER_FLAGS -shared -fPIC "../code/Linux_Ludum_OpenGL.cpp" -o "Ludum_OpenGL.so" $LINKER_FLAGS -lGL

# Main game build
#
g++ $COMPILER_FLAGS "../code/Linux_Ludum.cpp" -o "Ludum.bin" $LINKER_FLAGS

popd 2> /dev/null > /dev/null
