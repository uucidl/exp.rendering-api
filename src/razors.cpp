#include "razors.hpp"

#include "estd.hpp"
#include "glresource_types.hpp"
#include "gldebug.hpp"

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
#include <memory>
#include <sstream>
#include <string>
#include <vector>

static const double TAU =
        6.28318530717958647692528676655900576839433879875021;

static std::pair<int, int> rootViewport()
{
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        GLint wh[4];
        glGetIntegerv(GL_VIEWPORT, wh);

        return { wh[2], wh[3] };
}

static void clear()
{
        glClearColor (0.0, 0.0, 0.0, 0.0);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void defineNonMipmappedARGB32Texture(int const width, int const height,
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

struct Geometry {
        BufferResource vertices;
        BufferResource texcoords;
        BufferResource indices;
        size_t indicesCount;
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

static void perlin_noise(uint32_t* data, int width, int height,
                         float zplane=0.0f)
{
        for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                        uint8_t values[4];
                        float alpha;
                        for (size_t i = 0; i < sizeof values / sizeof values[0]; i++) {
                                float val = 0.5 + stb_perlin_noise3(13.0 * x / width, 17.0 * y / height,
                                                                    zplane);
                                if (i == 0) {
                                        alpha = val;
                                } else {
                                        val *= alpha;
                                }

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

struct RenderingProgram {
        GLuint programId;
        GLsizei elementCount;
        VertexArrayResource array;
};

static void validate(ShaderProgramResource const& program)
{
        glValidateProgram (program.id);
        GLint status;
        glGetProgramiv (program.id, GL_VALIDATE_STATUS, &status);
        if (status == GL_FALSE) {
                GLint length;
                glGetProgramiv (program.id, GL_INFO_LOG_LENGTH, &length);

                std::vector<char> pinfo;
                pinfo.reserve(length + 1);

                glGetProgramInfoLog (program.id, length, &length, &pinfo.front());

                printf ("ERROR: validating program [%s]\n", &pinfo.front());
        }
}

static void defineRenderingProgram(RenderingProgram& renderingProgram,
                                   ShaderProgramResource const& program, Geometry const& geometry)
{
        auto const programId = program.id;
        renderingProgram.programId = programId;
        renderingProgram.elementCount = geometry.indicesCount;

        withVertexArray(renderingProgram.array,
        [&program,&geometry,programId]() {
                GLuint const position_attrib = glGetAttribLocation(program.id, "position");
                GLuint const texcoord_attrib = glGetAttribLocation(program.id, "texcoord");

                glBindBuffer(GL_ARRAY_BUFFER, geometry.texcoords.id);
                glVertexAttribPointer(texcoord_attrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

                glBindBuffer(GL_ARRAY_BUFFER, geometry.vertices.id);
                glVertexAttribPointer(position_attrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.indices.id);

                glEnableVertexAttribArray(position_attrib);
                glEnableVertexAttribArray(texcoord_attrib);

                validate(program);
        });
}

static void drawTriangles(RenderingProgram const& primitive,
                          TextureResource const& texture)
{
        withTexture(texture,
        [&primitive]() {
                auto const program = primitive.programId;
                glUseProgram(program);

                auto const resolutionLoc = glGetUniformLocation(program, "iResolution");
                if (resolutionLoc) {
                        GLint wh[4];
                        glGetIntegerv(GL_VIEWPORT, wh);
                        glUniform3f(resolutionLoc, wh[2], wh[3], 0.0f);
                }

                withVertexArray(primitive.array, [&primitive]() {
                        glDrawElements(GL_TRIANGLES, primitive.elementCount, GL_UNSIGNED_INT, 0);
                });
                glUseProgram(0);
        });
}

struct SimpleShaderProgram : public ShaderProgramResource {
        VertexShaderResource vertexShader;
        FragmentShaderResource fragmentShader;
};

class SplitLines
{
public:
        SplitLines(std::string const& source)
        {
                std::stringstream ss {source};
                std::string item;
                while (getline(ss, item, '\n')) {
                        lines.push_back(item + "\n");
                }
                for (auto const& str : lines) {
                        cstrs.emplace_back(str.c_str());
                }
        }

        std::vector<std::string const> lines;
        std::vector<char const*> cstrs;
};

template <typename ResourceType>
void innerCompile(ResourceType const& shader, std::string const& source)
{
        SplitLines lines (source);
        glShaderSource(shader.id, lines.cstrs.size(), &lines.cstrs.front(), NULL);
        glCompileShader(shader.id);

        GLint status;
        glGetShaderiv (shader.id, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
                GLint length;
                glGetShaderiv (shader.id, GL_INFO_LOG_LENGTH, &length);

                std::vector<char> sinfo;
                sinfo.reserve(length + 1);
                glGetShaderInfoLog(shader.id, length, &length, &sinfo.front());

                printf ("ERROR compiling shader [%s] with source [\n", &sinfo.front());
                printf ("%s", source.c_str());
                printf ("]\n");
        }
}

static void compile(VertexShaderResource const& shader,
                    std::string const& source)
{
        return innerCompile(shader, source);
}

static void compile(FragmentShaderResource const& shader,
                    std::string const& source)
{
        return innerCompile(shader, source);
}

static void defineProgram(SimpleShaderProgram& program,
                          std::string const& vertexShaderSource,
                          std::string const& fragmentShaderSource)
{
        compile(program.vertexShader, vertexShaderSource);
        compile(program.fragmentShader, fragmentShaderSource);

        glAttachShader(program.id, program.vertexShader.id);
        glAttachShader(program.id, program.fragmentShader.id);
        glLinkProgram(program.id);

        withShaderProgram(program,
        [&program]() {
                GLint textureLoc = glGetUniformLocation(program.id, "tex");
                GLint colorLoc = glGetUniformLocation(program.id, "g_color");
                GLint transformLoc = glGetUniformLocation(program.id, "transform");

                float idmatrix[4*4] = {
                        1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f,
                };
                glUniformMatrix4fv(transformLoc, 1, GL_FALSE, idmatrix);
                glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);

                glUniform1i(textureLoc, 0); // bind to texture unit 0
        });
}

#include "inlineshaders.hpp"

static void withPremultipliedAlphaBlending(std::function<void()> fn)
{
        glEnable(GL_BLEND);
        glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDisable (GL_DEPTH_TEST);
        glDepthMask (GL_FALSE);

        fn();

        glDepthMask (GL_TRUE);
        glEnable (GL_DEPTH_TEST);
        glDisable(GL_BLEND);
}

static int nextPowerOfTwo(int number)
{
        return static_cast<int> (pow(2.0, ceil(log2(number))));
}

static void seed()
{
        static struct Seed {
                Seed()
                {
                        auto resolution = rootViewport();

                        auto plane = 0.0f;
                        for (auto const& texture : textures) {
                                withTexture(texture,
                                [&]() {
                                        defineNonMipmappedARGB32Texture
                                        (nextPowerOfTwo(resolution.first),
                                         nextPowerOfTwo(resolution.second),
                                        [=](uint32_t* pixels, int width, int height) {
                                                perlin_noise(pixels, width, height, plane);
                                        });
                                });
                                plane += 0.9f;
                        }

                        define2DQuad(quadGeometry, -1.0, -1.0, 2.0, 2.0, 0.0, 0.0, 1.0, 1.0);
                        defineProgram(program, defaultVS, defaultFS);

                        defineRenderingProgram(texturedQuad, program, quadGeometry);
                };

                TextureResource textures[4];
                Geometry quadGeometry;
                SimpleShaderProgram program;
                RenderingProgram texturedQuad;
        } all;

        if (all.program.id > 0) {
                static int i = 0;

                withPremultipliedAlphaBlending
                ([&] () {
                        auto colorLoc = glGetUniformLocation(all.program.id, "g_color");
                        auto period = 24;
                        withShaderProgram(all.program, [=]() {
                                auto origin = period*(i/period);
                                auto maxAlpha = 0.09f;
                                auto const alpha = maxAlpha * sin(TAU * (i - origin)/(2.0 * period));
                                glUniform4f(colorLoc, alpha*1.0f, alpha*1.0f, alpha*1.0f, alpha);
                        });
                        drawTriangles(all.texturedQuad, all.textures[i/10 % 4]);
                });
                OGL_TRACE;

                i++;
        }
}

RazorsResource makeRazors()
{
        return estd::make_unique<Razors>();
}

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

static void projectFramebuffer(Framebuffer const& source,
                               float const scale = 1.0f)
{
        static struct Projector {
                Projector ()
                {
                        define2DQuad(quadGeometry,  -1.0, -1.0, 2.0, 2.0, 0.0, 0.0, 1.0, 1.0);
                        defineProgram(program, defaultVS, projectorFS);

                        defineRenderingProgram(texturedQuad, program, quadGeometry);
                }

                Geometry quadGeometry;
                SimpleShaderProgram program;
                RenderingProgram texturedQuad;
        } all;

        if (all.program.id > 0) {
                GLint colorLoc = glGetUniformLocation(all.program.id, "g_color");
                GLint transformLoc = glGetUniformLocation(all.program.id, "transform");

                float idmatrix[4*4] = {
                        scale, 0.01f, 0.0f, 0.0f,
                        -0.01f, scale, 0.0f, 0.0f,
                        0.0f, 0.0f, scale, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f,
                };
                withShaderProgram
                (all.program, [=]() {
                        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, idmatrix);
                        auto const alpha = 0.9998f;
                        glUniform4f(colorLoc, alpha*1.0f, alpha*1.0f, alpha*1.0f, alpha);
                });
                drawTriangles(all.texturedQuad, source.result);
        }
}

void draw(Razors& self, double ms)
{
        OGL_TRACE;

        withOutputTo(self.previousFrame,
        [&self,ms] () {
                clear();
                projectFramebuffer(self.resultFrame, 0.990f + 0.010 * sin(TAU * ms / 5000.0));
        });

        withOutputTo(self.resultFrame,
        [&self] () {
                clear();
                projectFramebuffer(self.previousFrame);
                seed();
        });

        clear();
        projectFramebuffer(self.resultFrame);
}
