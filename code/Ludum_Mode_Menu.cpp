internal b32 Button(Ui_Context *ui, u32 id, v2 pos, Image_Handle image) {
    b32 result = false;

    b32 hot = false;

    if (WasPressed(ui->left_button)) {
        if (id == ui->hot) {
            ui->active = id;
            result = true;
        }
    }
    else {
        Amt_Image *info = GetImageInfo(ui->batch->assets, image);
        v2 dim = V2(info->width, info->height);
        if (dim.w > dim.h) {
            dim.h = dim.h / dim.w;
            dim.w = 1;
        }
        else {
            dim.w = dim.w / dim.h;
            dim.h = 1;
        }

        rect2 bounds;
        bounds.min = pos - (0.5 * dim);
        bounds.max = pos + (0.5 * dim);

        if (Contains(bounds, ui->mouse)) {
            ui->hot = id;
            hot = true;
        }
    }

    v4 c = V4(1, 1, 1, 1);
    if (hot) { c = V4(0.70, 0.70, 0.70, 1.0); }

    DrawQuad(ui->batch, image, pos, 1, 0, c);

    return result;
}

internal void ModeMenu(Game_State *state) {
    Clear(&state->mode_alloc);

    Mode_Menu *menu = AllocStruct(&state->mode_alloc, Mode_Menu);

    Sound_Handle music = GetSoundByName(&state->assets, "main_menu_calm");
    menu->music = PlaySound(state, music, 0.3, PlayingSound_Looped);

    state->mode = GameMode_Menu;
    state->menu = menu;
}

internal void UpdateRenderModeMenu(Game_State *state, Game_Input *input, Draw_Command_Buffer *draw_buffer) {
    Mode_Menu *menu = state->menu;

    v4 clear_colour = V4(0.0, 41.0 / 255.0, 45.0 / 255.0, 1.0);

    Render_Batch _batch = CreateRenderBatch(draw_buffer, &state->assets, clear_colour);
    Render_Batch *batch = &_batch;

    Game_Controller *controller = GameGetController(input, 1);
    if (!controller->connected) { controller = GameGetController(input, 0); }

    SetCameraTransform(batch, 0, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1), V3(0, 0, 3));

    rect3 camera_bounds = GetCameraBounds(&batch->game_tx);
    rect2 screen_bounds = Rect2(camera_bounds);

    Image_Handle background = GetImageByName(&state->assets, "menu_background");
    if (menu->showing_credits) {
        background = GetImageByName(&state->assets, "credits");
    }

    DrawQuad(batch, background, V2(0, 0), screen_bounds.max - screen_bounds.min, 0, V4(1, 1, 1, 1));

    if (menu->showing_credits) {
        if (IsPressed(controller->menu)) {
            menu->showing_credits = false;
        }

        return;
    }


    Image_Handle title = GetImageByName(&state->assets, "menu_title");
    DrawQuad(batch, title, V2(0, 0.8), 2, 0, V4(1, 1, 1, 1));

    Image_Handle text[] = {
        GetImageByName(&state->assets, "menu_start"),
        GetImageByName(&state->assets, "menu_credits"),
        GetImageByName(&state->assets, "menu_exit")
    };

    Ui_Context *ui = &menu->ui;

    ui->batch = batch;
    ui->mouse = V2(Unproject(&batch->game_tx, V2(input->mouse_clip)));
    ui->left_button = input->mouse_buttons[MouseButton_Left];

    if (Button(ui, 1, V2(1.2, -0.50), text[0])) {
        menu->music->flags = 0;
        menu->music->volume = 0;

        PlaySound(state, GetSoundByName(&state->assets, "game_start"), 0.3);

        ModePlay(state);
        return;
    }

    if (Button(ui, 2, V2(0.7, -0.75), text[1])) {
        menu->showing_credits = true;
        PlaySound(state, GetSoundByName(&state->assets, "select_simple"), 0.3);
    }

    if (Button(ui, 3, V2(1.2, -1.05), text[2])) {
        input->requested_quit = true;
    }
}
