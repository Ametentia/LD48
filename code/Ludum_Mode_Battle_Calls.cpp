internal Call CreateCall(Random *random, u8 song) {
    Call genned =  {
        {},
        {},
        0,
        0,
        {},
        {},
        0,
        0,
        "0_call_2",
        {
            "a3",
            "g3",
            "f3",
            "g3"
        },{
            "ea",
            "eg",
            "ef",
            "eg"
        }
    };
    u32 count = 0;
    u32 hit = 0;
    for(u32 i = 0; i < 10; i++) {
        if(NextRandom(random) % 20 < 10-hit) {
            genned.beats[count] = cast(f32)(i)*0.25;
            count++;
            hit++;
        }
        else {
            hit = 0;
        }
    }
    for(u32 i = 0; i < count; i++) {
        genned.beatButtons[i] = cast(Button)(NextRandom(random)%4);
    }
    genned.beat_count = count;
    genned.successThreshold = count/2;
    return genned;
}

internal CallSet CreateRandom(Random *random, u8 boss) {
    u32 count = (NextRandom(random) % 3)+ 3;
    if(boss) {
        count = (NextRandom(random) % 3)+ 7;
    }
    CallSet calls = {};
    calls.current_call = 0;
    calls.hits = 0;
    calls.attempt_count = 0;
    calls.song = 0;
    calls.call_count = count-1;
    
    for(u32 i = 0; i < count; i++) {
        calls.calls[i] = CreateCall(random, calls.song);
    }
    u32 note_count = 0;
    for(u32 i = 0; i < count; i++){
        note_count += calls.calls[i].beat_count;
    }
    calls.total_notes = note_count;
    return calls;
}

internal CallSet GetCall(Random *random, u8 finalBoss, u8 boss) {
    if(!finalBoss){
        return CreateRandom(random, boss);
    }
    CallSet encounters = {
        {
            {
                {0,0.5,1,1.5,2, 2.5, 2.75},
                {Button_Up, Button_Down, Button_Right, Button_Left, Button_Down, Button_Up, Button_Right},
                7,
                4,
                {},
                {},
                0,
                0,
                "0_call_p",
                {"e3",
                "db",
                "a3",
                "b3"}
            },{
                {0,0.5,1,1.5,2, 2.5, 2.75},
                {Button_Up, Button_Down, Button_Right, Button_Left, Button_Down, Button_Up, Button_Right},
                7,
                4,
                {},
                {},
                0,
                0,
                "1_call_p",
                {"e3",
                "db",
                "a3",
                "b3"}
            },{
                {0,0.25,0.75,1,1.25,1.75, 2.25, 2.5},
                {Button_Left, Button_Up, Button_Up, Button_Right, Button_Left, Button_Down, Button_Up, Button_Down},
                8,
                4,
                {},
                {},
                0,
                0,
                "2_call_p",
                {"e3",
                "db",
                "a3",
                "b3"}
            },{
                {0,0.25,0.75,1,1.25,1.75, 2.25, 2.5},
                {Button_Left, Button_Up, Button_Up, Button_Right, Button_Left, Button_Down, Button_Up, Button_Down},
                8,
                4,
                {},
                {},
                0,
                0,
                "3_call_p",
                {"e3",
                "db",
                "a3",
                "b3"}
            },{
                {0, 0.25, 0.5,1, 1.5, 2, 2.5, 2.75},
                {Button_Left, Button_Left, Button_Up, Button_Down, Button_Left, Button_Down},
                8,
                4,
                {},
                {},
                0,
                0,
                "4_call_p",
                {"e3",
                "d3",
                "a3",
                "b3"}
            },
            {
                {0, 0.25, 0.5,1, 1.5, 2, 2.5, 2.75},
                {Button_Left, Button_Left, Button_Up, Button_Down, Button_Left, Button_Down},
                8,
                4,
                {},
                {},
                0,
                0,
                "5_call_p",
                {"e3",
                "d3",
                "a3",
                "b3"}
            },{
                {0,0.25,0.5,0.75,1,1.75, 2.25, 2.5, 2.75},
                {Button_Left, Button_Left, Button_Left, Button_Down, Button_Left, Button_Left, Button_Left, Button_Down, Button_Right},
                9,
                5,
                {},
                {},
                0,
                0,
                "6_call_p",
                {"e3",
                "g3",
                "a3",
                "0X_harp_c"}
            },{
                {0,0.25,0.5,0.75,1,1.75, 2.25, 2.5, 2.75},
                {Button_Left, Button_Left, Button_Left, Button_Down, Button_Left, Button_Left, Button_Left, Button_Down, Button_Right},
                9,
                5,
                {},
                {},
                0,
                0,
                "7_call_p",
                {"e3",
                "g3",
                "a3",
                "0X_harp_c"}
            },{
                {0,0.5,0.75,1, 1.25,1.75, 2.25, 2.5, 2.75},
                {Button_Left, Button_Left, Button_Left, Button_Right, Button_Left, Button_Left, Button_Left, Button_Down, Button_Left},
                9,
                5,
                {},
                {},
                0,
                0,
                "8_call_p",
                {"e3",
                "bb",
                "a3",
                "g3"}
            },{
                {0,0.25,0.5,1,1.25, 1.75, 2, 2.25, 2.5, 2.75},
                {Button_Left, Button_Left, Button_Up, Button_Up, Button_Down, Button_Left, Button_Up, Button_Left, Button_Down, Button_Right},
                9,
                5,
                {},
                {},
                0,
                0,
                "9_call_p",
                {"g3",
                "bb",
                "a3",
                "d3"}
            }
        },
        0,
        5,
        100,
        0,
        1,
        9
    };
    return encounters;
}
