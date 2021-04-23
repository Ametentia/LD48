#if !defined(LUDUM_RENDERER_H_)
#define LUDUM_RENDERER_H_

struct Draw_Command_Buffer;
struct Renderer_Parameters;
struct Renderer_Context;

#define RENDERER_INITIALISE(name) Renderer_Context *name(Renderer_Parameters *params)
#define RENDERER_SHUTDOWN(name) void name(Renderer_Context *renderer)

typedef RENDERER_INITIALISE(Renderer_Initialise);
typedef RENDERER_SHUTDOWN(Renderer_Shutdown);

#define RENDERER_BEGIN_FRAME(name) Draw_Command_Buffer *name(Renderer_Context *renderer, v2u render_size)
#define RENDERER_END_FRAME(name) void name(Renderer_Context *renderer, Draw_Command_Buffer *buffer)

typedef RENDERER_BEGIN_FRAME(Renderer_Begin_Frame);
typedef RENDERER_END_FRAME(Renderer_End_Frame);

// @Todo(James): Maybe I should change the name of this to something else because the way you look up
// textures from the asset manager is using the 'Image_Handle' structure and this could be confusing
// because of how similar their names are
//
union Texture_Handle {
    struct {
        u32 index;
        u16 width;
        u16 height;
    };

    u64 value;
};

struct Renderer_Parameters {
    umm command_buffer_size;

    u32 max_immediate_vertex_count;
    u32 max_immediate_index_count;

    u32 max_texture_handle_count;
    umm texture_transfer_buffer_size;

    void *platform_data;
};

// :TextureParameters Maybe we want flags in this for operating on texture parameters
//
union Texture_Transfer_Info {
    struct {
        Texture_Handle handle;
        umm transfer_size;
    };

    u8 pad[64]; // Pad to 64 bytes
};

struct Texture_Transfer_Buffer {
    u8 *base;
    umm size;
    umm used;
};

enum Renderer_Context_Flags {
    RendererContext_Initalised = 0x1,
};

struct Renderer_Context {
    u64 flags;

    // This is included for completeness sakes but doesn't really need to be called directly out of this
    // This will *always* be the same as the function pointer loaded from the .dll/.so
    //
    // Given this structure is able to be allocated the Shutdown function pointer will *always* be valid
    // regardless of if the renderer was successfully initalised
    //
    Renderer_Initialise  *Initalise;
    Renderer_Shutdown    *Shutdown;

    Renderer_Begin_Frame *BeginFrame;
    Renderer_End_Frame   *EndFrame;

    Texture_Transfer_Buffer texture_buffer;
};

enum Draw_Command_Type {
    DrawCommand_Draw_Command_Clear = 0,
    DrawCommand_Draw_Command_Vertex_Batch
};

struct Draw_Command_Clear {
    v4 colour;
    b32 depth;
};

struct Draw_Command_Vertex_Batch {
    // Does not need to be set. The 0 index is reserved for a 1x1 white pixel which can be used for 'no texture'
    //
    Texture_Handle texture;

    mat4 transform;

    u32 vertex_offset;
    u32 vertex_count;

    u32 index_offset;
    u32 index_count;
};

struct Renderer_Setup {
    v2u window_size;
    v2u render_size;
};

struct Draw_Command_Buffer {
    Renderer_Setup setup;

    u8 *base;
    umm size;
    umm used;

    Textured_Vertex *vertices;
    u32 vertex_count;
    u32 max_vertex_count;

    u32 *indices;
    u32 index_count;
    u32 max_index_count;
};

enum Render_Transform_Flags {
    RenderTransform_Orthographic = 0x1,
};

struct Camera_Transform {
    v3 x;
    v3 y;
    v3 z;
    v3 p;
};

struct Render_Transform {
    u32 flags;

    Camera_Transform camera;
    mat4_inv projection;
};

struct Asset_Manager;

struct Render_Batch {
    Draw_Command_Buffer *buffer;

    Asset_Manager *assets;

    Render_Transform game_tx;
    Draw_Command_Vertex_Batch *vertex_batch;
};

inline b32 IsValid(Renderer_Context *renderer) {
    b32 result = (renderer != 0) && (renderer->flags & RendererContext_Initalised);
    return result;
}

inline b32 IsValid(Texture_Handle handle) {
    b32 result = (handle.value != 0);
    return result;
}

inline b32 IsEqual(Texture_Handle a, Texture_Handle b) {
    b32 result = (a.value == b.value);
    return result;
}

// Used to set the camera transform. x, y, z are the coordinate axes of the camera transform and can be scaled
// and way you feel like. p is the position of the camera, moving this will scroll the camera etc.
//
// :Render_Transform_Flags see this for flag details
//
// n and f are the near and far planes of the camera frustum. fov is only used for perspective (default).
// fov is in radians.
//
// The centre of the screen is always (0, 0). This makes sure it scales correctly with different resolutions
// and window sizes
//
//
internal void SetCameraTransform(Render_Batch *batch, u32 flags, v3 x, v3 y, v3 z, v3 p, f32 n, f32 f, f32 fov);

// This will unproject from 2d screen-space coordinates to 3d world-space coordinates based on the passed render
// transform
//
internal v3 Unproject(Render_Transform *tx, v2 clip_xy);

// This will get the bounds of the camera frustum in world space
//
internal rect3 GetCameraBounds(Render_Transform *tx);

// Create a render batch for calling the draw commands
//
// The Draw_Command_Buffer will be passed to the game from the platform layer (it comes from the renderer)
// The Asset_Manager is on the Game_State which will be available in the main game entry point
// Clear colour can be whatever. It automatically does a DrawClear with this colour.
//
internal Render_Batch CreateRenderBatch(Draw_Command_Buffer *command_buffer,
        Asset_Manager *assets, v4 clear_colour);

// Draw commands
// Should be called every frame in 'immediate' style
//
// Clear           - clears the screen to specified colour, optionally clears depth buffer
// DrawQuad        - draws a quad at the given centre, dimensions and angle
// DrawQuadOutline - draws a quad with lines to make an outlined version with transparent interior
// DrawLine        - draws a line from start to end with given colours and thickness
// DrawCircle      - draws a circle at the given centre and radius.
//
// Drawing quads and circles can be textured using an image handle. Using an image handle of { 0 } will use
// a default white texture which can be used for drawing solid coloured versions.
//
struct Image_Handle;
internal void DrawClear(Render_Batch *batch, v4 colour, b32 depth);
internal void DrawQuad(Render_Batch *batch, Image_Handle image, v2 centre, v2 dim, f32 angle, v4 colour);
internal void DrawQuad(Render_Batch *batch, Image_Handle image, v2 centre, f32 scale, f32 angle, v4 colour);
internal void DrawQuadOutline(Render_Batch *batch, v2 centre, v2 dim, f32 angle, v4 colour, f32 thickness);
internal void DrawLine(Render_Batch *batch, v2 start, v2 end, v4 start_colour, v4 end_colour, f32 thickness);
internal void DrawCircle(Render_Batch *batch, Image_Handle image,
        v2 centre, f32 radius, f32 angle, v4 colour, u32 segment_count);

struct Animation;

// Create an animation based of the given texture atlas with a number of rows and columns
//
internal Animation CreateAnimation(Image_Handle image, u32 rows, u32 columns, f32 time_per_frame);

// Update and draw the animation with the specified parameters
//
internal void DrawAnimation(Render_Batch *batch, Animation *anim, f32 delta_time,
        v2 centre, f32 scale, f32 angle, v4 colour);

// Ignore me :)
// This needs to be forward declared for the asset system because C++ is a crap langauge
//
internal Texture_Transfer_Info *GetTextureTransferMemory(Texture_Transfer_Buffer *buffer, u32 width, u32 height);

#endif  // LUDUM_RENDERER_H_
