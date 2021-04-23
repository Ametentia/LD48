#if !defined(LUDUM_MODE_SPLASH_H_)
#define LUDUM_MODE_SPLASH_H_

struct Mode_Splash {
    Memory_Allocator *alloc;
    Memory_Allocator *temp;
    
    u8 steps;
    f32 time;
    f32 fade_start;
};

#endif  // LUDUM_MODE_SPLASH_H_
