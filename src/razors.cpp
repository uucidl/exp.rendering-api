#include "razors.hpp"

#include "estd.hpp"
#include "glresource_types.hpp"

#include <GL/glew.h>

#include <cstdlib>

static std::pair<int, int> rootViewport()
{
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        GLint wh[4];
        glGetIntegerv(GL_VIEWPORT, wh);

        return { wh[2], wh[3] };
}

static void defineStandardNonMipmappedTexture(int const width,
                int const height)
{
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     width,
                     height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_INT_8_8_8_8_REV,
                     NULL);
        // no mipmapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

static void createFramebuffer(FramebufferResource& framebuffer,
                              TextureResource& framebufferResult, RenderbufferResource& renderbuffer,
                              std::pair<int, int> resolution)
{
        if (!GLEW_EXT_framebuffer_object) {
                std::exit(1);
        }

        withTexture(framebufferResult,
                    std::bind(defineStandardNonMipmappedTexture,
                              resolution.first, resolution.second));

        withFramebuffer(framebuffer,
        [&framebufferResult,&renderbuffer,resolution]() {
                glFramebufferTexture2D(GL_FRAMEBUFFER,
                                       GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_2D,
                                       framebufferResult.id,
                                       0);

                auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                switch(status) {
                case GL_FRAMEBUFFER_COMPLETE:
                        // we're cool
                        break;
                case GL_FRAMEBUFFER_UNSUPPORTED:
                        printf("GL_FRAMEBUFFER_UNSUPPORTED");
                        std::exit(1);
                default:
                        printf("Unknown error %d\n", status);
                        std::exit(1);
                }

                withRenderbuffer(renderbuffer,
                [resolution]() {
                        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
                                              resolution.first, resolution.second);
                });
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                          GL_RENDERBUFFER, renderbuffer.id);
        });
}

struct Framebuffer {
        Framebuffer(int desiredWidth, int desiredHeight)
        {
                createFramebuffer(output,
                                  result,
                                  depthbuffer,
                { desiredWidth, desiredHeight });
                width = desiredWidth;
                height = desiredHeight;
        }

        FramebufferResource output;
        TextureResource result;
        RenderbufferResource depthbuffer;
        int width;
        int height;
};

static void withOutputTo(Framebuffer const& framebuffer,
                         std::function<void()> draw)
{
        auto resolution = rootViewport();
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.output.id);
        glDrawBuffer (GL_COLOR_ATTACHMENT0);
        glReadBuffer (GL_COLOR_ATTACHMENT0);
        glViewport (0, 0, framebuffer.width, framebuffer.height);

        draw();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glReadBuffer (GL_BACK);
        glDrawBuffer (GL_BACK);
        glViewport(0, 0, resolution.first, resolution.second);
}

class Razors
{
public:
        Razors(std::pair<int, int> resolution = rootViewport()) :
                previousFrame(resolution.first, resolution.second),
                resultFrame(resolution.first, resolution.second)
        {}

        Framebuffer previousFrame;
        Framebuffer resultFrame;
};

static void projectFramebuffer(Framebuffer const& source)
{
}

static void seed()
{
}

RazorsResource makeRazors()
{
        return estd::make_unique<Razors>();
}

void draw(Razors& self)
{
        withOutputTo(self.previousFrame,
        [&self] () {
                projectFramebuffer(self.resultFrame);
        });

        withOutputTo(self.resultFrame,
        [&self] () {
                projectFramebuffer(self.previousFrame);
                seed();
        });
        projectFramebuffer(self.resultFrame);
}
