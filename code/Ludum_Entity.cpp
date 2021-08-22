internal void MoveToRoom(Player *player, v2s target) {
    s32 x, y;
    Room *new_room = 0;

    if (target.x < 0) { // Move left room
        new_room = player->room->connections[0];

        x = new_room->dim.x - 1;
        y = new_room->dim.y / 2;
    }
    else if (target.x >= cast(s32) player->room->dim.x) { // Move right room
        new_room = player->room->connections[1];

        x = 0;
        y = new_room->dim.y / 2;
    }
    else if (target.y < 0) { // Move down room
        new_room = player->room->connections[2];

        x = new_room->dim.x / 2;
        y = new_room->dim.y - 1;
    }
    else if (target.y >= cast(s32) player->room->dim.y) { // Move up room
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

    if (player->first_move) { player->move_delay_timer = -1; }

    if (IsPressed(controller->up)) {
        target_offset.y = 1;
        move_animation = &world->player_animations[1];
    }
    else if (IsPressed(controller->down)) {
        target_offset.y = -1;
        move_animation = &world->player_animations[0];
    }
    else if (IsPressed(controller->left)) {
        target_offset.x = -1;
        move_animation = &world->player_animations[3];
    }
    else if (IsPressed(controller->right)) {
        target_offset.x = 1;
        move_animation = &world->player_animations[2];
    }
    else {
        player->first_move = true;
    }

    if (player->move_delay_timer >= 0) {
        target_offset = V2S(0, 0);
    }
    else {
        player->move_delay_timer = 0.3f;
    }

    v2s target = V2S(player->grid_pos.x + target_offset.x, player->grid_pos.y + target_offset.y);
    if (!IsEqual(target_offset, V2S(0, 0))) {
        player->first_move = false;
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

    UpdateAnimation(player->animation, dt);
}

internal v2 GetPlayerWorldPosition(World *world, Player *player) {
    v2 result;
    if (!IsEqual(player->grid_pos, player->last_pos)) {
        f32 alpha = player->move_timer / 0.30f;
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

internal void GenerateEnemies(World *world) {
    u32 min_enemy_count = world->room_count + (world->room_count / 2);
    u32 max_enemy_count = 6 * world->room_count;

    world->enemy_count = RandomBetween(&world->rng, min_enemy_count, max_enemy_count - 1);
    for (u32 it = 0; it < world->enemy_count; ++it) {
        Enemy *enemy = &world->enemies[it];

        u32 room_index = it / 6;
        Room *room = &world->rooms[room_index];

        room->enemy_count += 1;
        Assert(room->enemy_count <= 6);

        enemy->room  = room;
        enemy->alive = true;

        enemy->move_timer = RandomUnilateral(&world->rng);

        enemy->grid_pos.x = RandomBetween(&world->rng, 0U, enemy->room->dim.x);
        enemy->grid_pos.y = RandomBetween(&world->rng, 0U, enemy->room->dim.y);

        enemy->last_pos = enemy->grid_pos;

        enemy->animation = &world->enemy_animation;
    }
}

internal void UpdateRenderEnemies(Render_Batch *batch, World *world, f32 dt, Room *draw_room) {
    UpdateAnimation(&world->enemy_animation, dt);

    for (u32 it = 0; it < world->enemy_count; ++it) {
        Enemy *enemy = &world->enemies[it];
        if (!enemy->alive) { continue; }

        enemy->move_timer += dt;

        enemy->move_delay_timer -= dt;
        if (enemy->move_delay_timer < 0) {
            v2 dir = RandomDir(&world->rng);
            s32 x = cast(s32) dir.x;
            s32 y = cast(s32) dir.y;

            v2s target_pos;
            target_pos.x = enemy->grid_pos.x + x;
            target_pos.y = enemy->grid_pos.y + y;
            if (IsValid(enemy->room, target_pos.x, target_pos.y)) {
                enemy->last_pos = enemy->grid_pos;
                enemy->grid_pos = target_pos;
                enemy->move_timer = 0;

                Tile *old_tile = GetTileFromRoom(enemy->room, enemy->last_pos.x, enemy->last_pos.y);
                old_tile->flags &= ~TileFlag_HasEnemy;

                Tile *new_tile = GetTileFromRoom(enemy->room, enemy->grid_pos.x, enemy->grid_pos.y);
                new_tile->flags |= TileFlag_HasEnemy;
            }

            enemy->move_delay_timer = 0.5;
        }

        v2 world_pos;
        if (!IsEqual(enemy->grid_pos, enemy->last_pos)) {
            f32 alpha = enemy->move_timer / 0.15f;
            v2 a = RoomGridToWorld(world, enemy->room, enemy->grid_pos);
            v2 b = RoomGridToWorld(world, enemy->room, enemy->last_pos);

            world_pos = Lerp(a, b, alpha);

            if (alpha >= 1) {
                enemy->last_pos = enemy->grid_pos;
                world_pos = RoomGridToWorld(world, enemy->room, enemy->grid_pos);
            }
        }
        else {
            world_pos = RoomGridToWorld(world, enemy->room, enemy->grid_pos);
        }

        if (enemy->room == draw_room) {
            DrawAnimation(batch, enemy->animation, V3(world_pos), world->tile_size);
        }
    }
}
