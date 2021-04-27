#if !defined(LUDUM_MODE_PLAY_H_)
#define LUDUM_MODE_PLAY_H_
#include "Ludum_Mode_Battle.h"

enum Level_State {
    LevelState_Playing = 0,
    LevelState_TransitionOut,
    LevelState_TransitionIn,
    LevelState_Next,
    LevelState_TransitionBattle
};

// @Todo(James): These two structs are the same
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
    Room *room;

    v2s grid_pos;
    v2s last_pos;

    f32 move_timer;
    f32 move_delay_timer;

    Animation *animation;
};

struct Mode_Play {
    Memory_Allocator *alloc;
    Memory_Allocator *temp;

    b32 in_battle;
    Temporary_Memory battle_mem;
    Mode_Battle *battle;

    Playing_Sound *music;

    Level_State level_state;

    Animation transition_in;
    Animation transition_out;
    Animation battle_transition;

    f32 transition_delay;

    Player player;
    World world;

    // @Todo(James): Reuse this for overview map
    //
    b32 debug_camera;
    v3 debug_camera_pos;
};

#endif  // LUDUM_MODE_PLAY_H_
