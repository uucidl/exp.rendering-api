#include "framebuffer.h"

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
                glGenTextures(1, &textureid);
                glBindTexture(GL_TEXTURE_2D, textureid);
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
                                       textureid,
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


                glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        ~Impl()
        {
                glDeleteFramebuffers(1, &id);
                glDeleteTextures(1, &textureid);
        }

        TextureImpl* asTexture()
        {
                return nullptr;
        }

        void on()
        {}

        void off()
        {}

        GLuint id;
        GLuint textureid;
        GLuint depthrenderbuffer;
};

Framebuffer::Framebuffer() : impl(new Framebuffer::Impl())
{}

Framebuffer::~Framebuffer() = default;

TextureImpl* Framebuffer::asTexture() const
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
