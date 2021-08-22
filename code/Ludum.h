#if !defined(LUDUM_H_)
#define LUDUM_H_

#include "Ludum_Types.h"
#include "Ludum_Intrinsics.h"
#include "Ludum_Utility.h"

#include "Ludum_Maths.h"

#include "Ludum_Platform.h"
#include "Ludum_Memory.h"

#include "Ludum_Renderer.h"

#include "Ludum_Assets.h"

#include "Ludum_World.h"

#include "Ludum_Mode_Play.h"
#include "Ludum_Mode_Splash.h"
#include "Ludum_Mode_Battle.h"
#include "Ludum_Mode_Menu.h"


enum Game_Mode {
    GameMode_None = 0,
    GameMode_Play,
    GameMode_Splash,
    GameMode_Menu
};

struct Game_State {
    // Anything allocated from this will last for the entire duration of the games runtime
    //
    Memory_Allocator perm;

    // This is cleared every frame and will expand to meet demand. Feel free to use it for anything
    // however, never keep anything allocated from this across frame boundaries
    //
    Memory_Allocator *temp;
    Temporary_Memory  temp_handle;

    // All assets are loaded into this
    //
    Asset_Manager assets;

    f32 master_volume;
    Playing_Sound *playing_sounds;
    Playing_Sound *free_playing_sounds;

    Memory_Allocator mode_alloc;

    Game_Mode mode;
    union {
        Mode_Play *play;
        Mode_Splash *splash;
        Mode_Menu *menu;
    };
};

#endif  // LUDUM_H_
