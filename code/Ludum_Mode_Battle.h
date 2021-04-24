#if !defined(LUDUM_MODE_BATTLE_H_)
#define LUDUM_MODE_BATTLE_H_
struct Mode_Battle {
    Memory_Allocator *alloc;
    Memory_Allocator *temp;
    Random random;

    u32 points;
    u8 health;
    u8 done;
};
#endif  // LUDUM_MODE_BATTLE_H_
