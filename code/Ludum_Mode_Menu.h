#if !defined(LUDUM_MODE_MENU_H_)
#define LUDUM_MODE_MENU_H_

struct Ui_Context {
    Render_Batch *batch;
    v2 mouse;
    Game_Button left_button;

    u32 hot;
    u32 active;
};

struct Mode_Menu {
    Memory_Allocator *alloc;

    Playing_Sound *music;

    b32 showing_credits;
    Ui_Context ui;
};

internal void ModeMenu(Game_State *state);

#endif  // LUDUM_MODE_MENU_H_
