#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>

#include <switch.h>

#include "Ludum_Types.h"
#include "Ludum_Utility.h"
#include "Ludum_Renderer.h"

#include "Ludum_Renderer_OpenGL.h"
#include "Ludum_Renderer_OpenGL.cpp"

global EGLDisplay global_display;
global EGLSurface global_surface;
global EGLContext global_context;

internal void *SwitchOpenGLAllocate(umm size) {
    void *result = malloc(size);
    return result;
}

internal void SwitchOpenGLDeallocate(void *mem) {
    free(mem);
}

internal RENDERER_BEGIN_FRAME(SwitchOpenGLBeginFrame) {
    OpenGL_State *gl = cast(OpenGL_State *) renderer;

    Draw_Command_Buffer *result = OpenGLBeginFrame(gl, render_region);
    return result;
}

internal RENDERER_END_FRAME(SwitchOpenGLEndFrame) {
    OpenGL_State *gl = cast(OpenGL_State *) renderer;
    OpenGLEndFrame(gl, buffer);

    eglSwapBuffers(eglGetCurrentDisplay(), eglGetCurrentSurface(EGL_DRAW));
}

internal RENDERER_SHUTDOWN(SwitchOpenGLShutdown) {
    OpenGL_State *gl = cast(OpenGL_State *) renderer;
    if (gl) {
        SwitchOpenGLDeallocate(renderer->texture_buffer.base);
        SwitchOpenGLDeallocate(gl->texture_handles);
        SwitchOpenGLDeallocate(gl->command_buffer_base);
        SwitchOpenGLDeallocate(gl->immediate_indices);
        SwitchOpenGLDeallocate(gl->immediate_vertices);

        SwitchOpenGLDeallocate(gl);

        eglMakeCurrent(global_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (global_context) { eglDestroyContext(global_display, global_context); }
        if (global_surface) { eglDestroySurface(global_display, global_surface); }

        eglTerminate(global_display);
    }
}

RENDERER_INITIALISE(SwitchOpenGLInitialise) {
    OpenGL_State *gl = cast(OpenGL_State *) SwitchOpenGLAllocate(sizeof(OpenGL_State));

    Renderer_Context *result = &gl->renderer;

    result->flags      = 0;

    result->Initalise  = SwitchOpenGLInitialise;
    result->Shutdown   = SwitchOpenGLShutdown;

    result->EndFrame   = 0;
    result->BeginFrame = 0;

    NWindow *window = cast(NWindow *) params->platform_data;

    global_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (global_display == EGL_NO_DISPLAY) {
        return result;
    }

    EGLint major, minor;
    if (!eglInitialize(global_display, &major, &minor)) {
        return result;
    }

    if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
        return result;
    }

    EGLConfig config;
    EGLint config_count;

    EGLint fb_attrs[] = {
        EGL_RED_SIZE,     8,
        EGL_BLUE_SIZE,    8,
        EGL_GREEN_SIZE,   8,
        EGL_ALPHA_SIZE,   8,
        EGL_DEPTH_SIZE,   24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };

    eglChooseConfig(global_display, fb_attrs, &config, 1, &config_count);
    if (config_count == 0) {
        return result;
    }

    global_surface = eglCreateWindowSurface(global_display, config, window, 0);
    if (!global_surface) {
        return result;
    }

    EGLint context_attrs[] = {
        EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
        EGL_CONTEXT_MAJOR_VERSION_KHR, 4,
        EGL_CONTEXT_MINOR_VERSION_KHR, 3,
        EGL_NONE
    };

    global_context = eglCreateContext(global_display, config, EGL_NO_CONTEXT, context_attrs);
    if (!global_context) {
        return result;
    }

    eglMakeCurrent(global_display, global_surface, global_surface, global_context);

    // Allocate immediate mode vertex array
    //
    gl->max_immediate_vertex_count = params->max_immediate_vertex_count;
    umm immediate_vertex_size      = gl->max_immediate_vertex_count * sizeof(Textured_Vertex);
    gl->immediate_vertices         = cast(Textured_Vertex *) SwitchOpenGLAllocate(immediate_vertex_size);

    // Allocate immediate mode index array
    //
    gl->max_immediate_index_count  = params->max_immediate_index_count;
    umm immediate_index_size       = gl->max_immediate_index_count * sizeof(u32);
    gl->immediate_indices          = cast(u32 *) SwitchOpenGLAllocate(immediate_index_size);

    // Allocate command buffer memory
    //
    gl->command_buffer_size = params->command_buffer_size;
    gl->command_buffer_base = cast(u8 *) SwitchOpenGLAllocate(gl->command_buffer_size);

    // Allocate texture handles
    //
    gl->max_texture_handle_count = params->max_texture_handle_count;
    gl->texture_handles = cast(GLuint *) SwitchOpenGLAllocate(gl->max_texture_handle_count * sizeof(GLuint));

    // Allocate texture transfer buffer
    //
    Texture_Transfer_Buffer *texture_buffer = &result->texture_buffer;
    texture_buffer->size = params->texture_transfer_buffer_size;
    texture_buffer->base = cast(u8 *) SwitchOpenGLAllocate(texture_buffer->size);
    texture_buffer->used = 0;

#define EGL_LOAD_FUNCTION(name) name = (type_##name *) eglGetProcAddress(#name)
    EGL_LOAD_FUNCTION(glShaderSource);
    EGL_LOAD_FUNCTION(glCompileShader);
    EGL_LOAD_FUNCTION(glGetShaderiv);
    EGL_LOAD_FUNCTION(glGetShaderInfoLog);
    EGL_LOAD_FUNCTION(glDeleteShader);
    EGL_LOAD_FUNCTION(glAttachShader);
    EGL_LOAD_FUNCTION(glLinkProgram);
    EGL_LOAD_FUNCTION(glGetProgramiv);
    EGL_LOAD_FUNCTION(glGetProgramInfoLog);
    EGL_LOAD_FUNCTION(glDeleteProgram);
    EGL_LOAD_FUNCTION(glGenVertexArrays);
    EGL_LOAD_FUNCTION(glGenBuffers);
    EGL_LOAD_FUNCTION(glBindVertexArray);
    EGL_LOAD_FUNCTION(glBindBuffer);
    EGL_LOAD_FUNCTION(glVertexAttribPointer);
    EGL_LOAD_FUNCTION(glEnableVertexAttribArray);
    EGL_LOAD_FUNCTION(glBufferData);
    EGL_LOAD_FUNCTION(glUseProgram);
    EGL_LOAD_FUNCTION(glUniformMatrix4fv);
    EGL_LOAD_FUNCTION(glGenerateMipmap);

    EGL_LOAD_FUNCTION(glGetUniformLocation);

    EGL_LOAD_FUNCTION(glCreateShader);
    EGL_LOAD_FUNCTION(glCreateProgram);
#undef EGL_LOAD_FUNCTION

    eglSwapInterval(global_display, 1);

    if (!OpenGLInitalise(gl)) {
        return result;
    }

    result->BeginFrame = SwitchOpenGLBeginFrame;
    result->EndFrame   = SwitchOpenGLEndFrame;

    result->flags |= RendererContext_Initalised;

    return result;
}
