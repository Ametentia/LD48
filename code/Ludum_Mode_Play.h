#if !defined(LUDUM_MODE_PLAY_H_)
#define LUDUM_MODE_PLAY_H_
#include "Ludum_Mode_Battle.h"

enum Level_State {
    LevelState_Playing = 0,
    LevelState_Transition,
    LevelState_Next
};

// @Todo(James): These two structs are basically the same
//
struct Enemy {
    Room *room;

    v2s grid_pos;
    v2s last_pos;

    f32 move_timer;
    f32 move_delay_timer;

    Animation *animation;

    b32 alive;
};

struct Player {
    v2s grid_pos;
    v2s last_pos;

    f32 move_timer;
    f32 move_delay_timer;

    Room *room;

    Animation  walk_animations[4];
    Animation *animation;
};

struct Mode_Play {
    Memory_Allocator *alloc;
    Memory_Allocator *temp;

    u8 in_battle;
    Temporary_Memory battle_mem;
    Mode_Battle *battle;

    Playing_Sound *music;
    Level_State level_state;
    Animation transition;
    f32 transition_delay;

    World world;

    Player player;
    s32 health;

    Animation enemy_animation;

    u32 enemy_count;
    Enemy *enemies;

    b32 debug_camera;
    v3 debug_camera_pos;
};

#endif  // LUDUM_MODE_PLAY_H_
