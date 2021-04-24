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

    play->images = AllocArray(play->alloc, Image_Handle, 10 * 10);

    Random rng = RandomSeed(time(0));

    Image_Handle handles[] = {
        { 0 },
        GetImageByName(&state->assets, "tile_00"),
        GetImageByName(&state->assets, "tile_01"),
        GetImageByName(&state->assets, "tile_02"),
        GetImageByName(&state->assets, "object_00")
    };

    for (u32 y = 0; y < 10; ++y) {
        for (u32 x = 0; x < 10; ++x) {
            u32 index = (y * 10) + x;
            Image_Handle handle = handles[RandomChoice(&rng, ArrayCount(handles))];

            play->images[index] = handle;
        }
    }

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

    Animation *anim = &play->player[0];

    if (IsPressed(controller->left)) {
        play->player_position.x -= (6 * dt);
        anim = &play->player[3];
    }

    if (IsPressed(controller->right)) {
        play->player_position.x += (6 * dt);
        anim = &play->player[2];
    }

    if (IsPressed(controller->down)) {
        play->player_position.y -= (6 * dt);
    }

    if (IsPressed(controller->up)) {
        play->player_position.y += (6 * dt);
        anim = &play->player[1];
    }

    SetCameraTransform(batch, 0, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1), V3(play->player_position, 6));

    UpdateAnimation(&play->player[0], dt);
    UpdateAnimation(&play->player[1], dt);
    UpdateAnimation(&play->player[2], dt);
    UpdateAnimation(&play->player[3], dt);

    v4 tile_colour = V4(196.0 / 255.0, 240.0 / 255.0, 194.0 / 255.0, 1.0);
    for (u32 y = 0; y < 10; ++y) {
        for (u32 x = 0; x < 10; ++x) {
            u32 index = (y * 10) + x;
            Image_Handle image = play->images[index];

            DrawQuad(batch, { 0 }, V2(0.6 * x, 0.6 * y), V2(0.6, 0.6), 0, tile_colour);
            if (IsValid(image)) {
                DrawQuad(batch, image, V2(0.6 * x, 0.6 * y), V2(0.6, 0.6), 0, V4(1, 1, 1, 1));
            }
        }
    }

    DrawQuad(batch, {0}, V2(0, 0), V2(1, 1), 0, V4(0, 1, 1, 1));
    DrawAnimation(batch, anim, dt, V3(play->player_position), V2(0.6, 0.6));
}
