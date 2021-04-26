internal v2 RoomGridToWorld(World *world, Room *room, v2s a) {
    Assert(a.x >= 0 && a.x < room->dim.x);
    Assert(a.y >= 0 && a.y < room->dim.y);

    v2 result;
    result.x = (room->pos.x + ((-0.5 * cast(f32) room->dim.x) + a.x)) * world->tile_size.x;
    result.y = (room->pos.y + ((-0.5 * cast(f32) room->dim.y) + a.y)) * world->tile_size.y;

    return result;
}

internal Tile *GetTileFromRoom(Room *room, u32 x, u32 y) {
    Tile *result = 0;

    u32 index = (y * room->dim.x) + x;
    result = &room->tiles[index];

    return result;
}

internal b32 IsValid(Room *room, u32 x, u32 y) {
    b32 result = (x < room->dim.x) && (y < room->dim.y);
    if (result) {
        Tile *tile = GetTileFromRoom(room, x, y);
        result &= ((tile->flags & TileFlag_Occupied) == 0);
    }

    return result;
}

internal void PlaceExitStructure(Asset_Manager *assets, Room *room, u32 layer, u32 x, u32 y) {
    s32 stairs_index = (y * room->dim.x) + x;

    for (u32 yi = (y - 3); yi < (y + 5); ++yi) {
        for (u32 xi = (x - 3); xi < (x + 5); ++xi) {
            Tile *t = &room->tiles[(yi * room->dim.x) + xi];
            t->image = {};
            t->flags = 0;
        }
    }

    Image_Handle stairs  = (layer == 6) ? GetImageByName(assets, "eurydice") : GetImageByName(assets, "stairs");
    Image_Handle rock    = GetImageByName(assets, "rock");
    Image_Handle brazier = GetImageByName(assets, "brazier_empty");

    s32 offsets[] = {
         cast(s32) room->dim.x,
         cast(s32) room->dim.x + 1,
         -1,
         0,
         2,
         -cast(s32) room->dim.x - 1,
         -cast(s32) room->dim.x + 2,
         -cast(s32) (2 * room->dim.x) - 1,
         -cast(s32) (2 * room->dim.x) + 2
    };

    for (u32 it = 0; it < ArrayCount(offsets); ++it) {
        Tile *tile = &room->tiles[stairs_index + offsets[it]];

        if (it == 3) {
            tile->image = stairs;
            tile->flags = TileFlag_IsExit;
        }
        else if (it == 7 || it == 8) {
            tile->image = brazier;
            tile->flags = TileFlag_Occupied;
        }
        else {
            tile->image = rock;
            tile->flags = TileFlag_Occupied;
        }
    }
}

internal void AddConnections(World *world, Asset_Manager *assets, Room *room) {
    v2 checks[4];
    checks[0] = room->pos - V2(world->max_room_dim.x, 0);
    checks[1] = room->pos + V2(world->max_room_dim.x, 0);
    checks[2] = room->pos - V2(0, world->max_room_dim.y);
    checks[3] = room->pos + V2(0, world->max_room_dim.y);

    Image_Handle door_side   = GetImageByName(assets, "door_side");
    Image_Handle door_bottom = GetImageByName(assets, "door_bottom");

    for (u32 it = 0; it < world->room_count; ++it) {
        Room *check_room = &world->rooms[it];
        if (check_room == room) { continue; }

#if 0
        rect2 room_bounds;
        room_bounds.min = check_room->pos - (0.5f * V2(check_room->dim));
        room_bounds.max = check_room->pos + (0.5f * V2(check_room->dim));
#else
        rect2 room_bounds;
        room_bounds.min = check_room->pos - (0.5f * V2(world->max_room_dim));
        room_bounds.max = check_room->pos + (0.5f * V2(world->max_room_dim));
#endif

        for (u32 c = 0; c < 4; ++c) {
            if (Contains(room_bounds, checks[c])) {
                room->connections[c] = check_room;

                u32 x = 0, y = 0;
                switch (c) {
                    case 0: {
                        x = 0;
                        y = room->dim.y / 2;
                    }
                    break;
                    case 1: {
                        x = room->dim.x - 1;
                        y = room->dim.y / 2;
                    }
                    break;
                    case 2: {
                        x = room->dim.x / 2;
                        y = 0;
                    }
                    break;
                    case 3: {
                        x = room->dim.x / 2;
                        y = room->dim.y - 1;
                    }
                    break;
                }

                u32 index = (y * room->dim.x) + x;
                Tile *tile = &room->tiles[index];

                tile->flags |= TileFlag_RoomTransition;
                if (c < 2) {
                    tile->image = door_side;
                }
                else {
                    tile->image = door_bottom;
                }
            }
        }
    }
}

internal b32 IsCorner(Room *room, u32 x, u32 y) {
    b32 result =
        ((x == 0) && (y == 0)) ||
        ((x == 0) && (y == (room->dim.y - 1))) ||
        ((x == (room->dim.x - 1)) && (y == 0)) ||
        ((x == (room->dim.x - 1)) && (y == (room->dim.y - 1)));

    return result;
}

internal f32 GetBorderRotation(Tile *tile) {
    f32 result = 0;
    if (tile->flags & TileFlag_LeftEdge) {
        if (tile->flags & TileFlag_BottomEdge) {
            result = -PI32 / 2;
        }
        else {
            result = PI32;
        }
    }
    else if (tile->flags & TileFlag_TopEdge) {
        result = PI32 / 2;
    }

    return result;
}

internal f32 GetDoorRotation(Tile *tile) {
    f32 result = 0;
    if (tile->flags & TileFlag_LeftEdge) {
        if (tile->flags & TileFlag_BottomEdge) {
            result = -PI32 / 2;
        }
        else {
            result = PI32;
        }
    }
    else if (tile->flags & TileFlag_TopEdge) {
        result = PI32;
    }

    return result;
}

internal void GenerateRoomLayout(World *world, Asset_Manager *assets, Room *room) {
    room->tiles = AllocArray(world->alloc, Tile, room->dim.x * room->dim.y);

    Image_Handle images[] = {
        GetImageByName(assets, "tile_00"),
        GetImageByName(assets, "tile_01"),
        GetImageByName(assets, "tile_02"),
        GetImageByName(assets, "object_00"),
        GetImageByName(assets, "rock")
    };

    Image_Handle stairs = GetImageByName(assets, "stairs");

    Image_Handle border_corner = GetImageByName(assets, "border_corner");
    Image_Handle border_bottom = GetImageByName(assets, "border_bottom");
    Image_Handle border        = GetImageByName(assets, "border_side");
    Image_Handle border_bottom_corner = GetImageByName(assets, "border_bottom_corner");

    b32 is_exit = room->flags & RoomFlag_HasExit;

    for (u32 y = 0; y < room->dim.y; ++y) {
        for (u32 x = 0; x < room->dim.x; ++x) {
            u32 index = (y * room->dim.x) + x;
            Tile *tile = &room->tiles[index];

            if (x == 0) {
                tile->flags |= TileFlag_LeftEdge;
            }
            else if (x == (room->dim.x - 1)) {
                tile->flags |= TileFlag_RightEdge;
            }

            if (y == 0) {
                tile->flags |= TileFlag_BottomEdge;
            }
            else if (y == (room->dim.y - 1)) {
                tile->flags |= TileFlag_TopEdge;
            }

            u32 ground_cover_count = ArrayCount(images);
            if (tile->flags & TileFlag_EdgeMask) {
                ground_cover_count -= 1;
                if (IsCorner(room, x, y)) {
                    if (tile->flags & TileFlag_BottomEdge) {
                        tile->image = border_bottom_corner;
                    }
                    else {
                        tile->image = border_corner;
                    }
                }
                else {
                    if (tile->flags & TileFlag_BottomEdge) {
                        tile->image = border_bottom;
                    }
                    else {
                        tile->image = border;
                    }
                }
            }

            if (!(tile->flags & TileFlag_EdgeMask)) {
                b32 has_layer = RandomChoice(&world->rng, 10) < 5;

                u32 layer_index = RandomChoice(&world->rng, ground_cover_count);
                Image_Handle layer = images[layer_index];
                if (has_layer) {
                    tile->image = layer;

                    if (layer_index >= ground_cover_count - 2) {
                        tile->flags |= TileFlag_Occupied;
                    }
                }
            }
        }
    }

    if (is_exit) {
        u32 x = RandomBetween(&world->rng, 5, room->dim.x - 5);
        u32 y = RandomBetween(&world->rng, 5, room->dim.y - 5);

        PlaceExitStructure(assets, room, world->layer_number, x, y);
    }
}

internal b32 CanPlaceRoom(World *world, v2 pos, v2u dim) {
    b32 result = true;

    rect2 bounds;
    bounds.min = pos - (0.5f * V2(dim));
    bounds.max = pos + (0.5f * V2(dim));

    for (u32 it = 0; it < world->room_count; ++it) {
        Room *room = &world->rooms[it];

        rect2 room_bounds;
        room_bounds.min = room->pos - (0.5f * V2(room->dim));
        room_bounds.max = room->pos + (0.5f * V2(room->dim));

        if (Overlaps(bounds, room_bounds)) {
            result = false;
            break;
        }
    }

    return result;
}

internal v2 RandomDir(Random *rng) {
    v2 result = V2(0, 0);

    u32 choice = RandomChoice(rng, 4);
    switch (choice) {
        case 0: { result = V2( 1,  0); } break;
        case 1: { result = V2(-1,  0); } break;
        case 2: { result = V2( 0,  1); } break;
        case 3: { result = V2( 0, -1); } break;
    }

    return result;
}

internal World CreateWorld(Memory_Allocator *alloc, Asset_Manager *assets, v2u dim, u32 layer_number) {
    World result = {};

    result.alloc        = alloc;

    result.tile_size    = V2(0.6, 0.6);
    result.max_room_dim = V2U(35, 35);
    result.world_dim    = dim;

    result.layer_number = layer_number;

    u64 seed = (time(0) << 14) ^ 3290394023;
    //result.rng          = RandomSeed(26531548851623);
    result.rng          = RandomSeed(seed);

    u32 max_room_count  = RandomBetween(&result.rng, 7, dim.x * dim.y);
    result.room_count   = 1;
    result.rooms        = AllocArray(alloc, Room, max_room_count);

    Room *first = &result.rooms[0];
    first->flags = RoomFlag_IsStart;

    first->pos   = V2(0, 0);

    first->dim.x = (RandomBetween(&result.rng, 12U, 35U) | 1);
    first->dim.y = (RandomBetween(&result.rng, 12U, 35U) | 1);

    Assert(((first->dim.x % 2) != 0) && ((first->dim.y % 2) != 0));

    v2 grid_pos = V2(0, 0);

    Room *prev = first;
    v2 from_dir = V2(0, 0);
    for (u32 it = 1; it < max_room_count; ++it) {
        Room *room = &result.rooms[it];

        v2 dir, new_pos;
        v2u dim;

        u32 tries = 10;
        b32 placed = true;

        while (true) {
            dir = RandomDir(&result.rng);

            v2 new_grid_pos = grid_pos + dir;
            while (new_grid_pos.x < -(0.5 * result.world_dim.x) ||
                   new_grid_pos.y < -(0.5 * result.world_dim.y) ||
                   new_grid_pos.x >= (0.5 * result.world_dim.x) ||
                   new_grid_pos.y >= (0.5 * result.world_dim.y))
            {
                dir = RandomDir(&result.rng);
                new_grid_pos = grid_pos + dir;
            }

            dim.x = (RandomBetween(&result.rng, 12U, 35U) | 1);
            dim.y = (RandomBetween(&result.rng, 12U, 35U) | 1);

            Assert(((dim.x % 2) != 0) && ((dim.y % 2) != 0));

            //new_pos = prev->pos + ((dir * 0.5 * V2(prev->dim)) + (dir * 0.5 * V2(dim)));
            new_pos = prev->pos + (dir * V2(result.max_room_dim));

            if (CanPlaceRoom(&result, new_pos, dim)) {
                grid_pos = new_grid_pos;
                break;
            }
            else if (tries == 0) {
                placed = false;
                break;
            }

            tries -= 1;
        }

        if (!placed) { break; }

        room->pos = new_pos;
        room->dim = dim;

        result.room_count += 1;

        from_dir = dir;
        prev = room;
    }

    prev->flags |= RoomFlag_HasExit;

    for (u32 it = 0; it < result.room_count; ++it) {
        Room *room = &result.rooms[it];
        GenerateRoomLayout(&result, assets, room);

        AddConnections(&result, assets, room);
    }

    return result;
}

internal void DrawRoom(Render_Batch *batch, World *world, Room *room) {
    v4 tile_colour = V4(196.0f / 255.0f, 240.0f / 255.0f, 194.0f / 255.0f, 1.0);

    v2 pos = room->pos * world->tile_size;
    v2 dim = V2(room->dim) * world->tile_size;

    DrawQuad(batch, { 0 }, pos - (0.5f * world->tile_size), dim, 0, tile_colour);

    for (u32 y = 0; y < room->dim.y; ++y) {
        for (u32 x = 0; x < room->dim.x; ++x) {
            u32 index = (y * room->dim.x) + x;
            Tile *tile = &room->tiles[index];

            v2 tile_pos = pos + (((-0.5f * V2(room->dim)) + V2(x, y)) * world->tile_size);

            f32 angle = 0;
            if (tile->flags & TileFlag_RoomTransition) { angle = GetDoorRotation(tile); }
            else if (tile->flags & TileFlag_EdgeMask) { angle = GetBorderRotation(tile); }

            if (IsValid(tile->image)) {
                DrawQuad(batch, tile->image, tile_pos, world->tile_size, angle);
            }
        }
    }
}

