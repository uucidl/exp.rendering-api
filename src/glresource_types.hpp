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

class TextureResource
{
        ENFORCE_ID_OBJECT(TextureResource);

public:
        TextureResource();
        ~TextureResource();

        GLuint id;
};

class FramebufferResource
{
        ENFORCE_ID_OBJECT(FramebufferResource);

public:
        FramebufferResource();
        ~FramebufferResource();

        GLuint id;
};

class RenderbufferResource
{
        ENFORCE_ID_OBJECT(RenderbufferResource);

public:
        RenderbufferResource();
        ~RenderbufferResource();

        GLuint id;
};

class BufferResource
{
        ENFORCE_ID_OBJECT(BufferResource);
public:
        BufferResource();
        ~BufferResource();

        GLuint id;
};

class VertexArrayResource
{
        ENFORCE_ID_OBJECT(VertexArrayResource);
public:
        VertexArrayResource();
        ~VertexArrayResource();

        GLuint id;
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
