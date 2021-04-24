// We can initialise the play mode here with whatever requires initialisation
//
internal void ModePlay(Game_State *state) {
    Clear(&state->mode_alloc);

    Mode_Play *play = AllocStruct(&state->mode_alloc, Mode_Play);
    play->alloc = &state->mode_alloc;
    play->temp  = state->temp;

    play->player_position = V2(0, 0);
    play->player[0] = CreateAnimation(GetImageByName(&state->assets, "forward_walk"),  4, 1, 0.23);
    play->player[1] = CreateAnimation(GetImageByName(&state->assets, "backward_walk"), 4, 1, 0.23);
    play->player[2] = CreateAnimation(GetImageByName(&state->assets, "right_walk"),    2, 1, 0.23);
    play->player[3] = CreateAnimation(GetImageByName(&state->assets, "left_walk"),     2, 1, 0.23);

    u32 world_dim = 50;
    play->world_dim = world_dim;

    play->images = AllocArray(play->alloc, Image_Handle, world_dim * world_dim);

    Random rng = RandomSeed(time(0));

    Image_Handle handles[] = {
        { 0 },
        GetImageByName(&state->assets, "tile_00"),
        GetImageByName(&state->assets, "tile_01"),
        GetImageByName(&state->assets, "tile_02"),
        GetImageByName(&state->assets, "object_00"),
        GetImageByName(&state->assets, "object_01")
    };

    for (u32 y = 0; y < world_dim; ++y) {
        for (u32 x = 0; x < world_dim; ++x) {
            u32 index = (y * world_dim) + x;

            u32 count = ArrayCount(handles);
            if (y == 0 || x == 0 || y == (world_dim - 1) || x == (world_dim - 1)) { count -= 2; }

            Image_Handle handle = handles[RandomChoice(&rng, count)];

            play->images[index] = handle;
        }
    }

    play->last_anim = &play->player[0];

    state->mode = GameMode_Play;
    state->play = play;
}

// This is where most of the game logic will happen
//
internal void UpdateRenderModePlay(Game_State *state, Game_Input *input, Draw_Command_Buffer *draw_buffer) {
    Mode_Play *play = state->play;

    Render_Batch _batch = CreateRenderBatch(draw_buffer, &state->assets,
            V4(0.0, 41.0 / 255.0, 45.0 / 255.0, 1.0));
    Render_Batch *batch = &_batch;

    Game_Controller *controller = GameGetController(input, 1);
    if (!controller->connected) {
        controller = GameGetController(input, 0);
    }

    f32 dt = input->delta_time;

    play->move_time -= dt;
    if (play->move_time < 0) {
        if (IsPressed(controller->left)) {
            play->player_position.x -= 1;
            play->last_anim = &play->player[3];
            play->move_time = 0.15;
        }

        if (IsPressed(controller->right)) {
            play->player_position.x += 1;
            play->last_anim = &play->player[2];
            play->move_time = 0.15;
        }

        if (IsPressed(controller->down)) {
            play->player_position.y -= 1;
            play->last_anim = &play->player[0];
            play->move_time = 0.15;
        }

        if (IsPressed(controller->up)) {
            play->player_position.y += 1;
            play->last_anim = &play->player[1];
            play->move_time = 0.15;
        }
    }

    Animation *anim = play->last_anim;

    v2 player_pos = play->player_position * V2(0.6, 0.6);

    SetCameraTransform(batch, 0, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1), V3(player_pos, 6));

    UpdateAnimation(&play->player[0], dt);
    UpdateAnimation(&play->player[1], dt);
    UpdateAnimation(&play->player[2], dt);
    UpdateAnimation(&play->player[3], dt);

    v4 tile_colour = V4(196.0 / 255.0, 240.0 / 255.0, 194.0 / 255.0, 1.0);
    for (u32 y = 0; y < play->world_dim; ++y) {
        for (u32 x = 0; x < play->world_dim; ++x) {
            u32 index = (y * play->world_dim) + x;
            Image_Handle image = play->images[index];

            v2 size = V2(0.6, 0.6);
            v2 pos  = size * V2(x, y);

            DrawQuad(batch, { 0 }, pos, size, 0, tile_colour);
            if (IsValid(image)) { DrawQuad(batch, image, pos, 0.6, 0); }

            f32 border_scale = 1.17;
            if (x == 0) {
                Image_Handle border = { 0 };
                f32 angle = 0;

                if (y == 0) {
                    border = GetImageByName(&state->assets, "border_corner");
                    angle  = -PI32 / 2;
                }
                else if (y == (play->world_dim - 1)) {
                    border = GetImageByName(&state->assets, "border_corner");
                    angle  = PI32;
                }
                else {
                    border = GetImageByName(&state->assets, "border_side");
                    angle  = PI32;
                }

                DrawQuad(batch, border, pos, border_scale * size, angle);
            }
            else if (x == (play->world_dim - 1)) {
                Image_Handle border;
                f32 angle = 0;

                if (y == 0) {
                    border = GetImageByName(&state->assets, "border_corner");
                }
                else if (y == (play->world_dim - 1)) {
                    border = GetImageByName(&state->assets, "border_corner");
                    angle = PI32 / 2;
                }
                else {
                    border = GetImageByName(&state->assets, "border_side");
                }

                DrawQuad(batch, border, pos, border_scale * size, angle);
            }
            else if (y == 0) {
                Image_Handle border = GetImageByName(&state->assets, "border_side");
                DrawQuad(batch, border, pos, border_scale * size, -PI32 / 2);
            }
            else if (y == (play->world_dim - 1)) {
                Image_Handle border = GetImageByName(&state->assets, "border_side");
                DrawQuad(batch, border, pos, border_scale * size, PI32 / 2);
            }
        }
    }

    DrawAnimation(batch, anim, dt, V3(player_pos), V2(0.6, 0.6));
}
