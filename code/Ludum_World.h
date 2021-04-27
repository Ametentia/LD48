#if !defined(LUDUM_WORLD_H_)
#define LUDUM_WORLD_H_

enum Tile_Flag {
    TileFlag_Occupied       = 0x1,
    TileFlag_IsExit         = 0x2,
    TileFlag_RoomTransition = 0x4,
    TileFlag_HasEnemy       = 0x8,

    TileFlag_LeftEdge   = 0x10,
    TileFlag_RightEdge  = 0x20,
    TileFlag_TopEdge    = 0x40,
    TileFlag_BottomEdge = 0x80,
    TileFlag_EdgeMask   = TileFlag_LeftEdge | TileFlag_RightEdge | TileFlag_BottomEdge | TileFlag_TopEdge,

    TileFlag_Amulet = 0x100,
    TileFlag_StringReinforcement = 0x200,
    TileFlag_Repellant = 0x400,
    TileFlag_ExtraString = 0x800,
    TileFlag_ShopItem = TileFlag_ExtraString|TileFlag_Amulet|TileFlag_StringReinforcement|TileFlag_Repellant,
    TileFlag_HasHermes = 0x1000,
    TileFlag_ShopEmpty = 0x2000
};

enum Room_Flag {
    RoomFlag_HasExit = 0x1,
    RoomFlag_IsStart = 0x2,
    RoomFlag_IsShop  = 0x4
};

struct Tile {
    u32 flags;

    u32 x, y;
    Image_Handle image;
    Image_Handle shop_sprite;
};

struct Room {
    u32 flags;
    u32 enemy_count;

    v2 pos;
    v2u dim;

    Tile *tiles;

    Room *connections[4];
    
    Animation hermes;
};

struct World {
    v2 tile_size;

    Random rng;

    v2u max_room_dim;
    v2u world_dim;
    u32 layer_number;

    u32 room_count;
    Room *rooms;

    Memory_Allocator *alloc;
};

#endif  // LUDUM_WORLD_H_
