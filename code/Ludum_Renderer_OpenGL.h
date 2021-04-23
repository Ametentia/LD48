#if !defined(LUDUM_RENDERER_OPENGL_H_)
#define LUDUM_RENDERER_OPENGL_H_

struct OpenGL_Info {
    string renderer;
    string vendor;

    u32 version_major;
    u32 version_minor;
};

struct OpenGL_State {
    Renderer_Context renderer;

    OpenGL_Info info;

    Draw_Command_Buffer command_buffer;
    u8 *command_buffer_base;
    umm command_buffer_size;

    // @Todo(James): This only really supports the use of a single shader, make it more extensible
    //
    GLuint program;
    GLint  program_transform_loc;

    // Immediate mode handles
    //
    GLuint immediate_vao;
    GLuint immediate_vbo;
    GLuint immediate_ebo;

    u32 max_texture_handle_count;
    GLuint *texture_handles;

    Textured_Vertex *immediate_vertices;
    u32 max_immediate_vertex_count;

    u32 *immediate_indices;
    u32 max_immediate_index_count;
};

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;

#define GL_FRAGMENT_SHADER      0x8B30
#define GL_VERTEX_SHADER        0x8B31
#define GL_COMPILE_STATUS       0x8B81
#define GL_LINK_STATUS          0x8B82
#define GL_ARRAY_BUFFER         0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_BGR                  0x80E0
#define GL_BGRA                 0x80E1
#define GL_STREAM_DRAW          0x88E0
#define GL_STATIC_DRAW          0x88E4
#define GL_DYNAMIC_DRAW         0x88E8
#define GL_CLAMP_TO_EDGE        0x812F
#define GL_MULTISAMPLE          0x809D

typedef void type_glShaderSource(GLuint, GLsizei, const GLchar **, const GLint *);
typedef void type_glCompileShader(GLuint);
typedef void type_glGetShaderiv(GLuint, GLenum, GLint *);
typedef void type_glGetShaderInfoLog(GLuint, GLsizei, GLsizei *, GLchar *);
typedef void type_glDeleteShader(GLuint);
typedef void type_glAttachShader(GLuint, GLuint);
typedef void type_glLinkProgram(GLuint);
typedef void type_glGetProgramiv(GLuint, GLenum, GLint *);
typedef void type_glGetProgramInfoLog(GLuint, GLsizei, GLsizei *, GLchar *);
typedef void type_glDeleteProgram(GLuint);
typedef void type_glGenVertexArrays(GLsizei, GLuint *);
typedef void type_glGenBuffers(GLsizei, GLuint *);
typedef void type_glBindVertexArray(GLuint);
typedef void type_glBindBuffer(GLenum, GLuint);
typedef void type_glVertexAttribPointer(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *);
typedef void type_glEnableVertexAttribArray(GLuint);
typedef void type_glBufferData(GLenum, GLsizeiptr, const GLvoid *, GLenum);
typedef void type_glUseProgram(GLuint);
typedef void type_glUniformMatrix4fv(GLint, GLsizei, GLboolean, const GLvoid *);
typedef void type_glGenerateMipmap(GLenum);

typedef GLint type_glGetUniformLocation(GLuint, const GLchar *);

typedef GLuint type_glCreateShader(GLenum);
typedef GLuint type_glCreateProgram();

#define OPENGL_GLOBAL_FUNCTION(name) static type_##name *name
OPENGL_GLOBAL_FUNCTION(glShaderSource);
OPENGL_GLOBAL_FUNCTION(glCompileShader);
OPENGL_GLOBAL_FUNCTION(glGetShaderiv);
OPENGL_GLOBAL_FUNCTION(glGetShaderInfoLog);
OPENGL_GLOBAL_FUNCTION(glDeleteShader);
OPENGL_GLOBAL_FUNCTION(glAttachShader);
OPENGL_GLOBAL_FUNCTION(glLinkProgram);
OPENGL_GLOBAL_FUNCTION(glGetProgramiv);
OPENGL_GLOBAL_FUNCTION(glGetProgramInfoLog);
OPENGL_GLOBAL_FUNCTION(glDeleteProgram);
OPENGL_GLOBAL_FUNCTION(glGenVertexArrays);
OPENGL_GLOBAL_FUNCTION(glGenBuffers);
OPENGL_GLOBAL_FUNCTION(glBindVertexArray);
OPENGL_GLOBAL_FUNCTION(glBindBuffer);
OPENGL_GLOBAL_FUNCTION(glVertexAttribPointer);
OPENGL_GLOBAL_FUNCTION(glEnableVertexAttribArray);
OPENGL_GLOBAL_FUNCTION(glBufferData);
OPENGL_GLOBAL_FUNCTION(glUseProgram);
OPENGL_GLOBAL_FUNCTION(glUniformMatrix4fv);
OPENGL_GLOBAL_FUNCTION(glGenerateMipmap);

OPENGL_GLOBAL_FUNCTION(glGetUniformLocation);

OPENGL_GLOBAL_FUNCTION(glCreateShader);
OPENGL_GLOBAL_FUNCTION(glCreateProgram);
#undef OPENGL_GLOBAL_FUNCTION

#endif  // LUDUM_RENDERER_OPENGL_H_
