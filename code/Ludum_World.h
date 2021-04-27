#if !defined(LUDUM_WORLD_H_)
#define LUDUM_WORLD_H_

enum Tile_Flag {
    TileFlag_Occupied       = 0x1,
    TileFlag_IsExit         = 0x2,
    TileFlag_RoomTransition = 0x4,
    TileFlag_HasEnemy       = 0x8,
    TileFlag_HasBoss        = 0x10,

    TileFlag_LeftEdge   = 0x20,
    TileFlag_RightEdge  = 0x40,
    TileFlag_TopEdge    = 0x80,
    TileFlag_BottomEdge = 0x100,
    TileFlag_EdgeMask   = TileFlag_LeftEdge | TileFlag_RightEdge | TileFlag_BottomEdge | TileFlag_TopEdge,

    TileFlag_ShopEmpty = 0x100,
    TileFlag_StringReinforcement = 0x200,
    TileFlag_Repellant = 0x400,
    TileFlag_ExtraString = 0x800,
    TileFlag_ShopItem = TileFlag_ExtraString|TileFlag_StringReinforcement|TileFlag_Repellant,
    TileFlag_HasHermes = 0x1000
};

enum Room_Flag {
    RoomFlag_HasExit = 0x1,
    RoomFlag_IsStart = 0x2,
    RoomFlag_IsShop  = 0x4
};

enum Item_Flag{
    ItemFlag_StringReinforcement = 0x200,
    ItemFlag_Repellant = 0x400,
    ItemFlag_ExtraString = 0x800
};

struct Tile {
    u32 flags;

    u32 x, y;
    Image_Handle image;
    Image_Handle shop_sprite;
};

struct Hermes{
    Animation anim;
    v2 pos;
};

struct Room {
    u32 flags;
    u32 enemy_count;

    v2 pos;
    v2u dim;

    Tile *tiles;

    Room *connections[4];
    Hermes hermes;
};

struct Enemy;
struct World {
    Memory_Allocator *alloc;

    v2 tile_size;

    Random rng;

    v2u max_room_dim;
    v2u world_dim;
    u32 layer_number;

    u32 room_count;
    Room *rooms;

    u32 enemy_count;
    Enemy *enemies;

    Image_Handle boss_image;
    Room *boss_room;
    v2s boss_pos;
    b32 boss_alive;

    // Not sure if these should go here but whatever
    //
    Animation player_animations[4];
    Animation enemy_animation;
    Animation brazier;
};

#endif  // LUDUM_WORLD_H_
