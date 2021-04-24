internal void ModeBattle(Game_State *state, Mode_Battle *battle) {
    battle->points = 5;
}

internal void UpdateRenderModeBattle(Game_State *state, Game_Input *input, Draw_Command_Buffer *draw_buffer, Mode_Battle *battle) {
    Render_Batch _batch = CreateRenderBatch(draw_buffer, &state->assets,
            V4(0.0, 41.0 / 255.0, 45.0 / 255.0, 1.0));
    Render_Batch *batch = &_batch;

    Game_Controller *controller = GameGetController(input, 1);
    if (!controller->connected) {
        controller = GameGetController(input, 0);
    }

    if (JustPressed(controller->action)) {
        battle->done = 1;
    }
}
