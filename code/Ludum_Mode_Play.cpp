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
    play->world = CreateWorld(play->alloc, &state->assets, V2U(5, 5));

    World *world = &play->world;

    Player *player = &play->player;
    play->health = 4;


    // Setup walk animations
    //
    world->player_animations[0] = CreateAnimation(GetImageByName(&state->assets, "forward_walk"),  4, 1, 0.25);
    world->player_animations[1] = CreateAnimation(GetImageByName(&state->assets, "backward_walk"), 4, 1, 0.25);
    world->player_animations[2] = CreateAnimation(GetImageByName(&state->assets, "right_walk"),    2, 1, 0.25);
    world->player_animations[3] = CreateAnimation(GetImageByName(&state->assets, "left_walk"),     2, 1, 0.25);

    world->enemy_animation = CreateAnimation(GetImageByName(&state->assets, "enemy"), 2, 1, 0.45);

    world->brazier = CreateAnimation(GetImageByName(&state->assets, "brazier_spritesheet"), 2, 1, 0.1);
    world->boss_alive = 1;

    // Allocate ememy array
    //
    u32 max_enemy_count = 6 * world->world_dim.x * world->world_dim.y;
    world->enemies      = AllocArray(play->alloc, Enemy, max_enemy_count);

    GenerateEnemies(world);

    Image_Handle transition_in  = GetImageByName(&state->assets, "transition_spritesheet");
    Image_Handle transition_out = GetImageByName(&state->assets, "transition_out_spritesheet");

    play->transition_in     = CreateAnimation(transition_in, 5, 2, 0.1);
    play->transition_out    = CreateAnimation(transition_out, 5, 2, 0.1);
    play->battle_transition = CreateAnimation(transition_in, 5, 2, 0.03);

    play->level_state = LevelState_TransitionOut;


    // Player
    //
    player->room     = &world->rooms[0];
    player->grid_pos = V2S(player->room->dim.x / 2, player->room->dim.y / 2);
    player->last_pos = player->grid_pos;

    player->animation = &world->player_animations[0];

    state->mode = GameMode_Play;
    state->play = play;
}

// This is where most of the game logic will happen
//
internal void UpdateRenderModePlay(Game_State *state, Game_Input *input, Draw_Command_Buffer *draw_buffer) {
    Mode_Play *play = state->play;
    World *world   = &play->world;
    if (play->in_battle) {
        UpdateRenderModeBattle(state, input, draw_buffer, play->battle);
        if (play->battle->done) {
            EndTemp(play->battle_mem);
            if(play->battle->boss) {
                world->boss_alive = 0;
            }
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


    if (play->end_screen) {
        SetCameraTransform(batch, 0, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1), V3(0, 0, 3));

        rect3 camera_bounds = GetCameraBounds(&batch->game_tx);
        rect2 screen_bounds = Rect2(camera_bounds);

        Image_Handle background = GetImageByName(&state->assets, "end_screen");
        DrawQuad(batch, background, V2(0, 0), screen_bounds.max - screen_bounds.min, 0, V4(1, 1, 1, 1));

        for (u32 it = 0; it < ArrayCount(controller->buttons); ++it) {
            if (IsPressed(controller->buttons[it])) {
                ModeMenu(state);
            }
        }

        return;
    }

    f32 dt = input->delta_time;

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

            play->battle->final_boss = 0;
            play->battle->boss = 0;
            ModeBattle(state, play->battle, &play->battle_mem);
            play->battle->health = &play->health;
            play->music->volume = 0;
            play->music->flags = 0;
            player_tile->flags &= ~TileFlag_HasEnemy;

            play->level_state = LevelState_TransitionBattle;
        }
        else if (player_tile->flags & TileFlag_HasBoss && world->boss_alive) {
            play->battle_mem = BeginTemp(play->alloc);
            play->battle     = AllocStruct(play->alloc, Mode_Battle);
            play->in_battle  = 1;
            play->battle->boss = 1;
            if (world->layer_number == 4) {
                play->battle->final_boss = 1;
            }

            ModeBattle(state, play->battle, &play->battle_mem);
            play->battle->enemy = world->layer_number + 3;
            play->battle->health = &play->health;
            play->music->volume = 0;
            play->music->flags = 0;
            player_tile->flags &= ~TileFlag_HasBoss;
            play->level_state = LevelState_TransitionBattle;
        }
        else {
            Tile *last_tile = GetTileFromRoom(player->room, player->last_pos.x, player->last_pos.y);
            if (last_tile->flags & TileFlag_IsExit) {
                if (world->layer_number == 4) {
                    play->end_screen = true;
                }
                else {
                    if (play->level_state == LevelState_Next) {

                        RecreateWorld(world, &state->assets);
                        play->world.boss_alive = 1;

                        ZeroSize(world->enemies, 6 * world->world_dim.x * world->world_dim.y);
                        GenerateEnemies(&play->world);

                        // Reset player
                        //
                        player->room = &world->rooms[0];
                        player->grid_pos = V2S(player->room->dim.x / 2, player->room->dim.y / 2);
                        player->last_pos = player->grid_pos;

                        player->animation = &world->player_animations[0];

                        ResetAnimation(&play->transition_in);
                        play->transition_delay = 0;

                        play->level_state = LevelState_TransitionOut;
                        return;
                    }
                    else {
                        play->level_state = LevelState_TransitionIn;
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

    // Draw active room
    //
    DrawRoom(batch, world, player->room);
    DrawAnimation(batch, player->animation, V3(player_world_pos), world->tile_size);

#if 1
    Room *ra = player->room;
    Room *rb = &world->rooms[world->room_count - 1];
    v2 dir = Normalise(rb->pos - ra->pos);
    DrawLine(batch, player_world_pos, player_world_pos + (2 * dir), V4(0, 0, 1, 1), V4(0, 0, 1, 1), 0.05);
#endif

    UpdateRenderEnemies(batch, world, dt, player->room);

    if (world->boss_alive) {
        v2 boss_world = RoomGridToWorld(world, world->boss_room, world->boss_pos) + 0.5f * world->tile_size;
        DrawQuad(batch, world->boss_image, boss_world, 2 * world->tile_size);

        UpdateAnimation(&world->brazier, dt);

        v2 left_pos  = RoomGridToWorld(world, world->boss_room, V2S(world->boss_pos.x - 1, world->boss_pos.y));
        v2 right_pos = RoomGridToWorld(world, world->boss_room, V2S(world->boss_pos.x + 2, world->boss_pos.y));

        DrawAnimation(batch, &world->brazier, V3(left_pos),  world->tile_size);
        DrawAnimation(batch, &world->brazier, V3(right_pos), world->tile_size);
    }
    else {
        v2 left_pos  = RoomGridToWorld(world, world->boss_room, V2S(world->boss_pos.x - 1, world->boss_pos.y));
        v2 right_pos = RoomGridToWorld(world, world->boss_room, V2S(world->boss_pos.x + 2, world->boss_pos.y));

        Image_Handle brazier = GetImageByName(&state->assets, "brazier_empty");
        DrawQuad(batch, brazier, left_pos, world->tile_size);
        DrawQuad(batch, brazier, right_pos, world->tile_size);
    }

    if (play->level_state == LevelState_TransitionIn) {
        if (!IsFinished(&play->transition_in)) {
            UpdateAnimation(&play->transition_in, dt);
        }
        else {
            play->transition_delay += dt;
            if (play->transition_delay >= 0.9) {
                play->level_state = LevelState_Next;
            }
        }

        DrawAnimation(batch, &play->transition_in, V3(player_world_pos), V2(10, 10));
    }
    else if (play->level_state == LevelState_TransitionOut) {
        if (!IsFinished(&play->transition_out)) {
            UpdateAnimation(&play->transition_out, dt);
        }
        else {
            play->transition_delay += dt;
            if (play->transition_delay >= 0.2) {
                play->level_state = LevelState_Playing;
                ResetAnimation(&play->transition_out);
                return;
            }
        }

        DrawAnimation(batch, &play->transition_out, V3(player_world_pos), V2(10, 10));
    }
    else if (play->level_state == LevelState_TransitionBattle) {
        if (!IsFinished(&play->battle_transition)) {
            UpdateAnimation(&play->battle_transition, dt);
        }
        else {
            play->transition_delay += dt;
            if (play->transition_delay >= 0.4) {
                play->level_state = LevelState_Playing;

                play->battle_mem = BeginTemp(play->alloc);
                play->battle     = AllocStruct(play->alloc, Mode_Battle);
                play->in_battle  = true;

                ModeBattle(state, play->battle, &play->battle_mem);

                play->music->volume = 0;
                play->music->flags  = 0;
                play->battle->health = &play->health;

                Tile *player_tile = GetTileFromRoom(player->room, player->grid_pos.x, player->grid_pos.y);
                player_tile->flags &= ~TileFlag_HasEnemy;

                for (u32 it = 0; it < world->enemy_count; ++it) {
                    Enemy *enemy = &world->enemies[it];

                    if (IsEqual(enemy->grid_pos, player->grid_pos)) {
                        enemy->alive = false;
                    }
                }

                ResetAnimation(&play->transition_out);
                return;
            }
        }

        DrawAnimation(batch, &play->battle_transition, V3(player_world_pos), V2(10, 10));
    }

    if (play->level_state == LevelState_Playing) {
        SetCameraTransform(batch, 0, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1), V3(0, 0, 5));

        rect3 camera_bounds = GetCameraBounds(&batch->game_tx);
        rect2 screen_bounds = { V2(camera_bounds.min), V2(camera_bounds.max) };

        v2 pos = V2(screen_bounds.min.x + 0.2, screen_bounds.max.y - 0.2f) + V2(0.05, -0.05);
        DrawQuad(batch, { 0 }, pos, V2(0.50, 0.50), 0, clear_colour);

        Image_Handle image = GetImageByName(&state->assets, "health_full");
        DrawQuad(batch, image, pos, 0.4, 0, V4(1, 1, 1, 1));
    }

}
