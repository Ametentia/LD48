#if !defined(LUDUM_MODE_PLAY_H_)
#define LUDUM_MODE_PLAY_H_
#include "Ludum_Mode_Battle.h"

struct Mode_Play {
    Memory_Allocator *alloc;
    Memory_Allocator *temp;

    v3 camera_pos;

    // Can be changed to whatever we need
    //
    v2 player_position;
    Animation player[4];
    Animation *last_anim;

    u8 in_battle;
    Temporary_Memory battle_mem;
    Mode_Battle *battle;

    World world;

    v2 map_size;
    Tile *tile_arr;
    v2 tile_size;
    f32 tile_spacing;
};

#endif  // LUDUM_MODE_PLAY_H_
