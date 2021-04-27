#if !defined(LUDUM_MODE_BATTLE_H_)
#define LUDUM_MODE_BATTLE_H_

enum Button {
    Button_Up = 0,
    Button_Down,
    Button_Left,
    Button_Right
};

struct Call {
    f32 beats[20];
    Button beatButtons[20];
    s8 beat_count;
    u8 successThreshold;
    v2 pos[40];
    u8 visable[20];
    s8 beat_shown;
    s8 hits;
    const char *call_asset_name;
    const char *note_asset_names[4];
    const char *note_enemy_names[4];
};

struct CallSet {
    Call calls[10];
    u32 current_call;
    u32 hits;
    u32 total_notes;
    u32 attempt_count;
    u8 song;
    u32 call_count;
};

struct Mode_Battle {
    Memory_Allocator *alloc;
    Random random;
    Animation transition;
    f32 bpm;
    f32 intro_wait;
    f32 timer;
    s8 wave;
    u64 beat_wait;

    u32 points;
    u8 done;
    v2 metronome;
    f32 met_angle;
    v2 met_flip_range;
    f32 met_speed;
    f32 guide_height;
    f32 dance_timer;
    s8 dance_angle;
    CallSet calls;
    Image_Handle *beat_textures;
    Game_Button control_map[4];
    s32 *health;
    u8 boss;
    u8 final_boss;
    u8 enemy;
};
#endif  // LUDUM_MODE_BATTLE_H_
