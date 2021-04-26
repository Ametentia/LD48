// We can initialise the play mode here with whatever requires initialisation
//
#include "Ludum_Mode_Battle.cpp"
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
    play->player[0] = CreateAnimation(GetImageByName(&state->assets, "forward_walk"),  4, 1, 0.23);
    play->player[1] = CreateAnimation(GetImageByName(&state->assets, "backward_walk"), 4, 1, 0.23);
    play->player[2] = CreateAnimation(GetImageByName(&state->assets, "right_walk"),    2, 1, 0.23);
    play->player[3] = CreateAnimation(GetImageByName(&state->assets, "left_walk"),     2, 1, 0.23);
    Sound_Handle world_music = GetSoundByName(&state->assets, "overworld");
    play->music = PlaySound(state, world_music, 0.2, PlayingSound_Looped);

    Random rng = RandomSeed(time(0));
    Image_Handle handles[] = {
        { 0 },
        GetImageByName(&state->assets, "tile_00"),
        GetImageByName(&state->assets, "tile_01"),
        GetImageByName(&state->assets, "tile_02"),
        GetImageByName(&state->assets, "object_00"),
        GetImageByName(&state->assets, "object_01")
    };

    play->last_anim = &play->player[0];

    for(umm i = 0; i < play->map_size.x; i++){
        for(umm j = 0; j <play->map_size.y; j++){
            play->tile_arr[i*(umm)play->map_size.x+j].texture = handles[RandomChoice(&rng, ArrayCount(handles))];
        }
    }

    state->mode = GameMode_Play;
    state->play = play;
}

// Draw map based on size, tile dimensions and tile spacing
//
internal void DrawMap(Render_Batch *batch, Mode_Play *play, v2 size, v2 tile_dims, f32 spacing){

    for(umm i = 0; i < size.x; i++){
        for(umm j = 0; j < size.y; j++){
            Image_Handle texture = play->tile_arr[i*(umm)play->map_size.x+j].texture;
            DrawQuad(batch, {0}, V2(i*(tile_dims.x+spacing), j*(tile_dims.x+spacing)), V2(tile_dims.x,tile_dims.y), 0, V4(196/255.0, 240/255.0, 194/255.0, 1));
            if(IsValid(texture)){
                DrawQuad(batch, texture, V2(i*(tile_dims.x+spacing), j*(tile_dims.x+spacing)), V2(tile_dims.x,tile_dims.y));
            }
            play->tile_arr[i*(umm)size.x+j].pos = V2(i*(tile_dims.x+spacing), j*(tile_dims.y+spacing));
        }
    }
}

// This is where most of the game logic will happen
//
internal void UpdateRenderModePlay(Game_State *state, Game_Input *input, Draw_Command_Buffer *draw_buffer) {
    Mode_Play *play = state->play;
    if(play->in_battle) {
        UpdateRenderModeBattle(state, input, draw_buffer, play->battle);
        if(play->battle->done) {
            EndTemp(play->battle_mem);
            play->in_battle = 0;
            Sound_Handle world_music = GetSoundByName(&state->assets, "overworld");
            play->music = PlaySound(state, world_music, 0.2, PlayingSound_Looped);
        }
        return;
    }

    Render_Batch _batch = CreateRenderBatch(draw_buffer, &state->assets,
            V4(0.0, 41.0 / 255.0, 45.0 / 255.0, 1.0));
    Render_Batch *batch = &_batch;

    Game_Controller *controller = GameGetController(input, 1);

    if (!controller->connected) {
        controller = GameGetController(input, 0);
    }

    f32 dt = input->delta_time;

    f32 dx = play->tile_size.x+play->tile_spacing;
    f32 dy = play->tile_size.y+play->tile_spacing;

    if (JustPressed(controller->left)) {
        if (play->player_position.x != 0) {
            play->player_position -= V2(dx, 0);
        }

        play->last_anim = &play->player[3];
    }

    if (JustPressed(controller->right)) {
        if (play->player_position.x + dx < dx * play->map_size.x) {
            play->player_position += V2(dx, 0);
        }

        play->last_anim = &play->player[2];
    }

    if (JustPressed(controller->up)) {
        if (play->player_position.y + dy < dy * play->map_size.y) {
            play->player_position += V2(0, dy);
        }

        play->last_anim = &play->player[1];
    }

    if (JustPressed(controller->down)) {
        if (play->player_position.y != 0) {
            play->player_position -= V2(0, dy);
        }

        play->last_anim = &play->player[0];
    }

    if (JustPressed(controller->action)) {
        play->battle_mem = BeginTemp(play->alloc);
        play->in_battle = 1;
        play->battle = AllocStruct(play->alloc, Mode_Battle);
        ModeBattle(state, play->battle, &play->battle_mem);
        play->music->volume = 0;
        play->music->flags = 0;
    }

    Animation *anim = play->last_anim;

    SetCameraTransform(batch, 0, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1), V3(play->player_position, 6));

    UpdateAnimation(&play->player[0], dt);
    UpdateAnimation(&play->player[1], dt);
    UpdateAnimation(&play->player[2], dt);
    UpdateAnimation(&play->player[3], dt);

    DrawMap(batch, play, play->map_size, play->tile_size, play->tile_spacing);
    DrawAnimation(batch, anim, dt, V3(play->player_position), V2(0.6, 0.6));
}
