// We can initialise the play mode here with whatever requires initialisation
//
#include "Ludum_Mode_Battle.cpp"
internal void ModePlay(Game_State *state) {
    Clear(&state->mode_alloc);

    Mode_Play *play = AllocStruct(&state->mode_alloc, Mode_Play);
    play->alloc = &state->mode_alloc;
    play->temp = state->temp;
    play->player_position = V2(0, 0);
    play->map_size = V2(10,12);
    play->tile_arr = AllocArray(play->alloc, Tile, play->map_size.x*play->map_size.y);
    play->tile_size = V2(0.5,0.5);
    play->tile_spacing = 0.01;
    play->player[0] = CreateAnimation(GetImageByName(&state->assets, "forward_walk"),  4, 1, 0.23);
    play->player[1] = CreateAnimation(GetImageByName(&state->assets, "backward_walk"), 4, 1, 0.23);
    play->player[2] = CreateAnimation(GetImageByName(&state->assets, "right_walk"),    2, 1, 0.23);
    play->player[3] = CreateAnimation(GetImageByName(&state->assets, "left_walk"),     2, 1, 0.23);

    play->camera_pos.z = 100;

    play->last_anim = &play->player[0];

    // World gen stuff
    //
    play->world = CreateWorld(play->alloc, &state->assets, V2U(5, 5));

    state->mode = GameMode_Play;
    state->play = play;
}

// This is where most of the game logic will happen
//
internal void UpdateRenderModePlay(Game_State *state, Game_Input *input, Draw_Command_Buffer *draw_buffer) {
    Mode_Play *play = state->play;
    if(play->in_battle) {
        UpdateRenderModeBattle(state, input, draw_buffer, play->battle);
        if(play->battle->done) {
            EndTemp(play->battle_mem);
            play->in_battle = 0;
        }
        return;
    }

    Render_Batch _batch = CreateRenderBatch(draw_buffer, &state->assets,
            V4(0.0, 41.0 / 255.0, 45.0 / 255.0, 1.0));
    Render_Batch *batch = &_batch;

    Game_Controller *controller = GameGetController(input, 1);

    if (!controller->connected) {
        controller = GameGetController(input, 0);
    }

    f32 dt = input->delta_time;

    f32 dx = play->tile_size.x+play->tile_spacing;
    f32 dy = play->tile_size.y+play->tile_spacing;

    if (JustPressed(controller->left)) {
        if (play->player_position.x != 0) {
            play->player_position -= V2(dx, 0);
        }

        play->last_anim = &play->player[3];
    }

    if (JustPressed(controller->right)) {
        if (play->player_position.x + dx < dx * play->map_size.x) {
            play->player_position += V2(dx, 0);
        }

        play->last_anim = &play->player[2];
    }

    if (JustPressed(controller->up)) {
        play->camera_pos.z -= 5;
    }

    if (JustPressed(controller->down)) {
        play->camera_pos.z += 5;
    }

    if (JustPressed(controller->action)) {
        play->battle_mem = BeginTemp(play->alloc);
        play->in_battle = 1;
        play->battle = AllocStruct(play->alloc, Mode_Battle);
        ModeBattle(state, play->battle);
    }

    if (IsPressed(input->mouse_buttons[MouseButton_Left])) {
        play->camera_pos -= V3((Abs(play->camera_pos.z) * 0.8 * input->mouse_delta));
    }

    Animation *anim = play->last_anim;

    SetCameraTransform(batch, 0, V3(1, 0, 0),
            V3(0, 1, 0), V3(0, 0, 1), play->camera_pos);

    // @Todo: Proper player entity
    //
    UpdateAnimation(&play->player[0], dt);
    UpdateAnimation(&play->player[1], dt);
    UpdateAnimation(&play->player[2], dt);
    UpdateAnimation(&play->player[3], dt);

    DrawAnimation(batch, anim, dt, V3(play->player_position), V2(0.6, 0.6));

    // @Todo(James): Make this its own 'DrawWorld' function or something
    //
    v4 tile_colour = V4(196.0f / 255.0f, 240.0f / 255.0f, 194.0f / 255.0f, 1.0);
    World *world = &play->world;
    for (u32 it = 0; it < world->room_count; ++it) {
        Room *room = &world->rooms[it];

        v4 c = V4(1, 0, 1, 1);
        if (room->flags & RoomFlag_IsStart) { c = V4(0, 1, 0, 1); }
        else if (room->flags & RoomFlag_HasExit) { c = V4(1, 0, 0, 1); }

        v2 pos = room->pos * world->tile_size;
        v2 dim = V2(room->dim) * world->tile_size;
        DrawQuadOutline(batch, pos, dim, 0, c, 0.25);

        for (u32 y = 0; y < room->dim.y; ++y) {
            for (u32 x = 0; x < room->dim.x; ++x) {
                u32 index = (y * room->dim.x) + x;
                Tile *tile = &room->tiles[index];

                v2 tile_pos = pos + (0.5 * world->tile_size) +
                    (((-0.5f * V2(room->dim)) + V2(x, y)) * world->tile_size);

                DrawQuad(batch, { 0 }, tile_pos, world->tile_size, 0, tile_colour);
                for (u32 l = 0; l < 3; ++l) {
                    if (IsValid(tile->layers[l])) {
                        f32 angle = 0;
                        // @Note: The top layer is reserved for borders as they need to be rotated in specific
                        // ways
                        //
                        if (l == 2) {
                            angle = GetBorderRotation(tile);
                        }

                        DrawQuad(batch, tile->layers[l], tile_pos, world->tile_size, angle);
                    }
                }
            }
        }

        for (u32 con = 0; con < 4; ++con) {
            Room *connection = room->connections[con];
            if (!connection) { continue; }

            v2 dir = Normalise(connection->pos - room->pos);
            v2 pos = (room->pos + (dir * 0.5 *  V2(room->dim))) * world->tile_size;

            DrawLine(batch, pos, pos + (2 * dir), V4(1, 0, 0, 1), V4(0, 1, 0, 0), 0.15);
        }
    }
}
