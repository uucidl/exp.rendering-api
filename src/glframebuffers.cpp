#include "glinlines.hpp"
#include "glframebuffers.hpp"
#include "glresource_types.hpp"
#include "gltexturing.hpp"

#include <GL/glew.h>

void createFramebuffer(FramebufferResource& framebuffer,
                       TextureResource& framebufferResult,
                       RenderbufferResource& renderbuffer,
                       std::pair<int, int> resolution)
{
        if (!GLEW_EXT_framebuffer_object) {
                std::exit(1);
        }

        withTexture(framebufferResult,
                    std::bind(defineNonMipmappedARGB32Texture,
                              resolution.first, resolution.second, nullptr));

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
                clear();
        });
}
