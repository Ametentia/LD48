#include "Ludum_Mode_Battle_Calls.cpp"
internal f32 BpmToBps(f32 bpm) {
    return bpm/60;
}

internal void ModeBattle(Game_State *state, Mode_Battle *battle, Temporary_Memory *alloc) {
    battle->alloc = alloc->alloc;
    battle->random = RandomSeed(time(NULL));
    battle->calls = GetCall(&battle->random, battle->final_boss, battle->boss);
    battle->points = 0;
    battle->metronome = V2(-2.8, -0.2);
    battle->met_angle = 180;
    battle->met_flip_range = V2(-0.3, 0);
    battle->guide_height = 2;
    battle->dance_angle = 1;
    u8 bpms[2] = {
        87, 94
    };
    battle->bpm = bpms[battle->calls.song];
    battle->intro_wait = 2;
    battle->wave = -1;
    battle->beat_wait = 8;
    battle->enemy = NextRandom(&battle->random) % 4;
    f32 speed = 6.2;
    if(battle->final_boss){
        speed = 5.6;
    }
    battle->met_speed = speed/8*BpmToBps(battle->bpm);
    char intros[2][8] = {
        "intro_2", "intro_p"
    };
    Sound_Handle intro = GetSoundByName(&state->assets, intros[battle->calls.song]);
    PlaySound(state, intro, 0.3, 0);
    battle->transition = CreateAnimation(GetImageByName(&state->assets, "transition_spritesheet"), 5, 2, 0.23);

    
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
        if(acc < 0.300 && acc >= -0.300 && call->visable) {
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
        battle->guide_height = Lerp(-2.3, 2, alpha);
    }
    else if(battle->metronome.x > battle->met_flip_range.y) {
        battle->met_angle = 0;
    }
    if(BeatCount(battle->timer, battle->bpm) >= battle->beat_wait) {
        battle->timer = 0;
        battle->wave += 1;
        battle->beat_wait = 8;
        battle->metronome.x = -2.8;
        battle->met_angle = 180;
        battle->guide_height = 2;
        Call call = battle->calls.calls[battle->calls.current_call];
        if(battle->wave>0) {
            if(call.hits >= call.successThreshold ) {
                battle->calls.current_call += 1;
                battle->calls.attempt_count += call.beat_count;
                u8 all_notes = battle->calls.hits >= battle->calls.total_notes;
                u8 all_calls = battle->calls.current_call > battle->calls.call_count;
                if(all_notes || all_calls){
                    char exits[3][9] = {
                        "finish_2", "finish_p"
                    };
                    Sound_Handle finale = GetSoundByName(&state->assets, exits[battle->calls.song]);
                    PlaySound(state, finale, 0.3, 0);

                    battle->done = 1;
                    return;
                }
            } else {
                *battle->health -= 1;
            }
        }
        call = battle->calls.calls[battle->calls.current_call];
        battle->calls.calls[battle->calls.current_call].hits = 0;
        battle->calls.calls[battle->calls.current_call].beat_shown = 0;
        Sound_Handle wave = GetSoundByName(&state->assets, call.call_asset_name);
        PlaySound(state, wave, 0.3, 0);
        for(u8 i = 0; i < 20; i++) { 
            battle->calls.calls[battle->calls.current_call].visable[i] = 0;
        }
    }
    if(battle->wave>-1) {
        battle->metronome.x += battle->met_speed*dt; 
    }
    f32 bar_progress = Min(cast(f32) battle->calls.hits/cast(f32)Max(battle->calls.total_notes, battle->calls.attempt_count), 1);

    const char *enemies[4] = {
        "cyclops", 
        "harpie", 
        "gorgon", 
        "skeleton" 
    };

    Image_Handle harp = GetImageByName(&state->assets, "player_strings");
    Image_Handle enemyBox = GetImageByName(&state->assets, "enemy_strings");
    Image_Handle metronome = GetImageByName(&state->assets, "arrow");
    Image_Handle sprite = GetImageByName(&state->assets, "player_backsprite");
    Image_Handle spotlight = GetImageByName(&state->assets, "spotlight");
    Image_Handle skele = GetImageByName(&state->assets, enemies[battle->enemy]);
    Image_Handle bar = GetImageByName(&state->assets, "phoenix_admiration_bar");
    Image_Handle bar_fill = GetImageByName(&state->assets, "bar_fill");
    Image_Handle string_hurt = GetImageByName(&state->assets, "cross");
    DrawQuad(batch, spotlight, V2(1.8, 0.3), 2.5, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, sprite, V2(-2, -1.7), 2.5, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, harp, V2(1.6, -1.5), 4, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, skele, V2(1.8, 1.1), 2.6 /*2.125*/, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, enemyBox, V2(-1.8, 1.1), 3.3, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, bar_fill, V2(Lerp(-1.6, -5.4, bar_progress), 2.35), 4.1, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, bar, V2(-1.6, 2.35), 4, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, {0}, V2(-4.1, 2.36), V2(1,1.3), 0, V4(0, 41.0 / 255.0, 45.0 / 255.0, 1.0));
    DrawQuad(batch, {0}, V2(-3.83, 2.5), V2(0.5,0.05), 0, V4(0, 41.0 / 255.0, 45.0 / 255.0, 1.0));
    for(u8 i = 0; i < 4-*battle->health; i++) {
        f32 height = -1.1 - 0.29*i;
        DrawQuad(batch, string_hurt, V2(0, height), 0.2, 0, V4(1,1,1,1));
    }
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
            if(!battle->final_boss) {
                Sound_Handle intro = GetSoundByName(&state->assets, callSet->calls[callSet->current_call].note_enemy_names[button]);
                PlaySound(state, intro, 0.3, 0);
            }
        }
    }
    u8 note_hit = 0;
    if(player_beat!=-1 && battle->wave > -1) {
        s8 beat_count = callSet->calls[callSet->current_call].beat_count;
        if(player_beat+1 > callSet->calls[callSet->current_call].beat_shown){
            callSet->calls[callSet->current_call].beat_shown = player_beat+1;
            Button button = callSet->calls[callSet->current_call].beatButtons[player_beat%beat_count];
        }
        Button button = callSet->calls[callSet->current_call].beatButtons[player_beat%beat_count];
        Game_Button controller_button = controller->buttons[button];
        if(JustPressed(controller_button) && !callSet->calls[callSet->current_call].visable[player_beat%beat_count]) {
            callSet->calls[callSet->current_call].visable[player_beat%beat_count] = 1;
            callSet->calls[callSet->current_call].hits += 1;
            callSet->hits += 1;
            Sound_Handle intro = GetSoundByName(&state->assets, callSet->calls[callSet->current_call].note_asset_names[button]);
            PlaySound(state, intro, 0.3, 0);
            note_hit = 1;
            callSet->calls[callSet->current_call].pos[player_beat] = V2(battle->metronome.x, -1.1 - 0.29*button);
        }
    }
    for(s8 i = 0; i < callSet->calls[callSet->current_call].beat_shown; i++) {
        s8 beat_count = callSet->calls[callSet->current_call].beat_count;
        Image_Handle beat = battle->beat_textures[callSet->calls[callSet->current_call].beatButtons[i%beat_count]];
        if(beat_count > i || callSet->calls[callSet->current_call].visable[i%beat_count]) {
            DrawQuad(batch, beat, callSet->calls[callSet->current_call].pos[i], 0.4, 0, V4(1, 1, 1, 1));
        }
    }
    if(!note_hit) {
        for(u8 i = 0; i < 4; i++) {
            if(JustPressed(controller->buttons[i])) {
                char sounds[3][6] = {
                    "bad_1",
                    "bad_2",
                    "bad_3"
                };
                Sound_Handle intro = GetSoundByName(&state->assets, sounds[NextRandom(&battle->random) % 2]);
                PlaySound(state, intro, 0.3, 0);
                s8 hits = battle->calls.calls[battle->calls.current_call].hits;
                battle->calls.calls[battle->calls.current_call].hits = Max(0, hits);
            }
        }
    }

    if (JustPressed(controller->action)) {
        battle->done = 1;
    }
}
