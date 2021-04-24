#if !defined(LUDUM_MODE_PLAY_H_)
#define LUDUM_MODE_PLAY_H_
#include "Ludum_Mode_Battle.h"

struct Tile {
    v2 pos;
    Image_Handle texture;
};
struct Mode_Play {
    Memory_Allocator *alloc;
    Memory_Allocator *temp;

    // Can be changed to whatever we need
    //
    v2 player_position;
    Image_Handle *images;

    u8 in_battle;
    Temporary_Memory battle_mem;
    Mode_Battle *battle;

    v2 map_size;
    Tile *tile_arr;
    v2 tile_size;
    f32 tile_spacing;
    Animation player[4];
};

#endif  // LUDUM_MODE_PLAY_H_
