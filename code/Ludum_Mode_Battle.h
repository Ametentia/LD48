#if !defined(LUDUM_MODE_BATTLE_H_)
#define LUDUM_MODE_BATTLE_H_

enum Enemy_Type {
    EnemyType_Standard = 0,
    EnemyType_Boss,
    EnemyType_FinalBoss
};

enum Button {
    Button_Up = 0,
    Button_Down,
    Button_Left,
    Button_Right
};

struct Call {
    u32 beat_count;
    f32 beats[20];
    Button beat_buttons[20];

    u32 success_threshold;
    v2 pos[40];

    b32 visible[20]; // @Todo: This can be a u32 of bits with an IsVisible check for index
    u32 beats_shown;

    u32 hits;

    Sound_Handle call_handle;
    Sound_Handle player_note_handles[4];
    Sound_Handle enemy_note_handles[4];
};

struct CallSet {
    u32 current_call;
    u32 call_count;
    Call *calls;

    u32 total_hits;
    u32 total_notes;
    u32 attempt_count;
};

struct Mode_Battle {
    Asset_Manager *assets;
    Memory_Allocator *alloc;
    Random random;

    Enemy_Type type;

    Animation transition;

    f32 bpm;
    f32 timer;

    u32 beat_wait;
    f32 intro_wait;

    s32 wave;

    u32 points;

    b32 done;

    v2 metronome;

    f32 met_speed;
    f32 met_angle;
    v2 met_flip_range;

    f32 guide_height;

    CallSet calls;
    Game_Button control_map[4];
    s32 *health;

    Image_Handle beat_textures[4];

    Image_Handle enemy_texture;
    Image_Handle enemy_hud;
};
#endif  // LUDUM_MODE_BATTLE_H_
