# Orpheus

This is the GitHub repository for our Ludum Dare 48 submission titled "Orpheus".

The submission page can be found here: https://ldjam.com/events/ludum-dare/48/orpheus

The theme was **Deeper and deeper**.

Relive the ancient fable of Orpheus rescuing his beloved Eurydice from the depths Hades, beat by beat! Delve through layers of Hades battling gorgons, cyclops, hydras and more with your trusty lyre.

The gameplay is akin to 2D roguelike dungeon crawlers with combat being carried out in a call-response minigame. The visuals are styled after old gameboy titles with 8-bit graphics (for the most part) and a restrictive colour palette.

## Building

All build scripts are in the `code` directory and should be executed from within the `code` directory.
All builds will go to the `build` directory which is automatically created on initial build.

### Windows

Dependencies:
* [MSVC++ compiler](https://visualstudio.microsoft.com/downloads/)
* [SDL2](http://libsdl.org/download-2.0.php) (install to `libs/SDL2`)

Build by running `build.bat`

### Linux

Dependencies:
* g++ compiler
* [SDL2](http://libsdl.org/download-2.0.php) (check your package manager)

Build by running `build.sh`

### Switch

The Switch build is for homebrew only.
If you need help setting up the devkitPro SDK a getting started guide can be found [here](https://devkitpro.org/wiki/Getting_Started).

Dependencies:
* devkitA64
* libnx
* switch-mesa
* switch-libdrm_nouveau

Build by running `switch.sh`

## Assets

The asset packer is currently unavailable. Please download one of the releases from the releases page and extract the
`data.amt` from it. Then place this file in `build/data/data.amt` for them to be built into the Switch romfs or to be loaded
at runtime by PC releases.
