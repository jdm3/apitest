#pragma once

#include "sparse_bindless_texarray.h"

class Texture2D;

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
class Textures_GL_Sparse_Bindless_Texture_Array : public Textures
{
public:
    Textures_GL_Sparse_Bindless_Texture_Array();
    virtual ~Textures_GL_Sparse_Bindless_Texture_Array();

    virtual bool init() override;

    virtual bool begin(void* hwnd, GfxSwapChain* swap_chain, GfxFrameBuffer* frame_buffer) override;
    virtual void end(GfxSwapChain* swap_chain) override;

    virtual void draw(Matrix* transforms, int count) override;

private:
    struct Vertex
    {
        Vec3 pos;
        Vec2 tex;
    };

    GLuint m_ib;
    GLuint m_vb_pos;
    GLuint m_vb_tex;
    GLuint m_vs;
    GLuint m_fs;
    GLuint m_prog;
    GLuint m_texbuf;

    GLuint m_transform_buffer;

    Texture2D* mTex1;
    Texture2D* mTex2;

    DrawElementsIndirectCommand m_commands[TEXTURES_COUNT];

    TextureManager mTexManager;
};