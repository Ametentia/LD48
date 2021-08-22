#!/usr/bin/env bash

# @Note: This should just be '/opt/devkitpro'
# Get the devkit path and put the switch tools into the PATH so we can compile
DevkitPath="$(echo $DEVKITPRO | sed -e 's/^\([a-zA-Z]\):/\/\1/')"
PATH="$PATH:$DevkitPath/tools/bin:$DevkitPath/devkitA64/bin"

# Architecture information
Arch="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE"

# Compiler flags
CompilerFlags="-g -ggdb -Wall -O2 -ffunction-sections $Arch -D__SWITCH__ -fno-rtti -fno-exceptions -Wno-switch -Wno-unused-function"
Includes="-isystem $DevkitPath/libnx/include -isystem $DevkitPath/portlibs/switch/include"

# Linker flags
LinkerFlags="-specs=$DevkitPath/libnx/switch.specs -g $Arch -Wl,-Map,Switch_Test.map"
Libs="-L$DevkitPath/libnx/lib -L$DevkitPath/portlibs/switch/lib -lSDL2 -lEGL -lGLESv2 -lglapi -ldrm_nouveau -lnx -lm"

pushd "../build"

# OpenGL Renderer
aarch64-none-elf-g++ $CompilerFlags $Includes -c ../code/Switch_Ludum_OpenGL.cpp -o Switch_Ludum_OpenGL.o

# Main game code
aarch64-none-elf-g++ $CompilerFlags $Includes -c ../code/Switch_Ludum.cpp -o Switch_Ludum.o

# Linking the objects together
aarch64-none-elf-g++ $LinkerFlags Switch_Ludum.o Switch_Ludum_OpenGL.o $Libs -o Switch_Ludum.elf

# Create the required .nacp and finally output the .nro file which can be executed by the switch
nacptool --create "Orpheus" "Ametentia" "1.2.0" "Orpheus.nacp"
elf2nro Switch_Ludum.elf Orpheus.nro --nacp=Orpheus.nacp --romfsdir=./romfs --icon="./icon_256.jpg"

#--icon="$DevkitPath/libnx/default_icon.jpg"

popd
