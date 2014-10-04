#include <GL/glew.h>
#include <string>

#include "main_types.hpp"
#include "quad.hpp"
#include "../src/gldebug.hpp"
#include "../src/glresource_types.hpp"
#include "../src/glshaders.hpp"
#include "../src/gltexturing.hpp"

namespace
{
struct SimpleShaderProgram : public ShaderProgramResource {
        VertexShaderResource vertexShader;
        FragmentShaderResource fragmentShader;
};
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
}

struct MainShaderVariables {
        GLint textureUniforms[2];
        GLint positionAttrib;
        GLint texcoordAttrib;
};

static MainShaderVariables getMainShaderVariables(ShaderProgramResource const&
                program)
{
        auto const id = program.id;
        MainShaderVariables variables = {
                {
                        glGetUniformLocation(id, "tex0"),
                        glGetUniformLocation(id, "tex1")
                },
                glGetAttribLocation(program.id, "position"),
                glGetAttribLocation(program.id, "texcoord")
        };

        return variables;
}

static void define2dTrianglesVertexArray(BufferResource const& indices,
                GLint positionAttrib, BufferResource const& positions,
                GLvoid* positionsOffset,
                GLint texcoordAttrib, BufferResource const& texcoords,
                GLvoid* texcoordsOffset)
{
        glBindBuffer(GL_ARRAY_BUFFER, texcoords.id);
        glVertexAttribPointer(texcoordAttrib, 2, GL_FLOAT, GL_FALSE, 0,
                              texcoordsOffset);

        glBindBuffer(GL_ARRAY_BUFFER, positions.id);
        glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, 0,
                              positionsOffset);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices.id);
        glEnableVertexAttribArray(texcoordAttrib);
        glEnableVertexAttribArray(positionAttrib);
}

void render_textured_quad_v1(uint64_t time_micros)
{
        static Tasks tasks;
        static RootDirFileSystem codeBaseFS { dirname(__FILE__) };

        static struct Resources {
                Resources() :
                        fileLoader(makeFileLoader(codeBaseFS, tasks))
                {
                        {
                                quadIndicesOffset = 0;
                                quadIndicesCount = define2dQuadIndices(indices);
                                quadPositionOffset = define2dQuadBuffer(vertices, -1.0, -1.0, 2.0, 2.0);
                                quadTexcoordsOffset = define2dQuadBuffer(texcoords, 0.0, 0.0, 1.0, 1.0);
                        }

                        loadFilePair(*fileLoader.get(), "main.vs",
                        "main.fs", [=](std::string const& contentVS, std::string const& contentFS) {
                                defineProgram(mainShader, contentVS, contentFS);
                                mainShaderVars = getMainShaderVariables(mainShader);

                                withShaderProgram(mainShader, [=]() {
                                        // bind to texture units 0 and 1
                                        glUniform1i(mainShaderVars.textureUniforms[0], 0);
                                        glUniform1i(mainShaderVars.textureUniforms[1], 1);
                                });

                                withVertexArray(vertexArray, [=]() {
                                        define2dTrianglesVertexArray
                                        (indices,
                                         mainShaderVars.positionAttrib, vertices, quadPositionOffset,
                                         mainShaderVars.texcoordAttrib, texcoords, quadTexcoordsOffset);
                                        validate(mainShader);
                                });
                        });

                        {
                                int const width = 8;
                                int const height = 8;

                                withTexture(quadTexture,
                                []() {
                                        defineNonMipmappedARGB32Texture(width, height, perlinNoisePixelFiller);
                                });
                        }
                }

                SimpleShaderProgram mainShader;
                MainShaderVariables mainShaderVars;
                FileLoaderResource fileLoader;

                TextureResource quadTexture;
                VertexArrayResource vertexArray;
                BufferResource indices;
                BufferResource vertices;
                BufferResource texcoords;

                GLvoid* quadIndicesOffset;
                size_t quadIndicesCount;
                GLvoid* quadPositionOffset;
                GLvoid* quadTexcoordsOffset;
        } resources;

        tasks.run();

        auto const& program = resources.mainShader;
        if (program.id > 0) {
                withShaderProgram(program, []() {
                        withTexture(resources.quadTexture, []() {
                                withVertexArray(resources.vertexArray, []() {
                                        glDrawElements(GL_TRIANGLES, resources.quadIndicesCount, GL_UNSIGNED_INT,
                                                       resources.quadIndicesOffset);
                                });
                        });
                });
        }
}

