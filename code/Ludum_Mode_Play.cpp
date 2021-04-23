// We can initialise the play mode here with whatever requires initialisation
//
internal void ModePlay(Game_State *state) {
    Clear(&state->mode_alloc);

    Mode_Play *play = AllocStruct(&state->mode_alloc, Mode_Play);
    play->alloc = &state->mode_alloc;
    play->temp  = state->temp;

    play->player_position = V2(0, 0);

    state->mode = GameMode_Play;
    state->play = play;
}

// This is where most of the game logic will happen
//
internal void UpdateRenderModePlay(Game_State *state, Game_Input *input, Draw_Command_Buffer *draw_buffer) {
    Mode_Play *play = state->play;

    Render_Batch _batch = CreateRenderBatch(draw_buffer, &state->assets, V4(0.25, 0.25, 0.25, 1.0));
    Render_Batch *batch = &_batch;

    SetCameraTransform(batch, 0, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1), V3(0, 0, 5));

    Game_Controller *controller = GameGetController(input, 1);
    if (!controller->connected) {
        controller = GameGetController(input, 0);
    }

    f32 dt = input->delta_time;

    if (IsPressed(controller->left)) {
        play->player_position += dt * V2(-1, 0);
    }

    if (IsPressed(controller->right)) {
        play->player_position += dt * V2(1, 0);
    }

    if (IsPressed(controller->up)) {
        play->player_position += dt * V2(0, 1);
    }

    if (IsPressed(controller->down)) {
        play->player_position += dt * V2(0, -1);
    }

    DrawQuad(batch, { 0 }, play->player_position, V2(0.5, 0.5), 0, V4(1, 0, 1, 1));
}
