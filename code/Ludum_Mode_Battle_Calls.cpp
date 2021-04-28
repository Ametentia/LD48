internal Call CreateCall(Asset_Manager *assets, Random *random) {
    Call result = {};

    // Call backing track
    //
    result.call_handle = GetSoundByName(assets, "0_call_2");

    // Player notes
    //
    result.player_note_handles[0] = GetSoundByName(assets, "a3");
    result.player_note_handles[1] = GetSoundByName(assets, "g3");
    result.player_note_handles[2] = GetSoundByName(assets, "f3");
    result.player_note_handles[3] = GetSoundByName(assets, "g3");

    // Enemy notes
    //
    result.enemy_note_handles[0] = GetSoundByName(assets, "ea");
    result.enemy_note_handles[1] = GetSoundByName(assets, "eg");
    result.enemy_note_handles[2] = GetSoundByName(assets, "ef");
    result.enemy_note_handles[3] = GetSoundByName(assets, "eg");

    u32 count = 0;
    u32 hit   = 0;

    for (u32 i = 0; i < 10; i++) {
        if (NextRandom(random) % 20 < 10 - hit) {
            result.beats[count] = i * 0.25f;

            count += 1;
            hit   += 1;
        }
        else {
            hit = 0;
        }
    }

    for (u32 i = 0; i < count; i++) {
        result.beat_buttons[i] = cast(Button) (NextRandom(random) % 4);
    }

    result.beat_count       = count;
    result.success_threshold = count / 2;

    return result;
}

internal CallSet CreateRandom(Mode_Battle *battle, Random *random, Enemy_Type type) {
    CallSet result = {};

    // @Note: Was a -1 on the call_count so we can decrease these by 1 if we don't need that many
    //
    u32 count = 0;
    if (type == EnemyType_Boss) { count = RandomBetween(random, 7U, 9U); }
    else { count = RandomBetween(random, 3U, 5U); }

    result.call_count = count;
    result.calls      = AllocArray(battle->alloc, Call, result.call_count);

    for (u32 i = 0; i < count; i++) {
        result.calls[i] = CreateCall(battle->assets, random);
        result.total_notes += result.calls[i].beat_count;
    }

    return result;
}

internal void GetFinalBossCalls(Asset_Manager *assets, CallSet *set) {
    Sound_Handle hades_calls[] = {
        GetSoundByName(assets, "0_call_p"),
        GetSoundByName(assets, "1_call_p"),
        GetSoundByName(assets, "2_call_p"),
        GetSoundByName(assets, "3_call_p"),
        GetSoundByName(assets, "4_call_p"),
        GetSoundByName(assets, "5_call_p"),
        GetSoundByName(assets, "6_call_p"),
        GetSoundByName(assets, "7_call_p"),
        GetSoundByName(assets, "8_call_p"),
        GetSoundByName(assets, "9_call_p")
    };

    Sound_Handle note_harp_c = GetSoundByName(assets, "0X_harp_c");
    Sound_Handle note_e3 = GetSoundByName(assets, "e3");
    Sound_Handle note_db = GetSoundByName(assets, "db");
    Sound_Handle note_a3 = GetSoundByName(assets, "a3");
    Sound_Handle note_b3 = GetSoundByName(assets, "b3");
    Sound_Handle note_d3 = GetSoundByName(assets, "d3");
    Sound_Handle note_g3 = GetSoundByName(assets, "g3");
    Sound_Handle note_bb = GetSoundByName(assets, "bb");

    set->call_count  = 10;
    set->total_hits  = 5; // Why is this 5 by default?
    set->total_notes = 82; // This was 100 before, but adding up the beat_counts of each call is 82?

    set->calls[0] = {
        .beat_count   = 7,
        .beats        = { 0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 2.75 },
        .beat_buttons = {
            Button_Up, Button_Down, Button_Right, Button_Left, Button_Down, Button_Up, Button_Right
        },
        .success_threshold = 4,
        .call_handle = hades_calls[0],
        .player_note_handles = { note_e3, note_db, note_a3, note_b3 }
    };

    set->calls[1] = {
        .beat_count   = 7,
        .beats        = { 0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 2.75 },
        .beat_buttons = {
            Button_Up, Button_Down, Button_Right, Button_Left, Button_Down, Button_Up, Button_Right
        },
        .success_threshold   = 4,
        .call_handle         = hades_calls[1],
        .player_note_handles = { note_e3, note_db, note_a3, note_b3 }
    };

    set->calls[2] = {
        .beat_count   = 8,
        .beats        = { 0.0, 0.25, 0.75, 1.0, 1.25, 1.75, 2.25, 2.5 },
        .beat_buttons = {
            Button_Left, Button_Up, Button_Up, Button_Right, Button_Left, Button_Down, Button_Up, Button_Down
        },
        .success_threshold   = 4,
        .call_handle         = hades_calls[2],
        .player_note_handles = { note_e3, note_db, note_a3, note_b3 }
    };

    set->calls[3] = {
        .beat_count = 8,
        .beats      = { 0.0, 0.25, 0.75, 1.0, 1.25, 1.75, 2.25, 2.5 },
        .beat_buttons = {
            Button_Left, Button_Up, Button_Up, Button_Right, Button_Left, Button_Down, Button_Up, Button_Down
        },
        .success_threshold   = 4,
        .call_handle         = hades_calls[3],
        .player_note_handles = { note_e3, note_db, note_a3, note_b3 }
    };

    set->calls[4] = {
        .beat_count   = 8,
        .beats        = { 0.0, 0.25, 0.5, 1.0, 1.5, 2.0, 2.5, 2.75 },
        .beat_buttons = {
            // @Note: There aren't enough buttons in this for the number of beats?
            // Button_Up will just be used as default because its value is 0
            // :CallButtons
            //
            Button_Left, Button_Left, Button_Up, Button_Down, Button_Left, Button_Down
        },
        .success_threshold   = 4,
        .call_handle         = hades_calls[4],
        .player_note_handles = { note_e3, note_d3, note_a3, note_b3 }
    };

    set->calls[5] = {
        .beat_count   = 8,
        .beats        = { 0.0, 0.25, 0.5, 1.0, 1.5, 2.0, 2.5, 2.75 },
        .beat_buttons = {
            // :CallButtons
            //
            Button_Left, Button_Left, Button_Up, Button_Down, Button_Left, Button_Down
        },
        .success_threshold   = 4,
        .call_handle         = hades_calls[5],
        .player_note_handles = { note_e3, note_d3, note_a3, note_b3 }
    };

    set->calls[6] = {
        .beat_count   = 9,
        .beats        = { 0.0, 0.25, 0.5, 0.75, 1.0, 1.75, 2.25, 2.5, 2.75 },
        .beat_buttons = {
            // :CallButtons
            //
            Button_Left, Button_Left, Button_Left, Button_Down,
            Button_Left, Button_Left, Button_Down, Button_Right
        },
        .success_threshold   = 5,
        .call_handle         = hades_calls[6],
        .player_note_handles = { note_e3, note_g3, note_a3, note_harp_c }
    };

    set->calls[7] = {
        .beat_count   = 9,
        .beats        = { 0.0, 0.25, 0.5, 0.75, 1.0, 1.75, 2.25, 2.5, 2.75 },
        .beat_buttons = {
            // :CallButtons
            //
            Button_Left, Button_Left, Button_Left, Button_Down,
            Button_Left, Button_Left, Button_Down, Button_Right
        },
        .success_threshold   = 5,
        .call_handle         = hades_calls[7],
        .player_note_handles = { note_e3, note_g3, note_a3, note_harp_c }
    };

    set->calls[8] = {
        .beat_count   = 9,
        .beats        = { 0.0, 0.5, 0.75, 1.0, 1.25, 1.75, 2.25, 2.5, 2.75 },
        .beat_buttons = {
            Button_Left, Button_Left, Button_Left, Button_Right,
            Button_Left, Button_Left, Button_Left, Button_Down, Button_Left
        },
        .success_threshold   = 5,
        .call_handle         = hades_calls[8],
        .player_note_handles = { note_e3, note_bb, note_a3, note_g3 }
    };

    set->calls[9] = {
        .beat_count   = 9,
        .beats        = { 0.0, 0.25, 0.5, 1.0, 1.25, 1.75, 2.0, 2.25, 2.75 },
        .beat_buttons = {
            Button_Left, Button_Left, Button_Up, Button_Down,
            Button_Left, Button_Up, Button_Left, Button_Down, Button_Right
        },
        .success_threshold   = 5,
        .call_handle         = hades_calls[9],
        .player_note_handles = { note_g3, note_bb, note_a3, note_d3 }
    };
}

internal CallSet GetCallSet(Mode_Battle *battle, Random *random, Enemy_Type type) {
    CallSet result = {};

    if (type != EnemyType_FinalBoss) { result = CreateRandom(battle, random, type); }
    else {
        result.call_count = 9;
        result.calls      = AllocArray(battle->alloc, Call, result.call_count);

        GetFinalBossCalls(battle->assets, &result);
    }

    return result;
}
