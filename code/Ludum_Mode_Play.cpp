// We can initialise the play mode here with whatever requires initialisation
//
#include "Ludum_Mode_Battle.cpp"
internal void ModePlay(Game_State *state) {
    Clear(&state->mode_alloc);

    Mode_Play *play = AllocStruct(&state->mode_alloc, Mode_Play);
    play->alloc = &state->mode_alloc;
    play->temp = state->temp;
    Sound_Handle world_music = GetSoundByName(&state->assets, "overworld");
    play->music = PlaySound(state, world_music, 0.2, PlayingSound_Looped);

    // World gen stuff
    //
    play->world = CreateWorld(play->alloc, &state->assets, V2U(5, 5), 1);

    World *world = &play->world;

    Player *player = &play->player;

    u32 min_enemy_count = world->room_count + (world->room_count / 2);
    u32 max_enemy_count = 4 * world->room_count;
    Assert(min_enemy_count < max_enemy_count);

    play->enemy_count = RandomBetween(&world->rng, min_enemy_count, max_enemy_count);
    play->enemies     = AllocArray(play->alloc, Enemy, play->enemy_count);

    play->transition = CreateAnimation(GetImageByName(&state->assets, "transition_spritesheet"), 5, 2, 0.1);

    play->enemy_animation = CreateAnimation(GetImageByName(&state->assets, "enemy"), 2, 1, 0.45);
    for (u32 it = 0; it < play->enemy_count; ++it) {
        Enemy *enemy = &play->enemies[it];

        u32 room_index = RandomBetween(&world->rng, 0U, world->room_count - 1);
        Room *room = &world->rooms[room_index];
        while (room->enemy_count == 4) {
            room_index = RandomBetween(&world->rng, 0U, world->room_count - 1);
            room = &world->rooms[room_index];
        }

        room->enemy_count += 1;
        enemy->room = room;

        enemy->alive = true;

        enemy->move_timer = RandomUnilateral(&world->rng);

        enemy->grid_pos.x = RandomBetween(&world->rng, 0U, enemy->room->dim.x);
        enemy->grid_pos.y = RandomBetween(&world->rng, 0U, enemy->room->dim.y);

        enemy->animation = &play->enemy_animation;
    }


    // Setup walk animations
    //
    player->walk_animations[0] = CreateAnimation(GetImageByName(&state->assets, "forward_walk"),  4, 1, 0.25);
    player->walk_animations[1] = CreateAnimation(GetImageByName(&state->assets, "backward_walk"), 4, 1, 0.25);
    player->walk_animations[2] = CreateAnimation(GetImageByName(&state->assets, "right_walk"),    2, 1, 0.25);
    player->walk_animations[3] = CreateAnimation(GetImageByName(&state->assets, "left_walk"),     2, 1, 0.25);

    // Player
    //
    player->room     = &world->rooms[0];
    player->grid_pos = V2S(player->room->dim.x / 2, player->room->dim.y / 2);
    player->last_pos = player->grid_pos;

    player->animation = &player->walk_animations[0];

    state->mode = GameMode_Play;
    state->play = play;
}

// This is where most of the game logic will happen
//
internal void UpdateRenderModePlay(Game_State *state, Game_Input *input, Draw_Command_Buffer *draw_buffer) {
    Mode_Play *play = state->play;
    if (play->in_battle) {
        UpdateRenderModeBattle(state, input, draw_buffer, play->battle);
        if (play->battle->done) {
            EndTemp(play->battle_mem);
            play->in_battle = 0;
            Sound_Handle world_music = GetSoundByName(&state->assets, "overworld");
            play->music = PlaySound(state, world_music, 0.2, PlayingSound_Looped);
        }

        return;
    }

    v4 clear_colour = V4(0.0, 41.0 / 255.0, 45.0 / 255.0, 1.0);

    Render_Batch _batch = CreateRenderBatch(draw_buffer, &state->assets, clear_colour);
    Render_Batch *batch = &_batch;

    Game_Controller *controller = GameGetController(input, 1);
    if (!controller->connected) { controller = GameGetController(input, 0); }

    f32 dt = input->delta_time;

    World *world   = &play->world;
    Player *player = &play->player;

    if (play->level_state == LevelState_Playing) {
        MovePlayer(controller, world, dt, player);
    }

    v2 player_world_pos = GetPlayerWorldPosition(world, player);

    {
        Tile *player_tile = GetTileFromRoom(player->room, player->grid_pos.x, player->grid_pos.y);
        if (player_tile->flags & TileFlag_HasEnemy) {
            play->battle_mem = BeginTemp(play->alloc);
            play->battle     = AllocStruct(play->alloc, Mode_Battle);
            play->in_battle  = 1;

            ModeBattle(state, play->battle, &play->battle_mem);
            play->music->volume = 0;
            play->music->flags = 0;
            player_tile->flags &= ~TileFlag_HasEnemy;

            for (u32 it = 0; it < play->enemy_count; ++it) {
                Enemy *enemy = &play->enemies[it];

                if (IsEqual(enemy->grid_pos, player->grid_pos)) {
                    enemy->alive = false;
                }
            }
        }
        else {
            Tile *last_tile = GetTileFromRoom(player->room, player->last_pos.x, player->last_pos.y);
            if (last_tile->flags & TileFlag_IsExit) {
                if (world->layer_number == 6) {
                    input->requested_quit = true;
                }
                else {
                    if (play->level_state == LevelState_Next) {
                        play->world =
                            CreateWorld(play->alloc, &state->assets, V2U(5, 5), world->layer_number + 1);

                        player->room = &world->rooms[0];
                        player->grid_pos = V2S(player->room->dim.x / 2, player->room->dim.y / 2);
                        player->last_pos = player->grid_pos;

                        player->animation = &player->walk_animations[0];

                        ResetAnimation(&play->transition);
                        play->transition_delay = 0;

                        play->level_state = LevelState_Playing;
                        return;
                    }
                    else {
                        play->level_state = LevelState_Transition;
                    }
                }
            }
        }
    }


    // @Debug: Debug camera stuff
    //
    if (JustPressed(input->f[2])) {
        play->debug_camera = !play->debug_camera;
        if (play->debug_camera) {
            play->debug_camera_pos = V3(RoomGridToWorld(world, player->room, player->grid_pos), 10);
        }
    }

    if (play->debug_camera) {
        if (IsPressed(input->mouse_buttons[MouseButton_Left])) {
            play->debug_camera_pos -= V3((Abs(play->debug_camera_pos.z) * 0.8 * V2(input->mouse_delta)));
        }

        play->debug_camera_pos.z += input->mouse_delta.z;
        if (play->debug_camera_pos.z < 5) { play->debug_camera_pos.z = 5; }
        else if (play->debug_camera_pos.z > 125) { play->debug_camera_pos.z = 125; }
    }

    // Set camera transform
    //
    v3 camera_pos = (play->debug_camera) ? play->debug_camera_pos : V3(player_world_pos, 6);
    SetCameraTransform(batch, 0, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1), camera_pos);

    // Draw active world
    //
    DrawRoom(batch, world, player->room);

    UpdateAnimation(&play->enemy_animation, dt);
    UpdateAnimation(player->animation, dt);

    DrawAnimation(batch, player->animation, V3(player_world_pos), world->tile_size);

#if 1
    Room *ra = player->room;
    Room *rb = &world->rooms[world->room_count - 1];
    v2 dir = Normalise(rb->pos - ra->pos);
    DrawLine(batch, player_world_pos, player_world_pos + (2 * dir), V4(0, 0, 1, 1), V4(0, 0, 1, 1), 0.05);
#endif

    for (u32 it = 0; it < play->enemy_count; ++it) {
        Enemy *enemy = &play->enemies[it];
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

        if (enemy->room == player->room) {
            DrawAnimation(batch, enemy->animation, V3(world_pos), world->tile_size);
        }
    }

    if (play->level_state == LevelState_Transition) {
        if (!IsFinished(&play->transition)) {
            UpdateAnimation(&play->transition, dt);
        }
        else {
            play->transition_delay += dt;
            if (play->transition_delay >= 0.9) {
                play->level_state = LevelState_Next;
            }
        }

        DrawAnimation(batch, &play->transition, V3(player_world_pos), V2(10, 10));
    }
}
