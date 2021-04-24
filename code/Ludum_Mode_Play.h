#if !defined(LUDUM_MODE_PLAY_H_)
#define LUDUM_MODE_PLAY_H_

struct Mode_Play {
    Memory_Allocator *alloc;
    Memory_Allocator *temp;

    // Can be changed to whatever we need
    //
    v2 player_position;
    Animation player[4];
    Animation *last_anim;

    u32 world_dim;
    Image_Handle *images;

    f32 move_time;
};

#endif  // LUDUM_MODE_PLAY_H_
