internal void OpenGLGetInfo(OpenGL_Info *info) {
    info->renderer = WrapZ(cast(u8 *) glGetString(GL_RENDERER));
    info->vendor   = WrapZ(cast(u8 *) glGetString(GL_VENDOR));

    info->version_major = 0;
    info->version_minor = 0;

    u8 *version = cast(u8 *) glGetString(GL_VERSION);
    b32 found_point = false;
    while (version[0] != 0) {
        if (!found_point) {
            if (IsNumber(version[0])) {
                info->version_major *= 10;
                info->version_major += (version[0] - '0');
            }
            else if (version[0] == '.') {
                found_point = true;
            }
            else {
                break;
            }
        }
        else {
            if (IsNumber(version[0])) {
                info->version_minor *= 10;
                info->version_minor += (version[0] - '0');
            }
            else {
                break;
            }
        }

        version += 1;
    }
}

internal b32 OpenGLCompileProgram(GLuint *handle, const GLchar *vertex_code, const GLchar *fragment_code) {
    b32 result = false;

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_code, 0);
    glCompileShader(vertex_shader);

    s32 success = 0;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(vertex_shader, sizeof(log), 0, log);

        glDeleteShader(vertex_shader);

        return result;
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_code, 0);
    glCompileShader(fragment_shader);

    success = 0;
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(fragment_shader, sizeof(log), 0, log);

        glDeleteShader(fragment_shader);
        glDeleteShader(vertex_shader);

        return result;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), 0, log);

        glDeleteProgram(program);
    }
    else {
        *handle = program;
        result = true;
    }

    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);

    return result;
}

internal b32 OpenGLCompileSimpleProgram(OpenGL_State *gl) {
    b32 result = false;

    const GLchar *vertex_code = R"VERT(
        #version 330 core

        layout(location = 0) in vec3 position;
        layout(location = 1) in vec2 uv;
        layout(location = 2) in vec4 colour;

        out vec2 frag_uv;
        out vec4 frag_colour;

        uniform mat4 transform;

        void main() {
            gl_Position = transform * vec4(position, 1.0);

            frag_uv     = uv;
            frag_colour = colour;
        }
    )VERT";

    const GLchar *fragment_code = R"FRAG(
        #version 330 core

        in vec2 frag_uv;
        in vec4 frag_colour;

        out vec4 final_colour;

        uniform sampler2D image;

        void main() {
            final_colour = texture(image, frag_uv) * frag_colour;
        }
    )FRAG";

    result = OpenGLCompileProgram(&gl->program, vertex_code, fragment_code);
    if (result) {
        gl->program_transform_loc = glGetUniformLocation(gl->program, "transform");
        result = (gl->program_transform_loc != -1);
    }

    return result;
}

internal b32 OpenGLInitalise(OpenGL_State *gl) {
    b32 result = false;

    OpenGLGetInfo(&gl->info);

    glEnable(GL_MULTISAMPLE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glGenVertexArrays(1, &gl->immediate_vao);
    glGenBuffers(1, &gl->immediate_vbo);
    glGenBuffers(1, &gl->immediate_ebo);

    glBindVertexArray(gl->immediate_vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl->immediate_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl->immediate_ebo);

    umm stride = sizeof(Textured_Vertex);
    GLvoid *offset;

    offset = cast(GLvoid *) OffsetOf(Textured_Vertex, position);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, offset);

    offset = cast(GLvoid *) OffsetOf(Textured_Vertex, uv);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, offset);

    offset = cast(GLvoid *) OffsetOf(Textured_Vertex, colour);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, offset);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // Generate texture handles
    //
    glGenTextures(gl->max_texture_handle_count, gl->texture_handles);

    // Reserve the first texture handle as a white texture
    // @Todo: Maybe this should be on the game code side of this that requests a texture transfer of 1x1
    //
    u8 white_texture[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

    glBindTexture(GL_TEXTURE_2D, gl->texture_handles[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_BGRA, GL_UNSIGNED_BYTE, white_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);

    result = OpenGLCompileSimpleProgram(gl);
    return result;
}

// @Todo(James): Support texturing properly
//
internal GLuint OpenGLGetTextureHandle(OpenGL_State *gl, Texture_Handle handle) {
    GLuint result;

    if (handle.index < gl->max_texture_handle_count) {
        result = gl->texture_handles[handle.index];
    }
    else {
        result = gl->texture_handles[handle.index];
    }

    return result;
}

internal void OpenGLTransferTextures(OpenGL_State *gl, Texture_Transfer_Buffer *texture_buffer) {
    for (umm offset = 0; offset < texture_buffer->used;) {
        Texture_Transfer_Info *info = cast(Texture_Transfer_Info *) (texture_buffer->base + offset);
        offset += sizeof(Texture_Transfer_Info);

        if (IsValid(info->handle)) {
            GLuint handle = OpenGLGetTextureHandle(gl, info->handle);

            void *texture_data = cast(void *) (texture_buffer->base + offset);

            glBindTexture(GL_TEXTURE_2D, handle);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, info->handle.width, info->handle.height,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);

            glGenerateMipmap(GL_TEXTURE_2D);

            // :TextureParameters Allow these to be configurable
            //
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        offset += info->transfer_size;
    }

    texture_buffer->used = 0;

    glBindTexture(GL_TEXTURE_2D, 0);
}

internal Draw_Command_Buffer *OpenGLBeginFrame(OpenGL_State *gl, v2u render_size) {
    Draw_Command_Buffer *result = &gl->command_buffer;

    result->base = gl->command_buffer_base;
    result->size = gl->command_buffer_size;
    result->used = 0;

    // @Note: May differ in the future
    //
    result->setup.window_size = render_size;
    result->setup.render_size = render_size;

    result->vertices         = gl->immediate_vertices;
    result->max_vertex_count = gl->max_immediate_vertex_count;
    result->vertex_count     = 0;

    result->indices          = gl->immediate_indices;
    result->max_index_count  = gl->max_immediate_index_count;
    result->index_count      = 0;

    glScissor(0, 0, render_size.w, render_size.h);
    glViewport(0, 0, render_size.w, render_size.h);

    return result;
}

internal void OpenGLEndFrame(OpenGL_State *gl, Draw_Command_Buffer *command_buffer) {
    OpenGLTransferTextures(gl, &gl->renderer.texture_buffer);

    glBindVertexArray(gl->immediate_vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl->immediate_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl->immediate_ebo);

    umm vertex_size = command_buffer->vertex_count * sizeof(Textured_Vertex);
    umm index_size  = command_buffer->index_count  * sizeof(u32);

    glBufferData(GL_ARRAY_BUFFER,         vertex_size, command_buffer->vertices, GL_STREAM_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size,  command_buffer->indices,  GL_STREAM_DRAW);

    glUseProgram(gl->program);

    for (umm offset = 0; offset < command_buffer->used;) {
        u32 type = *cast(u32 *) (command_buffer->base + offset);
        offset += sizeof(u32);

        switch (type) {
            case DrawCommand_Draw_Command_Clear: {
                Draw_Command_Clear *command = cast(Draw_Command_Clear *) (command_buffer->base + offset);

                GLbitfield mask = GL_COLOR_BUFFER_BIT;
                if (command->depth) { mask |= GL_DEPTH_BUFFER_BIT; }

                glClearColor(command->colour.r, command->colour.g, command->colour.b, command->colour.a);
                glClear(mask);

                offset += sizeof(Draw_Command_Clear);
            }
            break;
            case DrawCommand_Draw_Command_Vertex_Batch: {
                Draw_Command_Vertex_Batch *command =
                    cast(Draw_Command_Vertex_Batch *) (command_buffer->base + offset);

                // @Speed: While this only has one texture it will be rebound every time a batch
                // is called... But as there is only one texture there should only really be one
                // batch
                //
                GLuint texture_handle = OpenGLGetTextureHandle(gl, command->texture);
                glBindTexture(GL_TEXTURE_2D, texture_handle);
                glUniformMatrix4fv(gl->program_transform_loc, 1, GL_TRUE, command->transform.m);

                const GLvoid *index_offset = cast(const GLvoid *) (command->index_offset * sizeof(u32));
                glDrawElements(GL_TRIANGLES, command->index_count, GL_UNSIGNED_INT, index_offset);

                offset += sizeof(Draw_Command_Vertex_Batch);
            }
            break;
        }
    }
}
