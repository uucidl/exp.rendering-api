#include "renderer.hpp"

#include "renderer_types.hpp"

#include "../src/estd.hpp"

FrameSeriesResource makeFrameSeries()
{
        return estd::make_unique<FrameSeries>();
}

namespace
{
struct ProgramBindings {
        std::vector<GLint> textureUniforms;
        std::vector<GLint> arrayAttribs;
};
}

ProgramBindings programBindings(ShaderProgramResource const& program,
                                ProgramInputs const& inputs)
{
        auto bindings = ProgramBindings {};

        std::transform(std::begin(inputs.textures),
                       std::end(inputs.textures),
                       std::back_inserter(bindings.textureUniforms),
        [&program](ProgramInputs::TextureInput const& element) {
                return glGetUniformLocation(program.id, element.name.c_str());
        });

        std::transform(std::begin(inputs.attribs),
                       std::end(inputs.attribs),
                       std::back_inserter(bindings.arrayAttribs),
        [&program](ProgramInputs::AttribArrayInput const& element) {
                return glGetAttribLocation(program.id, element.name.c_str());
        });

        return bindings;
};

void drawOne(FrameSeries& output,
             Program programDef,
             ProgramInputs inputs,
             GeometryDef geometryDef)
{
        output.beginFrame();

        // define and draw the content of the frame

        if (programDef.vertexShader.source.empty()
            || programDef.fragmentShader.source.empty()) {
                return;
        }

        auto const& program = output.program(programDef);
        withShaderProgram(program,
        [&output,&inputs,&program,&geometryDef]() {
                auto vars = programBindings(program, inputs);
                auto textureTargets = std::vector<GLenum> {};
                {
                        auto i = 0;
                        for (auto& textureInput : inputs.textures) {
                                auto unit = i;
                                auto& texture = output.texture(textureInput.content);
                                auto target = GL_TEXTURE0 + unit;

                                textureTargets.emplace_back(target);
                                OGL_TRACE;
                                glActiveTexture(target);
                                glBindTexture(GL_TEXTURE_2D, texture.id);

                                glUniform1i(vars.textureUniforms[i], unit);
                                i++;
                                OGL_TRACE;
                        }
                }

                auto const& mesh = output.mesh(geometryDef);

                // draw here
                withVertexArray(mesh.vertexArray, [&program,&vars,&mesh]() {
                        auto& vertexAttribVars = vars.arrayAttribs;

                        int i = 0;
                        for (auto attrib : vertexAttribVars) {

                                glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBuffers[i].id);
                                glVertexAttribPointer(attrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

                                glEnableVertexAttribArray(attrib);
                                i++;
                        }

                        validate(program);

                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.id);
                        glDrawElements(GL_TRIANGLES,
                                       mesh.indicesCount,
                                       GL_UNSIGNED_INT,
                                       0);

                        for (auto attrib : vertexAttribVars) {
                                glDisableVertexAttribArray(attrib);
                        }
                });

                // unbind
                for (auto target : textureTargets) {
                        glActiveTexture(target);
                        glBindTexture(GL_TEXTURE_2D, 0);
                }
                glActiveTexture(GL_TEXTURE0);
                OGL_TRACE;
        });
};
