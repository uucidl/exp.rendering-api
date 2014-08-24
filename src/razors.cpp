#include "razors.hpp"

#include "estd.hpp"
#include "glresource_types.hpp"

#define BEGIN_NOWARN_BLOCK \
        _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Wmissing-field-initializers\"")

#define END_NOWARN_BLOCK \
        _Pragma("clang diagnostic pop")

BEGIN_NOWARN_BLOCK

#  define STB_PERLIN_IMPLEMENTATION
#  include "stb_perlin.h"
#  undef STB_PERLIN_IMPLEMENTATION

END_NOWARN_BLOCK

#include <GL/glew.h>

#include <cstdlib>
#include <vector>

static std::pair<int, int> rootViewport()
{
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        GLint wh[4];
        glGetIntegerv(GL_VIEWPORT, wh);

        return { wh[2], wh[3] };
}

static void defineNonMipmappedTexture(int const width, int const height,
                                      std::function<void(uint32_t* pixels, int width, int height)> pixelFiller)
{
        // no mipmapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (!pixelFiller) {
                glTexImage2D(GL_TEXTURE_2D,
                             0,
                             GL_RGBA,
                             width,
                             height,
                             0,
                             GL_RGBA,
                             GL_UNSIGNED_INT_8_8_8_8_REV,
                             NULL);
                return;
        }

        std::vector<uint32_t> pixels (width * height);

        pixelFiller(&pixels.front(), width, height);

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     width,
                     height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_INT_8_8_8_8_REV,
                     &pixels.front());
}

static void createFramebuffer(FramebufferResource& framebuffer,
                              TextureResource& framebufferResult, RenderbufferResource& renderbuffer,
                              std::pair<int, int> resolution)
{
        if (!GLEW_EXT_framebuffer_object) {
                std::exit(1);
        }

        withTexture(framebufferResult,
                    std::bind(defineNonMipmappedTexture,
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

struct Geometry {
        BufferResource vertices;
        BufferResource texcoords;
        BufferResource indices;
        size_t indicesCount;

        VertexArrayResource array;
};

// as a sequence of triangles
static void define2DQuad(Geometry& geometry,
                         float x, float y,
                         float width, float height,
                         float umin, float vmin,
                         float umax, float vmax)
{
        withArrayBuffer(geometry.vertices,
        [=]() {
                float vertices[] = {
                        x, y,
                        x, y + height,
                        x + width, y + height,
                        x + width, y,
                };

                glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STREAM_DRAW);
        });

        withArrayBuffer(geometry.texcoords,
        [=]() {
                float texcoords[] = {
                        umin, vmin,
                        umin, vmax,
                        umax, vmax,
                        umax, vmin,
                };

                glBufferData(GL_ARRAY_BUFFER, sizeof texcoords, texcoords, GL_STREAM_DRAW);

        });

        GLuint indices[] = {
                0, 1, 2, 2, 3, 0,
        };

        geometry.indicesCount = sizeof indices / sizeof indices[0];

        withElementBuffer(geometry.indices,
        [=]() {
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof indices, indices,
                             GL_STREAM_DRAW);
        });
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

static void perlin_noise(uint32_t* data, int width, int height)
{
        for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                        uint8_t values[4];
                        for (size_t i = 0; i < sizeof values / sizeof values[0]; i++) {
                                float val = 0.5 + stb_perlin_noise3(13.0 * x / width, 17.0 * y / height, 0.0);
                                if (val > 1.0) {
                                        val = 1.0;
                                } else if (val < 0.0) {
                                        val = 0.0;
                                }

                                values[i] = (int) (255 * val) & 0xff;
                        }


                        data[x + y*width] = (values[0] << 24)
                                            | (values[1] << 16)
                                            | (values[2] << 8)
                                            | values[3];
                }
        }
}

static void seed()
{
        static struct Seed {
                Seed()
                {
                        auto resolution = rootViewport();

                        withTexture(texture,
                                    std::bind(defineNonMipmappedTexture,
                                              resolution.first, resolution.second,
                                              perlin_noise));

                        define2DQuad(quad, -1.0, -1.0, 2.0, 2.0, 0.0, 0.0, 1.0, 1.0);
                };

                TextureResource texture;
                Geometry quad;
        } all;
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
