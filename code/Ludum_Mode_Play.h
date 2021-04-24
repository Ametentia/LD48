#if !defined(LUDUM_MODE_PLAY_H_)
#define LUDUM_MODE_PLAY_H_

struct Mode_Play {
    Memory_Allocator *alloc;
    Memory_Allocator *temp;

    // Can be changed to whatever we need
    //
    v2 player_position;
    Animation anim;
};

#endif  // LUDUM_MODE_PLAY_H_
