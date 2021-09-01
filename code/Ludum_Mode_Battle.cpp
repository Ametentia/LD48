#include "Ludum_Mode_Battle_Calls.cpp"

internal Mode_Battle *ModeBattle(Game_State *state, u32 layer_index, Enemy_Type type) {
    Mode_Battle *result = AllocStruct(&state->mode_alloc, Mode_Battle);

    result->assets = &state->assets;
    result->alloc  = &state->mode_alloc;
    result->random = RandomSeed(time(0) ^ 8954898774);

    Image_Handle transition = GetImageByName(&state->assets, "transition_spritesheet");
    result->transition      = CreateAnimation(transition, 5, 2, 0.23);

    result->type = type;

    result->intro_wait =  2;
    result->beat_wait  =  8;
    result->wave       = -1;

    // @Note(James): I assumed 'battle->calls.song' was 0 as it was set to 0 and then never changed so
    // I have changed it so the values that would've been if 'song' was always 0
    // :Song
    //

    // Get or generate call set
    //
    result->calls = GetCallSet(result, &result->random, type);

    // Guide line setup
    //
    result->bpm = (type == EnemyType_FinalBoss) ? 94   : 87;
    f32 speed   = (type == EnemyType_FinalBoss) ? 5.6f : 6.2f;

    result->metronome      = V2(-2.8, -0.2);
    result->met_angle      = 180;
    result->met_flip_range = V2(-0.3, 0);
    result->guide_height   = 2;
    result->met_speed      = speed / 8.0f * (result->bpm / 60.0f);

    Sound_Handle intro = GetSoundByName(&state->assets, (type == EnemyType_FinalBoss) ? "intro_p" : "intro_2");
    PlaySound(state, intro, 0.3, 0);

    result->beat_textures[0] = GetImageByName(&state->assets, "up");
    result->beat_textures[1] = GetImageByName(&state->assets, "down");
    result->beat_textures[2] = GetImageByName(&state->assets, "left");
    result->beat_textures[3] = GetImageByName(&state->assets, "right");

    const char *enemy_texture_names[] = {
        "cyclops", "harpie", "gorgon", "skeleton",
        "hydra", "phoenix_01", "cronus", "hades_sprite"
    };

    const char *enemy_bar_texture_names[] = {
        "cyclops_bar", "harpy_bar", "gorgon_bar", "skeleton_bar",
        "hydra_bar", "phoenix_admiration_bar", "cronus_bar", "hades_bar"
    };

    Assert(ArrayCount(enemy_texture_names) == ArrayCount(enemy_bar_texture_names));

    u32 enemy_index = 0;
    if (type == EnemyType_Standard) {
        enemy_index = RandomChoice(&result->random, 4);
    }
    else {
        enemy_index = layer_index + 3;
    }

    result->debug_markers = 0;

    result->enemy_texture = GetImageByName(&state->assets, enemy_texture_names[enemy_index]);
    result->enemy_hud     = GetImageByName(&state->assets, enemy_bar_texture_names[enemy_index]);

    return result;
}

internal f32 BeatCount(f32 timer, f32 bpm) {
    f32 result = timer * (bpm / 60.0f);
    return result;
}

internal s32 BeatCheck(f32 bpm, Call *call, f32 timer){
    s32 result = -1;

    for (s32 i = 0; i < cast(s32) call->beat_count; i++) {
        f32 acc = BeatCount(timer, bpm) - call->beats[i];
        if (acc < 0.1 && acc >= 0) {
            result = i;
            break;
        }
    }

    return result;
}

internal s32 PlayerCheck(f32 bpm, Call *call, f32 timer){
    s32 result = -1;

    f32 offset = (bpm == 94) ? 4.6 : 4.1;

    for (s32 i = 0; i < cast(s32) call->beat_count; i++) {
        f32 acc = (BeatCount(timer, bpm) - offset) - (call->beats[i]);
        if (acc < 0.2 && acc >= -0.2) {
            result = i + call->beat_count;
            break;
        }
    }

    return result;
}

internal void UpdateRenderModeBattle(Game_State *state, Game_Input *input,
        Draw_Command_Buffer *draw_buffer, Mode_Battle *battle)
{
    v4 clear_colour = V4(0.0, 41.0 / 255.0, 45.0 / 255.0, 1.0);
    Render_Batch _batch = CreateRenderBatch(draw_buffer, &state->assets, clear_colour);
    Render_Batch *batch = &_batch;

    SetCameraTransform(batch, 0, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1), V3(0, 0, 6));

    Game_Controller *controller = GameGetController(input, 1);
    if (!controller->connected) {
        controller = GameGetController(input, 0);
    }

    f32 dt = input->delta_time;
    battle->timer += dt;

    if (battle->metronome.x > battle->met_flip_range.x && battle->metronome.x < battle->met_flip_range.y) {
        f32 pos = battle->metronome.x - battle->met_flip_range.x;

        f32 alpha = pos / (battle->met_flip_range.y - battle->met_flip_range.x);
        battle->met_angle    = Lerp( 0.0f, -180.0f, alpha);
        battle->guide_height = Lerp(-2.3f,  2.0f,   alpha);
    }
    else if (battle->metronome.x > battle->met_flip_range.y) {
        battle->met_angle = 0;
    }

    Call *call = &battle->calls.calls[battle->calls.current_call];

    if (BeatCount(battle->timer, battle->bpm) >= battle->beat_wait) {
        battle->timer        = 0;
        battle->beat_wait    = 8;

        battle->metronome.x  = -2.8;
        battle->guide_height = 2;
        battle->met_angle    = 180;

        battle->wave        += 1;

        if(battle->wave > 0) {
            if (call->hits >= call->success_threshold) {
                battle->calls.current_call  += 1;
                battle->calls.attempt_count += call->beat_count;

                b32 all_notes = battle->calls.total_hits >= battle->calls.total_notes;
                b32 all_calls = battle->calls.current_call >= battle->calls.call_count;
                if (all_notes || all_calls) {
                    FlushAllPlayingSounds(state);

                    const char *name = (battle->type == EnemyType_FinalBoss) ? "finish_p" : "finish_2";
                    PlaySound(state, GetSoundByName(&state->assets, name), 0.3, 0);

                    battle->done = true;
                    return;
                }

                call = &battle->calls.calls[battle->calls.current_call];
            } else {
                s32 health = *battle->health;
                if (health > 0) { health -= 1; }

                *battle->health = health;

                if (health == 0) {
                    FlushAllPlayingSounds(state);

                    battle->done = true;
                    return;
                }
            }
        }

        call->hits        = 0;
        call->beats_shown = 0;
        ZeroSize(call->visible, sizeof(call->visible));

        PlaySound(state, call->call_handle, 0.3, 0);
    }

    if (battle->wave > -1) { battle->metronome.x += battle->met_speed * dt; }

    // @Todo: We won't need to do this if we just pre-lookup the image handle for the enemy and
    // bar we are currently fighting
    // :SoundHandle
    //

    // Basic textures
    //
    Image_Handle string_hurt = GetImageByName(&state->assets, "cross");
    Image_Handle harp        = GetImageByName(&state->assets, "player_strings");
    Image_Handle sprite      = GetImageByName(&state->assets, "player_backsprite");

    Image_Handle spotlight   = GetImageByName(&state->assets, "spotlight");
    Image_Handle enemy_box   = GetImageByName(&state->assets, "enemy_strings");

    Image_Handle metronome   = GetImageByName(&state->assets, "arrow");
    Image_Handle bar_fill    = GetImageByName(&state->assets, "bar_fill");

    // Enemy specific
    //
    Image_Handle enemy     = battle->enemy_texture;
    Image_Handle enemy_bar = battle->enemy_hud;

    f32 progress_hits  = cast(f32) battle->calls.total_hits;
    f32 progress_denom = cast(f32) Max(battle->calls.total_notes, battle->calls.attempt_count);
    f32 bar_progress   = Min(progress_hits / progress_denom, 1);
    v2 bar_pos         = V2(Lerp(-1.6, -5.4, bar_progress), 2.35);

    DrawQuad(batch, spotlight, V2( 1.8,  0.3), 2.5, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, sprite,    V2(-2.0, -1.7), 2.5, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, harp,      V2( 1.6, -1.5), 4.0, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, enemy,     V2( 1.8,  1.1), 2.6, 0, V4(1, 1, 1, 1)); // scale = 2.125?
    DrawQuad(batch, enemy_box, V2(-1.8,  1.1), 3.3, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, bar_fill,  bar_pos,        4.1, 0, V4(1, 1, 1, 1));
    DrawQuad(batch, enemy_bar, V2(-1.6, 2.35), 4.0, 0, V4(1, 1, 1, 1));

    DrawQuad(batch, { 0 },     V2(-4.1, 2.36), V2(1.0, 1.30), 0, V4(0, 41.0 / 255.0, 45.0 / 255.0, 1.0));
    DrawQuad(batch, { 0 },     V2(-3.83, 2.5), V2(0.5, 0.05), 0, V4(0, 41.0 / 255.0, 45.0 / 255.0, 1.0));

    u32 health = *battle->health;
    for (u32 i = 0; i < 4 - health; i++) {
        f32 height = -1.1 - 0.29 * i;
        DrawQuad(batch, string_hurt, V2(0, height), 0.2, 0, V4(1, 1, 1, 1));
    }

    v2 lineHeight = V2(battle->metronome.x, battle->metronome.y + battle->guide_height);
    DrawLine(batch, battle->metronome, lineHeight, V4(1, 1, 1, 1));
    DrawQuad(batch, metronome, battle->metronome, 0.4, Radians(battle->met_angle), V4(1, 1, 1, 1));

    s32 cpu_beat    =   BeatCheck(battle->bpm, call, battle->timer);
    s32 player_beat = PlayerCheck(battle->bpm, call, battle->timer);

    if (cpu_beat != -1 && battle->wave > -1) {
        if((cpu_beat + 1) > cast(s32) call->beats_shown){
            call->beats_shown = cpu_beat + 1;
            Button button    = call->beat_buttons[cpu_beat];

            call->pos[cpu_beat] = V2(battle->metronome.x, 1.4 - (0.23 * cast(f32) button));

            if (battle->type != EnemyType_FinalBoss) {
                PlaySound(state, call->enemy_note_handles[button], 0.3, 0);
            }
        }
    }

    b32 note_hit = false;
    if (player_beat != -1 && battle->wave > -1) {
        s32 beat_count = call->beat_count;
        if ((player_beat + 1) > cast(s32) call->beats_shown) { call->beats_shown = player_beat + 1; }

        u32 beat_index = player_beat % beat_count;
        b32 visible    = call->visible[beat_index] == true;
        Button button  = call->beat_buttons[beat_index];

        if (JustPressed(controller->buttons[button]) && !visible) {
            call->visible[beat_index] = true;
            call->hits += 1;
            battle->calls.total_hits += 1;

            PlaySound(state, call->player_note_handles[button], 0.3, 0);

            call->pos[player_beat] = V2(battle->metronome.x, -1.1 - (0.29 * cast(f32) button));

            note_hit = true;
        }
        else if (!visible) {
            call->visible[beat_index] = 100;
            call->pos[player_beat]    = V2(battle->metronome.x, -1.1 - (0.29 * cast(f32) button));
        }
    }

    u32 beat_count = cast(u32) call->beat_count;
    for (u32 i = 0; i < call->beats_shown; i++) {
        s32 beat_index = i % beat_count;

        Image_Handle beat = battle->beat_textures[call->beat_buttons[beat_index]];
        if (beat_count > i || call->visible[beat_index]) {
            v4 c = V4(1, 1, 1, 1);
            if (call->visible[beat_index] == 100) {
                c = V4(1, 0, 0, 0.70);
                v2 pos = call->pos[i];

                if (battle->debug_markers == 1) { pos.x += 0.21; }
                else if (battle->debug_markers == 2) { pos.x += 0.42; }

                DrawQuad(batch, beat, pos, 0.4, 0, c);
            }
            else {
                DrawQuad(batch, beat, call->pos[i], 0.4, 0, c);
            }
        }
    }

    if (!note_hit) {
        for (u32 i = 0; i < 4; i++) {
            if (JustPressed(controller->buttons[i])) {
                Sound_Handle sounds[] = {
                    GetSoundByName(&state->assets, "bad_1"),
                    GetSoundByName(&state->assets, "bad_2"),
                    GetSoundByName(&state->assets, "bad_3")
                };

                u32 sound_choice   = RandomChoice(&battle->random, 3);
                Sound_Handle sound = sounds[sound_choice];
                PlaySound(state, sound, 0.3, 0);

                // @Todo: Can hits actually go negative. I didn't think it could so I changed it so an unsigned
                // integer making the second bit redundant but if it can there it will cause issues
                //
                u32 hits   = call->hits;
                call->hits = Max(0, hits);
            }
        }
    }
}
