#pragma once

#include <GL/glew.h>

#include <functional>

/**
 * Enforce identity semantics for this type name.
 *
 * an object with identity cannot be copied or moved, and must
 * instead be referenced to.
 */
#define ENFORCE_ID_OBJECT(Typename)                     \
        private:                                        \
        Typename(Typename&) = delete;                   \
        Typename(Typename&&) = delete;                  \
        Typename& operator=(Typename&) = delete;        \
        Typename& operator=(Typename&&) = delete;

#define DECLARE_RESOURCE_CLASS(Typename) \
        ENFORCE_ID_OBJECT(Typename);            \
        public:                                 \
        Typename();                             \
        ~Typename();                            \
        GLuint id;


class TextureResource
{
        DECLARE_RESOURCE_CLASS(TextureResource);
};

class FramebufferResource
{
        DECLARE_RESOURCE_CLASS(FramebufferResource);
};

class RenderbufferResource
{
        DECLARE_RESOURCE_CLASS(RenderbufferResource);
};

class BufferResource
{
        DECLARE_RESOURCE_CLASS(BufferResource);
};

class VertexArrayResource
{
        DECLARE_RESOURCE_CLASS(VertexArrayResource);
};

class ShaderProgramResource
{
        DECLARE_RESOURCE_CLASS(ShaderProgramResource);
};

class VertexShaderResource
{
        DECLARE_RESOURCE_CLASS(VertexShaderResource);
};

class FragmentShaderResource
{
        DECLARE_RESOURCE_CLASS(FragmentShaderResource);
};

void withTexture(TextureResource const& texture, std::function<void()> fn);
void withFramebuffer(FramebufferResource const& fb,
                     std::function<void ()> fn);
void withRenderbuffer(RenderbufferResource const& fb,
                      std::function<void ()> fn);
void withArrayBuffer(BufferResource const& buffer,
                     std::function<void()> fn);
void withElementBuffer(BufferResource const& buffer,
                       std::function<void()> fn);
void withVertexArray(VertexArrayResource const& vertexArray,
                     std::function<void()> fn);
