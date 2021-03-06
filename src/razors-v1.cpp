#include "razors.hpp"

#include "../gl3companion/gldebug.hpp"
#include "../gl3companion/glframebuffers.hpp"
#include "../gl3companion/glinlines.hpp"
#include "../gl3companion/glresource_types.hpp"
#include "../gl3companion/glshaders.hpp"
#include "../gl3companion/gltexturing.hpp"
#include "compiler.hpp"
#include "estd.hpp"
#include "razors-common.hpp"

#include <GL/glew.h>

#include <memory>
#include <string>
#include <cmath>

static const double TAU =
        6.28318530717958647692528676655900576839433879875021;

struct Texture : public TextureResource {
        GLint target;
};

static void withTexture(Texture const& texture, std::function<void()> fn)
{
        glBindTexture(texture.target, texture.id);
        fn();
        glBindTexture(texture.target, 0);
}

struct Framebuffer {
        Framebuffer(int desiredWidth, int desiredHeight)
        {
                createImageCaptureFramebuffer(output,
                                              result,
                                              depthbuffer,
                { desiredWidth, desiredHeight });

                result.target = GL_TEXTURE_2D;

                width = desiredWidth;
                height = desiredHeight;
        }

        FramebufferResource output;
        Texture result;
        RenderbufferResource depthbuffer;
        int width;
        int height;
};

struct Geometry {
        size_t indicesCount;
        BufferResource indices;
        BufferResource vertices;
        BufferResource texcoords;
};

// as a sequence of triangles
static void define2dQuadTriangles(Geometry& geometry,
                                  float x, float y,
                                  float width, float height,
                                  float umin, float vmin,
                                  float umax, float vmax)
{
        GLuint indices[] = {
                0, 1, 2, 2, 3, 0,
        };

        geometry.indicesCount = sizeof indices / sizeof indices[0];

        withElementBuffer(geometry.indices,
        [&indices]() {
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof indices, indices,
                             GL_STREAM_DRAW);
        });

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
}

class Razors
{
public:
        Razors(std::pair<int, int> resolution = viewport()) :
                previousFrame(resolution.first, resolution.second),
                resultFrame(resolution.first, resolution.second)
        {}

        Framebuffer previousFrame;
        Framebuffer resultFrame;
};

struct RenderingProgram {
        GLuint programId;
        GLsizei elementCount;
        VertexArrayResource array;
};

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
                          Texture const& texture)
{
        withTexture(texture,
        [&primitive]() {
                auto const program = primitive.programId;
                glUseProgram(program);

                auto const resolutionLoc = glGetUniformLocation(program, "iResolution");
                if (resolutionLoc) {
                        GLint wh[4];
                        glGetIntegerv(GL_VIEWPORT, wh);
                        glUniform3f(resolutionLoc,
                                    glfloat(wh[2]), glfloat(wh[3]), 0.0f);
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

static void defineProgram(SimpleShaderProgram& program,
                          std::string const& vertexShaderSource,
                          std::string const& fragmentShaderSource)
{
        compile(program.vertexShader, vertexShaderSource);
        compile(program.fragmentShader, fragmentShaderSource);
        link(program, program.vertexShader, program.fragmentShader);

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

static void seed(float maxAlpha=0.06f)
{
        static int const textureN = 12;
        static struct Seed {
                Seed()
                {
                        auto resolution = viewport();
                        auto txWidth = nextPowerOfTwo(resolution.first)/2;
                        auto txHeight = nextPowerOfTwo(resolution.second)/2;

                        texture.target = GL_TEXTURE_3D;
                        withTexture(texture,
                        [=]() {
                                defineNonMipmappedARGB32Texture3d
                                (txWidth, txHeight, textureN,
                                 seed_texture);
                        });

                        define2dQuadTriangles(quadTris, -1.0, -1.0, 2.0, 2.0, 0.0, 0.0, 1.0, 1.0);
                        defineProgram(program, seedVS, seedFS);

                        auto depthLoc = glGetUniformLocation(program.id, "depth");
                        withShaderProgram(program, [=]() {
                                glUniform1f(depthLoc, 0.0f);
                        });

                        defineRenderingProgram(texturedQuad, program, quadTris);
                };

                Texture texture;
                Geometry quadTris;
                SimpleShaderProgram program;
                RenderingProgram texturedQuad;
        } all;

        if (all.program.id > 0) {
                static int i = 0;

                withPremultipliedAlphaBlending
                ([&] () {
                        auto& program = all.program;
                        auto colorLoc = glGetUniformLocation(program.id, "g_color");
                        auto depthLoc = glGetUniformLocation(program.id, "depth");

                        auto period = 121.0;
                        auto phase = (1.0 + cos(TAU * (float) i / period)) / 2.0;

                        withShaderProgram(program, [=]() {
                                auto const alpha = maxAlpha;
                                glUniform4f(colorLoc,
                                            glfloat(alpha),
                                            glfloat(alpha),
                                            glfloat(alpha),
                                            glfloat(alpha));
                                glUniform1f(depthLoc, phase);

                        });
                        drawTriangles(all.texturedQuad, all.texture);
                });

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
        auto resolution = viewport();
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
                        define2dQuadTriangles(quadGeometry,  -1.0, -1.0, 2.0, 2.0, 0.0, 0.0, 1.0,
                                              1.0);
                        defineProgram(program, defaultVS, projectorFS);

                        defineRenderingProgram(texturedQuad, program, quadGeometry);
                }

                Geometry quadGeometry;
                SimpleShaderProgram program;
                RenderingProgram texturedQuad;
        } all;

        if (all.program.id > 0) {
                // respect source projector's aspect ratio
                float const yfactor = glfloat(source.height) / glfloat(source.width);
                define2dQuadTriangles(all.quadGeometry,
                                      -1.0f, -yfactor, 2.0f, 2.0f * yfactor,
                                      0.0f, 0.0f, 1.0f, 1.0f);

                GLint colorLoc = glGetUniformLocation(all.program.id, "g_color");
                GLint transformLoc = glGetUniformLocation(all.program.id, "transform");

                // scale to screen
                auto resolution = viewport();
                float const targetYFactor = glfloat(resolution.second) / glfloat(
                                                    resolution.first);

                float idmatrix[4*4] = {
                        scale, 0.01f, 0.0f, 0.0f,
                        -0.01f, scale / targetYFactor, 0.0f, 0.0f,
                        0.0f, 0.0f, scale, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f,
                };
                withShaderProgram
                (all.program, [transformLoc,colorLoc,&idmatrix]() {
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
                projectFramebuffer(self.resultFrame,
                                   glfloat(0.990f + 0.010f * sin(TAU * ms / 5000.0)));
                seed();
        });

        withOutputTo(self.resultFrame,
        [&self] () {
                clear();
                projectFramebuffer(self.previousFrame, 1.004f);
        });

        clear();
        projectFramebuffer(self.resultFrame);
}
