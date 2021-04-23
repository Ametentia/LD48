#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#include <gl/gl.h>

#include <SDL2/SDL_video.h>

#include "Ludum_Types.h"
#include "Ludum_Utility.h"
#include "Ludum_Renderer.h"

global SDL_Window *global_window;
global SDL_GLContext global_gl_context;

#include "Ludum_Renderer_OpenGL.h"
#include "Ludum_Renderer_OpenGL.cpp"

typedef void type_wglSwapIntervalEXT(int);
global type_wglSwapIntervalEXT *wglSwapIntervalEXT;

internal void *WinOpenGLAllocate(umm size) {
    void *result = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    return result;
}

internal RENDERER_BEGIN_FRAME(WinOpenGLBeginFrame) {
    OpenGL_State *gl = cast(OpenGL_State *) renderer;

    Draw_Command_Buffer *result = OpenGLBeginFrame(gl, render_size);
    return result;
}

internal RENDERER_END_FRAME(WinOpenGLEndFrame) {
    OpenGL_State *gl = cast(OpenGL_State *) renderer;
    OpenGLEndFrame(gl, buffer);

    SDL_GL_SwapWindow(global_window);
}

internal RENDERER_SHUTDOWN(WinOpenGLShutdown) {
    OpenGL_State *gl = cast(OpenGL_State *) renderer;
    if (gl) {
        // @Todo: Clean up resources and deallocate
        //
    }
}

extern "C" __declspec(dllexport) RENDERER_INITIALISE(WinOpenGLInitialise) {
    OpenGL_State *gl = cast(OpenGL_State *) WinOpenGLAllocate(sizeof(OpenGL_State));

    Renderer_Context *result = &gl->renderer;

    result->flags      = 0;

    result->Initalise  = WinOpenGLInitialise;
    result->Shutdown   = WinOpenGLShutdown;

    result->EndFrame   = 0;
    result->BeginFrame = 0;

    global_window = cast(SDL_Window *) params->platform_data;

    // Create OpenGL context
    //
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,              8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,            8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,             8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,            8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,            24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,          SDL_TRUE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    global_gl_context = SDL_GL_CreateContext(global_window);
    if (!global_gl_context) {
        // @Todo(James): Logging...
        //
        return result;
    }

    // Allocate immediate mode vertex array
    //
    gl->max_immediate_vertex_count = params->max_immediate_vertex_count;
    umm immediate_vertex_size      = gl->max_immediate_vertex_count * sizeof(Textured_Vertex);
    gl->immediate_vertices         = cast(Textured_Vertex *) WinOpenGLAllocate(immediate_vertex_size);

    // Allocate immediate mode index array
    //
    gl->max_immediate_index_count  = params->max_immediate_index_count;
    umm immediate_index_size       = gl->max_immediate_index_count * sizeof(u32);
    gl->immediate_indices          = cast(u32 *) WinOpenGLAllocate(immediate_index_size);

    // Allocate command buffer memory
    //
    gl->command_buffer_size = params->command_buffer_size;
    gl->command_buffer_base = cast(u8 *) WinOpenGLAllocate(gl->command_buffer_size);

    // Allocate texture handles
    //
    gl->max_texture_handle_count = params->max_texture_handle_count;
    gl->texture_handles = cast(GLuint *) WinOpenGLAllocate(gl->max_texture_handle_count * sizeof(GLuint));

    // Allocate texture transfer buffer
    //
    Texture_Transfer_Buffer *texture_buffer = &result->texture_buffer;
    texture_buffer->size = params->texture_transfer_buffer_size;
    texture_buffer->base = cast(u8 *) WinOpenGLAllocate(texture_buffer->size);
    texture_buffer->used = 0;

    // Load OpenGL extension functions. All functions must be loaded here
    //
#define WGL_LOAD_FUNCTION(name) name = (type_##name *) wglGetProcAddress(#name)
    WGL_LOAD_FUNCTION(glShaderSource);
    WGL_LOAD_FUNCTION(glCompileShader);
    WGL_LOAD_FUNCTION(glGetShaderiv);
    WGL_LOAD_FUNCTION(glGetShaderInfoLog);
    WGL_LOAD_FUNCTION(glDeleteShader);
    WGL_LOAD_FUNCTION(glAttachShader);
    WGL_LOAD_FUNCTION(glLinkProgram);
    WGL_LOAD_FUNCTION(glGetProgramiv);
    WGL_LOAD_FUNCTION(glGetProgramInfoLog);
    WGL_LOAD_FUNCTION(glDeleteProgram);
    WGL_LOAD_FUNCTION(glGenVertexArrays);
    WGL_LOAD_FUNCTION(glGenBuffers);
    WGL_LOAD_FUNCTION(glBindVertexArray);
    WGL_LOAD_FUNCTION(glBindBuffer);
    WGL_LOAD_FUNCTION(glVertexAttribPointer);
    WGL_LOAD_FUNCTION(glEnableVertexAttribArray);
    WGL_LOAD_FUNCTION(glBufferData);
    WGL_LOAD_FUNCTION(glUseProgram);
    WGL_LOAD_FUNCTION(glUniformMatrix4fv);
    WGL_LOAD_FUNCTION(glGenerateMipmap);

    WGL_LOAD_FUNCTION(glGetUniformLocation);

    WGL_LOAD_FUNCTION(glCreateShader);
    WGL_LOAD_FUNCTION(glCreateProgram);

    WGL_LOAD_FUNCTION(wglSwapIntervalEXT);
#undef WGL_LOAD_FUNCTION

    if (wglSwapIntervalEXT) {
        // Enable vsync by default
        //
        wglSwapIntervalEXT(1);
    }

    if (!OpenGLInitalise(gl)) {
        return result;
    }

    result->BeginFrame = WinOpenGLBeginFrame;
    result->EndFrame   = WinOpenGLEndFrame;

    result->flags |= RendererContext_Initalised;

    return result;
}
