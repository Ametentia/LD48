internal void MoveToRoom(Player *player, v2s target) {
    s32 x, y;
    Room *new_room = 0;

    if (target.x < 0) { // Move left room
        new_room = player->room->connections[0];

        x = new_room->dim.x - 1;
        y = new_room->dim.y / 2;
    }
    else if (target.x >= player->room->dim.x) { // Move right room
        new_room = player->room->connections[1];

        x = 0;
        y = new_room->dim.y / 2;
    }
    else if (target.y < 0) { // Move down room
        new_room = player->room->connections[2];

        x = new_room->dim.x / 2;
        y = new_room->dim.y - 1;
    }
    else if (target.y >= player->room->dim.y) { // Move up room
        new_room = player->room->connections[3];

        x = new_room->dim.x / 2;
        y = 0;
    }

    if (!new_room) { return; }

    player->grid_pos = V2S(x, y);
    player->last_pos = player->grid_pos;
    player->room     = new_room;
}

internal void MovePlayer(Game_Controller *controller, World *world, f32 dt, Player *player) {
    Animation *move_animation = 0;
    v2s target_offset = V2S(0, 0);

    player->move_delay_timer -= dt;
    if (player->move_delay_timer < 0) {
        if (IsPressed(controller->up)) {
            target_offset.y = 1;
            move_animation = &player->walk_animations[1];
        }
        else if (IsPressed(controller->down)) {
            target_offset.y = -1;
            move_animation = &player->walk_animations[0];
        }
        else if (IsPressed(controller->left)) {
            target_offset.x = -1;
            move_animation = &player->walk_animations[3];
        }
        else if (IsPressed(controller->right)) {
            target_offset.x = 1;
            move_animation = &player->walk_animations[2];
        }

        player->move_delay_timer = 0.15f;
    }

    v2s target = V2S(player->grid_pos.x + target_offset.x, player->grid_pos.y + target_offset.y);
    if (!IsEqual(target_offset, V2S(0, 0))) {
        if (IsValid(player->room, target.x, target.y)) {
            player->last_pos = player->grid_pos;
            player->move_timer = 0;

            player->grid_pos = target;
        }
        else {
            Tile *current_tile = GetTileFromRoom(player->room, player->grid_pos.x, player->grid_pos.y);
            if (current_tile->flags & TileFlag_RoomTransition) {
                MoveToRoom(player, target);
            }
        }
    }

    if (move_animation) { player->animation = move_animation; }

    player->move_timer += dt;
}

internal v2 GetPlayerWorldPosition(World *world, Player *player) {
    v2 result;
    if (!IsEqual(player->grid_pos, player->last_pos)) {
        f32 alpha = player->move_timer / 0.15f;
        v2 a = RoomGridToWorld(world, player->room, player->grid_pos);
        v2 b = RoomGridToWorld(world, player->room, player->last_pos);

        result = Lerp(a, b, alpha);

        if (alpha >= 1) {
            player->last_pos = player->grid_pos;
            result = RoomGridToWorld(world, player->room, player->grid_pos);
        }
    }
    else {
        result = RoomGridToWorld(world, player->room, player->grid_pos);
    }

    return result;
}
