#include "pch.h"

#include "streaming_vb_gl.h"

#define GL_USE_MAP 1
#define GL_USE_PERSISTENT_MAP 1

StreamingVB_GL::StreamingVB_GL()
    : m_ub()
    , m_vs()
    , m_fs()
    , m_prog()
    , m_dyn_vb()
    , m_dyn_offset()
    , m_dyn_vb_ptr()
{}

StreamingVB_GL::~StreamingVB_GL()
{
#if GL_USE_PERSISTENT_MAP
    glBindBuffer(GL_ARRAY_BUFFER, m_dyn_vb);
    glUnmapBuffer(GL_ARRAY_BUFFER);
#endif
    glDeleteBuffers(1, &m_dyn_vb);

    glDeleteBuffers(1, &m_ub);
    glDeleteShader(m_vs);
    glDeleteShader(m_fs);
    glDeleteProgram(m_prog);
}

bool StreamingVB_GL::init()
{
    // Uniform Buffer
    glGenBuffers(1, &m_ub);

    // Shaders
    if (!create_shader(GL_VERTEX_SHADER, "streaming_vb_gl_vs.glsl", &m_vs))
        return false;

    if (!create_shader(GL_FRAGMENT_SHADER, "streaming_vb_gl_fs.glsl", &m_fs))
        return false;

    if (!compile_program(&m_prog, m_vs, m_fs, 0))
        return false;

    // Dynamic vertex buffer
    glGenBuffers(1, &m_dyn_vb);
    glBindBuffer(GL_ARRAY_BUFFER, m_dyn_vb);
#if GL_USE_PERSISTENT_MAP
    GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    glBufferStorage(GL_ARRAY_BUFFER, DYN_VB_SIZE, NULL, flags);
    m_dyn_vb_ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, DYN_VB_SIZE, flags); 
#else
    glBufferData(GL_ARRAY_BUFFER, DYN_VB_SIZE, nullptr, GL_DYNAMIC_DRAW);
#endif

    return glGetError() == GL_NO_ERROR;
}

bool StreamingVB_GL::begin(GfxSwapChain* swap_chain, GfxFrameBuffer* frame_buffer)
{
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(swap_chain->wnd, &width, &height);

    // Bind and clear frame buffer
    int fbo = PTR_AS_INT(frame_buffer);
    float c[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
    float d = 1.0f;
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glClearBufferfv(GL_COLOR, 0, c);
    glClearBufferfv(GL_DEPTH, 0, &d);

    // Program
    glUseProgram(m_prog);

    // Uniforms
    Constants cb;
    cb.width = 2.0f / width;
    cb.height = -2.0f / height;

    glBindBuffer(GL_UNIFORM_BUFFER, m_ub);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Constants), &cb, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ub);

    // Input Layout
    glEnableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, m_dyn_vb);
    glVertexAttribPointer(0, 2, GL_FLOAT, FALSE, sizeof(VertexPos2), (void*)offsetof(VertexPos2, x));

    // Rasterizer State
    glDisable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, width, height);

    // Blend State
    glDisable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // Depth Stencil State
    glDisable(GL_DEPTH_TEST);
    glDepthMask(0);

    return true;
}

void StreamingVB_GL::end(GfxSwapChain* swap_chain)
{
    SDL_GL_SwapWindow(swap_chain->wnd);
#if defined(_DEBUG)
    GLenum error = glGetError();
    assert(error == GL_NO_ERROR);
#endif
}

void StreamingVB_GL::draw(VertexPos2* vertices, int count)
{
    int stride = sizeof(VertexPos2);
    int vertex_offset = (m_dyn_offset + stride - 1) / stride;
    int byte_offset = vertex_offset * stride;
    int size = count * stride;

#if GL_USE_PERSISTENT_MAP
    if (byte_offset + size > DYN_VB_SIZE)
    {
        vertex_offset = byte_offset = 0;
    }

    m_dyn_offset = byte_offset + size;
    void* dst = (unsigned char*)m_dyn_vb_ptr + byte_offset; 

    memcpy(dst, vertices, count * sizeof(VertexPos2));

#elif GL_USE_MAP
    GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
    if (byte_offset + size > DYN_VB_SIZE)
    {
        access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
        vertex_offset = byte_offset = 0;
    }

    m_dyn_offset = byte_offset + size;
    void* dst = glMapBufferRange(GL_ARRAY_BUFFER, byte_offset, size, access);
    if (!dst)
        return;

    memcpy(dst, vertices, count * sizeof(VertexPos2));

    glUnmapBuffer(GL_ARRAY_BUFFER);
#else
    if (byte_offset + size > DYN_VB_SIZE)
    {
        glBufferData(GL_ARRAY_BUFFER, DYN_VB_SIZE, nullptr, GL_DYNAMIC_DRAW);
        vertex_offset = byte_offset = 0;
    }

    glBufferSubData(GL_ARRAY_BUFFER, byte_offset, size, vertices);
    m_dyn_offset = byte_offset + size;
#endif

    glDrawArrays(GL_TRIANGLES, vertex_offset, count);
}