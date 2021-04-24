// We can initialise the play mode here with whatever requires initialisation
//
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
    state->mode = GameMode_Play;
    state->play = play;
}

// Draw map based on size, tile dimensions and tile spacing
//
internal void DrawMap(Render_Batch *batch, Mode_Play *play, v2 size, v2 tile_dims, f32 spacing){
    for(umm i = 0; i < size.x; i++){
        for(umm j = 0; j < size.y; j++){
            DrawQuad(batch, {0}, V2(i*(tile_dims.x+spacing), j*(tile_dims.x+spacing)), V2(tile_dims.x,tile_dims.y), 0, V4(196/255.0, 240/255, 194/255, 1));
            // DrawQuad(batch, GetImageByName(batch->assets, ), V2(i*(tile_dims.x+spacing), j*(tile_dims.x+spacing)), V2(tile_dims.x,tile_dims.y));
            play->tile_arr[i*(umm)size.x+j].pos = V2(i*(tile_dims.x+spacing), j*(tile_dims.y+spacing));
        }
    }
}

// This is where most of the game logic will happen
//
internal void UpdateRenderModePlay(Game_State *state, Game_Input *input, Draw_Command_Buffer *draw_buffer) {
    Mode_Play *play = state->play;

    Render_Batch _batch = CreateRenderBatch(draw_buffer, &state->assets, V4(0.25, 0.25, 0.25, 1.0));
    Render_Batch *batch = &_batch;
    Game_Controller *controller = GameGetController(input, 1);
    
    if (!controller->connected) {
        controller = GameGetController(input, 0);
    }
    
    f32 dt = input->delta_time;
    f32 dx = play->tile_size.x+play->tile_spacing;
    f32 dy = play->tile_size.y+play->tile_spacing;

    if (JustPressed(controller->left) && (play->player_position.x) != 0) {
        play->player_position -= V2(dx,0);
    }

    if (JustPressed(controller->right) && (play->player_position.x+dx) < dx*play->map_size.x) {
        play->player_position += V2(dx,0);
    }

    if (JustPressed(controller->up) && (play->player_position.y+dy) < dy*play->map_size.y) {
        play->player_position += V2(0,dy);
    }

    if (JustPressed(controller->down) && (play->player_position.y) != 0) {
        play->player_position -= V2(0,dy);
    }

    SetCameraTransform(batch, 0, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1), V3(play->player_position.x,play->player_position.y, 5));
    DrawQuad(batch, { 0 }, play->player_position, V2(0.5, 0.5), 0, V4(1, 0, 1, 1));
    DrawMap(batch, play, play->map_size, play->tile_size, play->tile_spacing);
}
