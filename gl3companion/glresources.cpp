#include "glresource_types.hpp"

TextureResource::TextureResource() : id(0)
{
        glGenTextures(1, &id);
}

TextureResource::~TextureResource()
{
        glDeleteTextures(1, &id);
}

FramebufferResource::FramebufferResource() : id(0)
{
        glGenFramebuffers(1, &id);
}

FramebufferResource::~FramebufferResource()
{
        glDeleteFramebuffers(1, &id);
}

RenderbufferResource::RenderbufferResource() : id(0)
{
        glGenRenderbuffers(1, &id);
}

RenderbufferResource::~RenderbufferResource()
{
        glDeleteRenderbuffers(1, &id);
}

BufferResource::BufferResource()
{
        glGenBuffers(1, &id);
}

BufferResource::~BufferResource()
{
        glDeleteBuffers(1, &id);
}

VertexArrayResource::VertexArrayResource()
{
        glGenVertexArrays(1, &id);
}

VertexArrayResource::~VertexArrayResource()
{
        glDeleteVertexArrays(1, &id);
};

ShaderProgramResource::ShaderProgramResource()
{
        id = glCreateProgram();
}

ShaderProgramResource::~ShaderProgramResource()
{
        glDeleteProgram(id);
};

VertexShaderResource::VertexShaderResource()
{
        id = glCreateShader(GL_VERTEX_SHADER);
};

VertexShaderResource::~VertexShaderResource()
{
        glDeleteShader(id);
}

FragmentShaderResource::FragmentShaderResource()
{
        id = glCreateShader(GL_FRAGMENT_SHADER);
};

FragmentShaderResource::~FragmentShaderResource()
{
        glDeleteShader(id);
}

void withTexture(TextureResource const& texture,
                 std::function<void()> fn)
{
        glBindTexture(GL_TEXTURE_2D, texture.id);
        fn();
        glBindTexture(GL_TEXTURE_2D, 0);
}

void withFramebuffer(FramebufferResource const& fb,
                     std::function<void ()> fn)
{
        glBindFramebuffer(GL_FRAMEBUFFER, fb.id);
        fn();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void withRenderbuffer(RenderbufferResource const& fb,
                      std::function<void()> fn)
{
        glBindRenderbuffer(GL_RENDERBUFFER, fb.id);
        fn();
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void withArrayBuffer(BufferResource const& buffer,
                     std::function<void()> fn)
{
        glBindBuffer(GL_ARRAY_BUFFER, buffer.id);
        fn();
        glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void withElementBuffer(BufferResource const& buffer,
                       std::function<void()> fn)
{
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.id);
        fn();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void withVertexArray(VertexArrayResource const& vertexArray,
                     std::function<void()> fn)
{
        glBindVertexArray(vertexArray.id);
        fn();
        glBindVertexArray(0);
}

void withShaderProgram(ShaderProgramResource const& program,
                       std::function<void()> fn)
{
        glUseProgram(program.id);
        fn();
        glUseProgram(0);
}
