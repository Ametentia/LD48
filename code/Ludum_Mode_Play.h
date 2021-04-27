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

struct Item{
    umm cost;
    f32 steal_chance;
    u32 flags;
};

struct Player {
    Room *room;

    v2s grid_pos;
    v2s last_pos;

    f32 move_timer;
    f32 move_delay_timer;

    umm money;
    Item item;
    bool carrying;

    bool repellant_active;
    f32 repellant_timer;

    Animation  walk_animations[4];
    Animation *animation;
};

struct Mode_Play {
    Memory_Allocator *alloc;
    Memory_Allocator *temp;

    // Can be changed to whatever we need
    //
    v2 player_position;
    umm grid_position;
    Animation *last_anim;

    b32 in_battle;
    Temporary_Memory battle_mem;
    Mode_Battle *battle;

    Playing_Sound *music;

    World world;

    Player player;
    s32 health;
    Level_State level_state;

    Animation transition_in;
    Animation transition_out;
    Animation battle_transition;

    f32 transition_delay;

    b32 end_screen;

    // @Todo(James): Reuse this for overview map
    //
    b32 debug_camera;
    v3 debug_camera_pos;
};

internal void GenerateShop(Game_State *state, Mode_Play *play);
#endif  // LUDUM_MODE_PLAY_H_
