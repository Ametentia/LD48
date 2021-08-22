#include <sys/mman.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include <SDL2/SDL_video.h>

#include "Ludum_Types.h"
#include "Ludum_Utility.h"
#include "Ludum_Renderer.h"

global SDL_Window *global_window;
global SDL_GLContext global_gl_context;

#include "Ludum_Renderer_OpenGL.h"
#include "Ludum_Renderer_OpenGL.cpp"

typedef void type_glXSwapIntervalEXT(Display *, GLXDrawable, int);
global type_glXSwapIntervalEXT *glXSwapIntervalEXT;

internal void *LinuxOpenGLAllocate(umm size) {
    void *result = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    return result;
}

internal RENDERER_BEGIN_FRAME(LinuxOpenGLBeginFrame) {
    OpenGL_State *gl = cast(OpenGL_State *) renderer;

    Draw_Command_Buffer *result = OpenGLBeginFrame(gl, render_region);
    return result;
}

internal RENDERER_END_FRAME(LinuxOpenGLEndFrame) {
    OpenGL_State *gl = cast(OpenGL_State *) renderer;
    OpenGLEndFrame(gl, buffer);

    SDL_GL_SwapWindow(global_window);
}

internal RENDERER_SHUTDOWN(LinuxOpenGLShutdown) {
    OpenGL_State *gl = cast(OpenGL_State *) renderer;
    if (gl) {
        // @Todo: Clean up resources and deallocate
        //
    }
}

extern "C" RENDERER_INITIALISE(LinuxOpenGLInitialise) {
    OpenGL_State *gl = cast(OpenGL_State *) LinuxOpenGLAllocate(sizeof(OpenGL_State));

    Renderer_Context *result = &gl->renderer;

    result->flags      = 0;

    result->Initalise  = LinuxOpenGLInitialise;
    result->Shutdown   = LinuxOpenGLShutdown;

    result->EndFrame   = 0;
    result->BeginFrame = 0;

    global_window = cast(SDL_Window *) params->platform_data;

    // Create OpenGL context
    //
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
    gl->immediate_vertices         = cast(Textured_Vertex *) LinuxOpenGLAllocate(immediate_vertex_size);

    // Allocate immediate mode index array
    //
    gl->max_immediate_index_count  = params->max_immediate_index_count;
    umm immediate_index_size       = gl->max_immediate_index_count * sizeof(u32);
    gl->immediate_indices          = cast(u32 *) LinuxOpenGLAllocate(immediate_index_size);

    // Allocate command buffer memory
    //
    gl->command_buffer_size = params->command_buffer_size;
    gl->command_buffer_base = cast(u8 *) LinuxOpenGLAllocate(gl->command_buffer_size);

    // Allocate texture handles
    //
    gl->max_texture_handle_count = params->max_texture_handle_count;
    gl->texture_handles = cast(GLuint *) LinuxOpenGLAllocate(gl->max_texture_handle_count * sizeof(GLuint));

    // Allocate texture transfer buffer
    //
    Texture_Transfer_Buffer *texture_buffer = &result->texture_buffer;
    texture_buffer->size = params->texture_transfer_buffer_size;
    texture_buffer->base = cast(u8 *) LinuxOpenGLAllocate(texture_buffer->size);
    texture_buffer->used = 0;

    // Load OpenGL extension functions. All functions must be loaded here
    //
#define GLX_LOAD_FUNCTION(name) name = (type_##name *) glXGetProcAddress((const GLubyte *) #name)
    GLX_LOAD_FUNCTION(glShaderSource);
    GLX_LOAD_FUNCTION(glCompileShader);
    GLX_LOAD_FUNCTION(glGetShaderiv);
    GLX_LOAD_FUNCTION(glGetShaderInfoLog);
    GLX_LOAD_FUNCTION(glDeleteShader);
    GLX_LOAD_FUNCTION(glAttachShader);
    GLX_LOAD_FUNCTION(glLinkProgram);
    GLX_LOAD_FUNCTION(glGetProgramiv);
    GLX_LOAD_FUNCTION(glGetProgramInfoLog);
    GLX_LOAD_FUNCTION(glDeleteProgram);
    GLX_LOAD_FUNCTION(glGenVertexArrays);
    GLX_LOAD_FUNCTION(glGenBuffers);
    GLX_LOAD_FUNCTION(glBindVertexArray);
    GLX_LOAD_FUNCTION(glBindBuffer);
    GLX_LOAD_FUNCTION(glVertexAttribPointer);
    GLX_LOAD_FUNCTION(glEnableVertexAttribArray);
    GLX_LOAD_FUNCTION(glBufferData);
    GLX_LOAD_FUNCTION(glUseProgram);
    GLX_LOAD_FUNCTION(glUniformMatrix4fv);
    GLX_LOAD_FUNCTION(glGenerateMipmap);

    GLX_LOAD_FUNCTION(glGetUniformLocation);

    GLX_LOAD_FUNCTION(glCreateShader);
    GLX_LOAD_FUNCTION(glCreateProgram);

    GLX_LOAD_FUNCTION(glXSwapIntervalEXT);
#undef GLX_LOAD_FUNCTION

    if (glXSwapIntervalEXT) {
        Display *display     = glXGetCurrentDisplay();
        GLXDrawable drawable = glXGetCurrentDrawable();

        if (drawable) {
            glXSwapIntervalEXT(display, drawable, 1);
        }
    }
    else {
        printf("Couldn't get vsync\n");
    }

    if (!OpenGLInitalise(gl)) {
        return result;
    }

    result->BeginFrame = LinuxOpenGLBeginFrame;
    result->EndFrame   = LinuxOpenGLEndFrame;

    result->flags |= RendererContext_Initalised;

    return result;

}
