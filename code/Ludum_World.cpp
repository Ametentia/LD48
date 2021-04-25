internal void GetAdjacentRooms(World *world, Room *room) {
    v2 checks[4];
    checks[0] = room->pos - V2(world->max_room_dim.x, 0);
    checks[1] = room->pos + V2(world->max_room_dim.x, 0);
    checks[2] = room->pos - V2(0, world->max_room_dim.y);
    checks[3] = room->pos + V2(0, world->max_room_dim.y);

    for (u32 it = 0; it < world->room_count; ++it) {
        Room *check_room = &world->rooms[it];
        if (check_room == room) { continue; }

        rect2 room_bounds;
        room_bounds.min = check_room->pos - (0.5f * V2(check_room->dim));
        room_bounds.max = check_room->pos + (0.5f * V2(check_room->dim));

        for (u32 c = 0; c < 4; ++c) {
            if (Contains(room_bounds, checks[c])) {
                room->connections[c] = check_room;
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

internal void GenerateRoomLayout(World *world, Asset_Manager *assets, Room *room) {
    room->tiles = AllocArray(world->alloc, Tile, room->dim.x * room->dim.y);

    Image_Handle images[] = {
        GetImageByName(assets, "tile_00"),
        GetImageByName(assets, "tile_01"),
        GetImageByName(assets, "tile_02"),
        GetImageByName(assets, "object_00")
    };

    Image_Handle stairs = GetImageByName(assets, "stairs");

    Image_Handle border_corner = GetImageByName(assets, "border_corner");
    Image_Handle border_bottom = GetImageByName(assets, "border_bottom");
    Image_Handle border        = GetImageByName(assets, "border_side");

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
                    tile->layers[2] = border_corner;
                }
                else {
                    if (tile->flags & TileFlag_BottomEdge) {
                        tile->layers[2] = border_bottom;
                    }
                    else {
                        tile->layers[2] = border;
                    }
                }
            }

            b32 has_layer = RandomChoice(&world->rng, 10) < 5;
            Image_Handle layer = images[RandomChoice(&world->rng, ground_cover_count)];
            if (has_layer) {
                tile->layers[0] = layer;
            }
        }
    }

    if (is_exit) {
        u32 x = RandomBetween(&world->rng, 1, room->dim.x - 1);
        u32 y = RandomBetween(&world->rng, 1, room->dim.y - 1);

        u32 index = (y * room->dim.x) + x;
        room->tiles[index].layers[0] = stairs;
        room->tiles[index].flags |= TileFlag_IsExit;
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

internal World CreateWorld(Memory_Allocator *alloc, Asset_Manager *assets, v2u dim) {
    World result = {};

    result.alloc        = alloc;

    result.tile_size    = V2(0.6, 0.6);
    result.max_room_dim = V2U(35, 35);
    result.world_dim    = dim;

    result.rng          = RandomSeed((time(0) << 14) ^ 3290394023);

    u32 max_room_count  = RandomBetween(&result.rng, 7, dim.x * dim.y);
    result.room_count   = 1;
    result.rooms        = AllocArray(alloc, Room, max_room_count);

    Room *first = &result.rooms[0];
    first->flags = RoomFlag_IsStart;

    first->pos   = V2(0, 0);

    first->dim.x = RandomBetween(&result.rng, 12U, 35U);
    first->dim.y = RandomBetween(&result.rng, 12U, 35U);

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
            while (new_grid_pos.x < -(0.5 * result.world_dim.x) || new_grid_pos.y < -(0.5 * result.world_dim.y) ||
                   new_grid_pos.x >= (0.5 * result.world_dim.x) ||
                   new_grid_pos.y >= (0.5 * result.world_dim.y))
            {
                dir = RandomDir(&result.rng);
                new_grid_pos = grid_pos + dir;
            }

            dim.x = RandomBetween(&result.rng, 12U, 35U);
            dim.y = RandomBetween(&result.rng, 12U, 35U);

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
        GetAdjacentRooms(&result, room);

        GenerateRoomLayout(&result, assets, room);
    }

    return result;
}
