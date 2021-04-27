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
    player->money    = 100;
    player->carrying = false;
    player->item = {
        0,
        0,
        0
    };
    player->repellant_active = false;
    player->repellant_timer = 0;

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

            if (play->health <= 0) {
                play->end_screen = 2;
            }
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
        if (play->end_screen == 2) {
            background = GetImageByName(&state->assets, "game_over");
        }

        DrawQuad(batch, background, V2(0, 0), screen_bounds.max - screen_bounds.min, 0, V4(1, 1, 1, 1));

        for (u32 it = 0; it < ArrayCount(controller->buttons); ++it) {
            if (JustPressed(controller->buttons[it])) {
                ModeMenu(state);
            }
        }

        return;
    }

    f32 dt = input->delta_time;

    Player *player = &play->player;

     if(player->repellant_active){
        player->repellant_timer-=dt;
        if(player->repellant_timer <= 0){
           player->repellant_active = false;
        }
    }

    if (play->level_state == LevelState_Playing) {
        MovePlayer(controller, world, dt, player);
    }

    v2 player_world_pos = GetPlayerWorldPosition(world, player);

    {
        Tile *player_tile = GetTileFromRoom(player->room, player->grid_pos.x, player->grid_pos.y);
        if (player_tile->flags & TileFlag_HasEnemy && !player->repellant_active) {
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

            for (u32 it = 0; it < world->enemy_count; ++it) {
                Enemy *enemy = &world->enemies[it];
                if (enemy->room != player->room) { continue; }

                if (IsEqual(enemy->grid_pos, player->grid_pos)) { enemy->alive = false; }
            }

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
            player_tile->flags &= ~TileFlag_HasEnemy;
            player_tile->flags &= ~TileFlag_HasBoss;
            world->boss_alive = false;
            play->battle->boss = 0;

            play->battle->boss = 1;
            if (world->layer_number == 4) {
                play->battle->final_boss = 1;
            }

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

    if (JustPressed(controller->interact)) {
        Tile *player_tile = GetTileFromRoom(player->room, player->grid_pos.x, player->grid_pos.y);
        
        umm cost = 10;
        if(player_tile->flags&TileFlag_ShopItem){
            if(player->money >= cost && !player->carrying){
                player->carrying = true;
                player->item = {
                    10,
                    0.2f,
                    player_tile->flags
                };
                player_tile->flags = TileFlag_ShopEmpty;
                player_tile->shop_sprite = {0};
            }
        }
    }
    if (JustPressed(controller->action)) {
        Tile *above_player = GetTileFromRoom(player->room, player->grid_pos.x, player->grid_pos.y+1);
        if(above_player->flags&TileFlag_HasHermes && player->carrying){
            player->money -= player->item.cost;
            if(player->item.flags & (ItemFlag_ExtraString|ItemFlag_StringReinforcement)){
                if(play->health != 4){
                    play->health++;
                }
            }
            if(player->item.flags&ItemFlag_Repellant){
                player->repellant_active = true;
                player->repellant_timer = 30;
            }
            player->item = {
                0,
                0,
                0
            };
            player->carrying = false;
        }
    }

    // Set camera transform
    //
    v3 camera_pos = (play->debug_camera) ? play->debug_camera_pos : V3(player_world_pos, 6);
    SetCameraTransform(batch, 0, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1), camera_pos);

    // Draw active room
    //
    DrawRoom(batch, world, player->room);

    if(player->carrying){
        Tile *tile = GetTileFromRoom(player->room, player->grid_pos.x, player->grid_pos.y);
        if(tile->flags&TileFlag_ShopItem|TileFlag_ShopEmpty){
            DrawQuad(batch, GetImageByName(&state->assets, "pay"), V3(player->room->hermes.pos.x,player->room->hermes.pos.y+0.5,0),V2(1,0.5));
        }
    }

    UpdateAnimation(&world->enemy_animation, dt);
    UpdateAnimation(player->animation, dt);
    if(player->room->flags&RoomFlag_IsShop){
        UpdateAnimation(&player->room->hermes.anim, dt);
    }
    DrawAnimation(batch, player->animation, V3(player_world_pos), world->tile_size);


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

        Image_Handle health[] = {
            GetImageByName(&state->assets, "health_1"),
            GetImageByName(&state->assets, "health_2"),
            GetImageByName(&state->assets, "health_3"),
            GetImageByName(&state->assets, "health_full")
        };

        Image_Handle image = health[play->health - 1];
        DrawQuad(batch, image, pos, 0.4, 0, V4(1, 1, 1, 1));
    }

}
