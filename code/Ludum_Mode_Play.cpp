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
    play->tile_spacing = 0;
    play->is_shop = true;
    play->shop_data.shop_tiles = AllocArray(play->alloc, Shop_Tile, 9);
    play->shop_data.tile_indexes = AllocArray(play->alloc, umm, 9);
    
    play->player[0] = CreateAnimation(GetImageByName(&state->assets, "forward_walk"),  4, 1, 0.23);
    play->player[1] = CreateAnimation(GetImageByName(&state->assets, "backward_walk"), 4, 1, 0.23);
    play->player[2] = CreateAnimation(GetImageByName(&state->assets, "right_walk"),    2, 1, 0.23);
    play->player[3] = CreateAnimation(GetImageByName(&state->assets, "left_walk"),     2, 1, 0.23);
    play->last_anim = &play->player[0];

    play->hermes = CreateAnimation(GetImageByName(&state->assets, "hermes_overworld"),  2, 1, 0.23);

    state->mode = GameMode_Play;
    state->play = play;
    GenerateShop(state, play);
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
}

internal void GenerateShop(Game_State *state, Mode_Play *play){
    Shop_Tile *tiles_arr = AllocArray(play->alloc, Shop_Tile, 4);
    tiles_arr[0] = {
        V2(1,1),
        {0},
        GetImageByName(&state->assets, "shop_tile"),
        0,
        0.2f,
        false
    };
    tiles_arr[1] = {
        V2(1,1),
        GetImageByName(&state->assets, "apollo_amulet"),
        GetImageByName(&state->assets, "shop_tile"),
        10,
        0.3f,
        false
    };
    tiles_arr[2] = {
        V2(1,1),
        GetImageByName(&state->assets, "extra_string"),
        GetImageByName(&state->assets, "shop_tile"),
        10,
        0.4f,
        false
    };
    tiles_arr[3] = {
        V2(1,1),
        GetImageByName(&state->assets, "string_reinforcement"),
        GetImageByName(&state->assets, "shop_tile"),
        10,
        0.2f,
        false
    };
    tiles_arr[4] = {
        V2(1,1),
        {0},
        GetImageByName(&state->assets, "shop_tile"),
        10,
        0.2f,
        true
    };

    Random rng = RandomSeed(time(0));
    v2 shop_tiles_grid = V2(3,3);
    umm items = 0;
    bool hermes = false;
    for(umm i = 0; i < shop_tiles_grid.x; i++){
        for(umm j = 0; j < shop_tiles_grid.y; j++){
            play->shop_data.tile_indexes[i*(umm)shop_tiles_grid.x+j] = ((play->map_size.x/2)-1+i)*play->map_size.x + (play->map_size.y/2)-1+j;
            if(items < 5){
                umm rand = RandomBetween(&rng, 0U, 3U);
                play->shop_data.shop_tiles[i*3+j] = tiles_arr[rand];
                if(rand > 0){
                    items++;
                }
            }
            else{
                if(hermes){
                    play->shop_data.shop_tiles[i*3+j] = tiles_arr[0];
                }
                else{
                    play->shop_data.shop_tiles[i*3+j] = tiles_arr[4];
                    hermes=true;
                }
            }
        }
    }
}

// Returns if an index is in the shop's tiles
//
internal bool InShop(umm index, Mode_Play *play){
    for(umm i = 0; i < 9; i++){
        if(index == play->shop_data.tile_indexes[i]){
            return true;
        }
    }
    return false;
}

// Draw map based on size, tile dimensions and tile spacing
//
internal void DrawMap(Render_Batch *batch, Mode_Play *play, v2 size, v2 tile_dims, f32 spacing, f32 dt){
    umm shop_tile_count = 0;
    for(umm i = 0; i < size.x; i++){
        for(umm j = 0; j < size.y; j++){
            Image_Handle texture = play->tile_arr[i*(umm)play->map_size.x+j].texture;
            v2 pos = V2(i*(tile_dims.x+spacing), j*(tile_dims.y+spacing));
            DrawQuad(batch, {0}, pos, V2(tile_dims.x,tile_dims.y), 0, V4(196/255.0, 240/255.0, 194/255.0, 1));
            if(play->is_shop && InShop(i*(umm)size.x+j, play)){
                DrawQuad(batch, play->shop_data.shop_tiles[shop_tile_count].bg_texture, pos, V2(tile_dims.x,tile_dims.y));
                if(IsValid(play->shop_data.shop_tiles[shop_tile_count].tile.texture)){
                    DrawQuad(batch, play->shop_data.shop_tiles[shop_tile_count].tile.texture, pos, V2(tile_dims.x,tile_dims.y));
                }
                if(play->shop_data.shop_tiles[shop_tile_count].hermes){
                    UpdateAnimation(&play->hermes, dt);
                    DrawAnimation(batch, &play->hermes, dt, V3(pos), V2(0.6,0.6));
                }
                shop_tile_count++;
            }
            else if(IsValid(texture)){
                DrawQuad(batch, texture, pos, V2(tile_dims.x,tile_dims.y));
            }
            play->tile_arr[i*(umm)size.x+j].pos = pos;
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
    }

    Animation *anim = play->last_anim;

    SetCameraTransform(batch, 0, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1), V3(play->player_position, 6));

    UpdateAnimation(&play->player[0], dt);
    UpdateAnimation(&play->player[1], dt);
    UpdateAnimation(&play->player[2], dt);
    UpdateAnimation(&play->player[3], dt);

    DrawMap(batch, play, play->map_size, play->tile_size, play->tile_spacing, dt);
    DrawAnimation(batch, anim, dt, V3(play->player_position), V2(0.6, 0.6));
}
