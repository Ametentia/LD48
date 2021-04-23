internal void ModeSplash(Game_State *state) {
    Clear(&state->mode_alloc);

    Mode_Splash *splash = AllocStruct(&state->mode_alloc, Mode_Splash);
    splash->alloc = &state->mode_alloc;
    splash->temp  = state->temp;
    splash->steps = 5;
    splash->time = 0;
    splash->fade_start = 2;

    state->mode = GameMode_Splash;
    state->splash = splash;
}

internal void DrawLogoSection(Image_Handle image, Render_Batch *batch, u8 count, u8 x, u8 y, v2 centre, v2 dim, v4 colour) {
    v2 half_dim = 0.5f * dim;
    v2 p0 = centre + V2(-half_dim.x, half_dim.y);
    v2 p1 = centre + -half_dim;
    v2 p2 = centre + V2(half_dim.x, -half_dim.y);
    v2 p3 = centre + half_dim;

    f32 width = 1.0/count;
    f32 height = 1.0/count;
    Texture_Handle texture = GetImageData(batch->assets, image);
    u32 col = ABGRPack(colour);
    DrawQuad(batch, texture,
            V3(p0), V2(width*x, height*y), col,
            V3(p1), V2(width*x, height*(y+1)), col,
            V3(p2), V2(width*(x+1), height*(y+1)), col,
            V3(p3), V2(width*(x+1), height*y), col);
}

internal void DrawLogoSection(Image_Handle image, Render_Batch *batch, u8 count, u8 x, u8 y, v2 centre, v2 dim) {
    DrawLogoSection(image, batch, count, x, y, centre, dim, V4(1,1,1,1));
}

internal void UpdateRenderModeSplash(Game_State *state, Game_Input *input, Draw_Command_Buffer *draw_buffer) {
    Render_Batch _batch = CreateRenderBatch(draw_buffer, &state->assets, V4(0,0,0,1.0));
    Render_Batch *batch = &_batch;
    SetCameraTransform(batch, 0, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1), V3(0, 0, 5));

    Mode_Splash *mode = state->splash;

    Game_Controller *controller = GameGetController(input, 1);
    if (!controller->connected) {
        controller = GameGetController(input, 0);
    }

    f32 dt = input->delta_time;
    mode->time += dt;

    if (IsPressed(controller->interact) || IsPressed(controller->action)) {
        ModePlay(state);
    }
    Image_Handle texture = GetImageByName(&state->assets, "Logo");
    f32 size = 1.1f;
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            if(i==1 && j==1) {
                continue;
            }
            DrawLogoSection(texture, batch, 3, i, j, V2(-size + i * size, size - j*size), V2(size, size));
        }
    }
    if(mode->time > mode->fade_start) {
        f32 diff = mode->time - mode->fade_start;
        DrawLogoSection(
            texture,
            batch,
            3, 1, 1,
            V2(0, 0),
            V2(size, size),
            V4(1,1,1, Min(diff / 1.3, 1))
        );
    }
}
