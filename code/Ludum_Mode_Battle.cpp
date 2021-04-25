internal f32 BpmToBps(f32 bpm) {
    return bpm/60;
}

internal void ModeBattle(Game_State *state, Mode_Battle *battle, Temporary_Memory *alloc) {
    battle->alloc = alloc->alloc;
    battle->points = 0;
    battle->metronome = V2(-2.9, -0.2);
    battle->met_angle = 180;
    battle->met_flip_range = V2(-0.3, 0.3);
    battle->guide_height = 2.3;
    battle->dance_angle = 1;
    battle->bpm = 98.0;
    battle->intro_wait = 2;
    battle->wave = -1;
    battle->beat_wait = 8;
    battle->met_speed = 6.8/8*BpmToBps(battle->bpm);
    Sound_Handle intro = GetSoundByName(&state->assets, "intro");
    PlaySound(state, intro, 0.3, 0);

    battle->calls = {
        {
            {
                {0,0.5,1,1.5,2},
                {Button_Up, Button_Down,Button_Up, Button_Down, Button_Up},
                5,
                4,
                {},
                {},
                0
            }
        },
        0
    };
    
    battle->beat_textures = AllocArray(battle->alloc, Image_Handle, 4);
    battle->beat_textures[0] = GetImageByName(&state->assets, "up");
    battle->beat_textures[1] = GetImageByName(&state->assets, "down");
    battle->beat_textures[2] = GetImageByName(&state->assets, "left");
    battle->beat_textures[3] = GetImageByName(&state->assets, "right");
}

internal f32 BeatCount(f32 timer, f32 bpm) {
    return timer*BpmToBps(bpm);
}

internal s8 beatCheck(f32 bpm, Call *call, f32 timer){
    for(s8 i = 0; i < call->beat_count; i++) {
        f32 acc = BeatCount(timer, bpm) - call->beats[i];
        if(acc < 0.1 && acc >= 0) {
            return i;
        }
    }
    return -1;
}

internal s8 playerCheck(f32 bpm, Call *call, f32 timer){
    for(s8 i = 0; i < call->beat_count; i++) {
        f32 acc = BeatCount(timer, bpm) - (call->beats[i])-4.0;
        if(acc < 0.20 && acc >= 0) {
            return i+call->beat_count;
        }
    }
    return -1;
}

internal void UpdateRenderModeBattle(Game_State *state, Game_Input *input, Draw_Command_Buffer *draw_buffer, Mode_Battle *battle) {
    Render_Batch _batch = CreateRenderBatch(draw_buffer, &state->assets,
            V4(0.0, 41.0 / 255.0, 45.0 / 255.0, 1.0));
    Render_Batch *batch = &_batch;
    SetCameraTransform(batch, 0, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1), V3(0, 0, 6));

    Game_Controller *controller = GameGetController(input, 1);
    if (!controller->connected) {
        controller = GameGetController(input, 0);
    }
    f32 dt = input->delta_time;
    battle->dance_timer += dt;
    battle->timer += dt;
    if(battle->dance_timer > BpmToBps(battle->bpm)) {
        battle->dance_timer = 0;
        battle->dance_angle *= -1;
    }
    if(battle->metronome.x > battle->met_flip_range.x && battle->metronome.x < battle->met_flip_range.y) {
        f32 pos = battle->metronome.x - battle->met_flip_range.x;
        f32 alpha = pos/(battle->met_flip_range.y - battle->met_flip_range.x);
        battle->met_angle = Lerp(0, -180, alpha);
        battle->guide_height = Lerp(-2.3, 2.3, alpha);
    }
    else if(battle->metronome.x > 3.9) {
        battle->metronome.x = -2.9;
        battle->met_angle = 180;
        battle->guide_height = 2.3;
        battle->calls.calls[battle->calls.current_call].beat_shown = 0;
        for(u8 i = 0; i < 20; i++) { 
            battle->calls.calls[battle->calls.current_call].visable[i] = 0;
        }
    }
    else if(battle->metronome.x > battle->met_flip_range.y) {
        battle->met_angle = 0;
    }
    if(BeatCount(battle->timer, battle->bpm) > battle->beat_wait) {
        battle->timer = 0;
        battle->wave += 1;
        battle->beat_wait = 8;
        Sound_Handle wave = GetSoundByName(&state->assets, "0_call");
        PlaySound(state, wave, 0.3, 0);
    }
    if(battle->wave>-1) {
        battle->metronome.x += battle->met_speed*dt; 
    }

    Image_Handle harp = GetImageByName(&state->assets, "player_strings");
    Image_Handle enemyBox = GetImageByName(&state->assets, "enemy_strings");
    Image_Handle metronome = GetImageByName(&state->assets, "arrow");
    Image_Handle sprite = GetImageByName(&state->assets, "player_backsprite");
    Image_Handle spotlight = GetImageByName(&state->assets, "spotlight");
    Image_Handle skele = GetImageByName(&state->assets, "skeleton");
    Image_Handle bar = GetImageByName(&state->assets, "bar_border");
    // TODO @Matt come on jesus look at this line, It doesn't even work
    DrawQuad(batch, spotlight, V2(1.8, 0.3), 2.5, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, sprite, V2(-2, -1.7), 2.5, Sin(Radians(Lerp(-15*battle->dance_angle, 15*battle->dance_angle, battle->dance_timer/BpmToBps(battle->bpm)))), V4(1, 1, 1, 1));
    DrawQuad(batch, harp, V2(1.45, -1.5), 4, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, skele, V2(1.8, 1.1), 2.125, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, enemyBox, V2(-2, 1.1), 3.3, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, bar, V2(0, 2.55), 4, 0, V4(1, 1, 1, 1));
    v2 lineHeight = V2(battle->metronome.x, battle->metronome.y+battle->guide_height);
    DrawLine(batch, battle->metronome, lineHeight, V4(1,1,1,1));
    DrawQuad(batch, metronome, battle->metronome, 0.4, Radians(battle->met_angle), V4(1, 1, 1, 1));
    CallSet *callSet = &(battle->calls);
    s8 cpu_beat = beatCheck(battle->bpm, &(callSet->calls[callSet->current_call]), battle->timer);
    s8 player_beat = playerCheck(battle->bpm, &(callSet->calls[callSet->current_call]), battle->timer);
    if(cpu_beat!=-1 && battle->wave > -1) {
        if(cpu_beat+1 > callSet->calls[callSet->current_call].beat_shown){
            callSet->calls[callSet->current_call].beat_shown = cpu_beat+1;
            Button button = callSet->calls[callSet->current_call].beatButtons[cpu_beat];
            callSet->calls[callSet->current_call].pos[cpu_beat] = V2(battle->metronome.x, 1.4 - 0.23 * button);
        }
    }
    if(player_beat!=-1 && battle->wave > -1) {
        s8 beat_count = callSet->calls[callSet->current_call].beat_count;
        if(player_beat+1 > callSet->calls[callSet->current_call].beat_shown){
            callSet->calls[callSet->current_call].beat_shown = player_beat+1;
            Button button = callSet->calls[callSet->current_call].beatButtons[player_beat%beat_count];
            callSet->calls[callSet->current_call].pos[player_beat] = V2(battle->metronome.x, -1.1 - 0.29*button);
        }
        Button button = callSet->calls[callSet->current_call].beatButtons[player_beat%beat_count];
        Game_Button controller_button;
        switch(button){
            case Button_Up: {
                controller_button = controller->up;
            }
            break;
            case Button_Left: {
                controller_button = controller->left;
            }
            break;
            case Button_Right: {
                controller_button = controller->right;
            }
            break;
            case Button_Down: {
                controller_button = controller->down;
            }
            break;
        }
        if(JustPressed(controller_button)) {
            callSet->calls[callSet->current_call].visable[player_beat%beat_count] = 1;
            printf("hit\n");
        }
    }
    if (JustPressed(controller->up)) {
        Sound_Handle intro = GetSoundByName(&state->assets, "0X_harp_d");
        PlaySound(state, intro, 0.3, 0);
    }
    else if (JustPressed(controller->down)) {
        Sound_Handle intro = GetSoundByName(&state->assets, "0X_harp_c");
        PlaySound(state, intro, 0.3, 0);
    }
    else if (JustPressed(controller->right)) {
        Sound_Handle intro = GetSoundByName(&state->assets, "e3");
        PlaySound(state, intro, 0.3, 0);
    }
    else if (JustPressed(controller->left)) {
        Sound_Handle intro = GetSoundByName(&state->assets, "f3");
        PlaySound(state, intro, 0.3, 0);
    }
    for(s8 i = 0; i < callSet->calls[callSet->current_call].beat_shown; i++) {
        s8 beat_count = callSet->calls[callSet->current_call].beat_count;
        Image_Handle beat = battle->beat_textures[callSet->calls[callSet->current_call].beatButtons[i%beat_count]];
        if(beat_count > i || callSet->calls[callSet->current_call].visable[i%beat_count]) {
            DrawQuad(batch, beat, callSet->calls[callSet->current_call].pos[i], 0.4, 0, V4(1, 1, 1, 1));
        }
    }



    if (JustPressed(controller->action)) {
        battle->done = 1;
    }
}
