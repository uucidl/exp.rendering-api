#include "framebuffer.h"
#include "texture_types.h"

#include <GL/glew.h>

#include <cstdlib>

std::pair<int, int> framebufferResolution()
{
        GLint wh[4];
        glGetIntegerv(GL_VIEWPORT, wh);

        return { wh[2], wh[3] };
}

class Framebuffer::Impl
{
public:
        Impl()
        {
                if (!GLEW_EXT_framebuffer_object) {
                        exit(1);
                }

                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                auto resolution = framebufferResolution();
                glBindTexture(GL_TEXTURE_2D, texture.ref);
                glTexImage2D(GL_TEXTURE_2D,
                             0,
                             GL_RGBA,
                             resolution.first,
                             resolution.second,
                             0,
                             GL_RGBA,
                             GL_UNSIGNED_INT_8_8_8_8_REV,
                             NULL);
                glBindTexture(GL_TEXTURE_2D, 0);

                glGenFramebuffers(1, &id);

                glBindFramebuffer(GL_FRAMEBUFFER, id);
                glFramebufferTexture2D(GL_FRAMEBUFFER,
                                       GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_2D,
                                       texture.ref,
                                       0);
                glGenRenderbuffers(1, &depthrenderbuffer);

                auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                switch(status) {
                case GL_FRAMEBUFFER_COMPLETE:
                        // we're cool
                        break;
                case GL_FRAMEBUFFER_UNSUPPORTED:
                        printf("GL_FRAMEBUFFER_UNSUPPORTED");
                        exit(1);
                default:
                        printf("Unknown error %d\n", status);
                        exit(1);
                }

                glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
                                      resolution.first, resolution.second);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                          GL_RENDERBUFFER, depthrenderbuffer);
                glBindRenderbuffer(GL_RENDERBUFFER, 0);


                glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        ~Impl()
        {
                glDeleteFramebuffers(1, &id);
                glDeleteRenderbuffers(1, &depthrenderbuffer);
        }

        Texture const& asTexture()
        {
                return texture;
        }

        void on()
        {}

        void off()
        {}

        GLuint id;
        Texture texture;
        GLuint depthrenderbuffer;
};

Framebuffer::Framebuffer() : impl(new Framebuffer::Impl())
{}

Framebuffer::~Framebuffer() = default;

Texture const& Framebuffer::asTexture() const
{
        return impl->asTexture();
}

void Framebuffer::on() const
{
        impl->on();
}

void Framebuffer::off() const
{
        impl->off();
}
