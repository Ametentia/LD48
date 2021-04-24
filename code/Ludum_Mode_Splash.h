#if !defined(LUDUM_MODE_SPLASH_H_)
#define LUDUM_MODE_SPLASH_H_

enum Move_Type {
    MoveType_None,
    MoveType_Left,
    MoveType_Right,
    MoveType_Up,
    MoveType_Down
};

struct Block {
    v2 pos;
    v2 blockPos;
};

struct Move {
    u8 index;
    Move_Type move;    
};

struct Mode_Splash {
    Memory_Allocator *alloc;
    Memory_Allocator *temp;
    
    u8 steps;
    f32 time;
    f32 fade_start;
    Block blocks[8];
    Move moves[10];
    f32 timer;
};

#endif  // LUDUM_MODE_SPLASH_H_
