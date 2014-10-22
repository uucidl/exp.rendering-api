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

        struct ArrayAttrib {
                GLint id;
                int componentCount;
        };
        std::vector<ArrayAttrib> arrayAttribs;

        std::vector<GLint> floatVectorsUniforms;
};
}

ProgramBindings programBindings(FrameSeries::ShaderProgramMaterials const&
                                program,
                                ProgramInputs const& inputs)
{
        auto bindings = ProgramBindings {};

        std::transform(std::begin(inputs.textures),
                       std::end(inputs.textures),
                       std::back_inserter(bindings.textureUniforms),
        [&program](ProgramInputs::TextureInput const& element) {
                return glGetUniformLocation(program.programId, element.name.c_str());
        });

        std::transform(std::begin(inputs.attribs),
                       std::end(inputs.attribs),
                       std::back_inserter(bindings.arrayAttribs),
                       [&program](ProgramInputs::AttribArrayInput const& element) ->
        ProgramBindings::ArrayAttrib {
                return {
                        glGetAttribLocation(program.programId, element.name.c_str()),
                        element.componentCount
                };
        });

        std::transform(std::begin(inputs.floatValues),
                       std::end(inputs.floatValues),
                       std::back_inserter(bindings.floatVectorsUniforms),
        [&program](ProgramInputs::FloatInput const& element) {
                return glGetUniformLocation(program.programId, element.name.c_str());
        });


        return bindings;
};

void drawOne(FrameSeries& output,
             ProgramDef programDef,
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
        glUseProgram(program.programId);
        {
                auto vars = programBindings(program, inputs);
                auto textureTargets = std::vector<GLenum> {};
                {
                        auto i = 0;
                        for (auto& textureInput : inputs.textures) {
                                auto unit = i;
                                auto uniformId = vars.textureUniforms[i];
                                i++;

                                if (uniformId < 0) {
                                        continue;
                                }
                                auto const& texture = output.texture(textureInput.content);
                                auto target = GL_TEXTURE0 + unit;
                                textureTargets.emplace_back(target);
                                glActiveTexture(target);
                                glBindTexture(GL_TEXTURE_2D, texture.textureId);
                                glUniform1i(uniformId, unit);
                                OGL_TRACE;
                        }
                }

                {
                        auto i = 0;
                        for (auto& floatInput : inputs.floatValues) {
                                auto uniformId = vars.floatVectorsUniforms[i];
                                i++;

                                auto const& values = floatInput.values;
                                auto const width  = values.size();
                                auto const last_row = floatInput.last_row;
                                if (last_row == 0) {
                                        switch (width) {
                                        case 4:
                                                glUniform4fv(uniformId,
                                                             1,
                                                             &values.front());
                                                break;
                                        case 3:
                                                glUniform3fv(uniformId,
                                                             1,
                                                             &values.front());
                                                break;
                                        case 2:
                                                glUniform2fv(uniformId,
                                                             1,
                                                             &values.front());
                                                break;
                                        case 1:
                                                glUniform1fv(uniformId,
                                                             1,
                                                             &values.front());
                                                break;
                                        default:
                                                printf("invalid number of float inputs: %lu\n", width);
                                        }
                                } else if (last_row == 1) {
                                        switch (width) {
                                        case 2:
                                                glUniformMatrix2fv(uniformId,
                                                                   1,
                                                                   GL_TRUE,
                                                                   &values.front());
                                                break;
                                        case 4:
                                                glUniformMatrix4x2fv(uniformId,
                                                                     1,
                                                                     GL_TRUE,
                                                                     &values.front());
                                                break;
                                        case 3:
                                                glUniformMatrix3x2fv(uniformId,
                                                                     1,
                                                                     GL_TRUE,
                                                                     &values.front());
                                                break;
                                        default:
                                                printf("invalid number of float inputs: %lu, rows: %u\n", width, last_row);
                                        }
                                } else if (floatInput.last_row == 2) {
                                        switch (width) {
                                        case 3:
                                                glUniformMatrix3fv(uniformId,
                                                                   1,
                                                                   GL_TRUE,
                                                                   &values.front());
                                                break;
                                        case 4:
                                                glUniformMatrix4x3fv(uniformId,
                                                                     1,
                                                                     GL_TRUE,
                                                                     &values.front());
                                                break;
                                        case 2:
                                                glUniformMatrix2x3fv(uniformId,
                                                                     1,
                                                                     GL_TRUE,
                                                                     &values.front());
                                                break;
                                        default:
                                                printf("invalid number of float inputs: %lu, rows: %u\n", width, last_row);
                                        }
                                } else if (floatInput.last_row == 3) {
                                        switch (width) {
                                        case 4:
                                                glUniformMatrix4fv(uniformId,
                                                                   1,
                                                                   GL_TRUE,
                                                                   &values.front());
                                                break;
                                        case 3:
                                                glUniformMatrix3x4fv(uniformId,
                                                                     1,
                                                                     GL_TRUE,
                                                                     &values.front());
                                                break;
                                        case 2:
                                                glUniformMatrix2x4fv(uniformId,
                                                                     1,
                                                                     GL_TRUE,
                                                                     &values.front());
                                                break;
                                        default:
                                                printf("invalid number of float inputs: %lu, rows: %u\n", width, last_row);
                                        }
                                }
                                OGL_TRACE;
                        }
                }

                auto const& mesh = output.mesh(geometryDef);

                // draw here
                glBindVertexArray(mesh.vertexArray);
                {
                        auto& vertexAttribVars = vars.arrayAttribs;

                        int i = 0;
                        for (auto attrib : vertexAttribVars) {

                                glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBuffers[i]);
                                glVertexAttribPointer(attrib.id, attrib.componentCount, GL_FLOAT, GL_FALSE, 0,
                                                      0);

                                glEnableVertexAttribArray(attrib.id);
                                i++;
                        }

                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indicesBuffer);
                        glDrawElements(GL_TRIANGLES,
                                       mesh.indicesCount,
                                       GL_UNSIGNED_INT,
                                       0);

                        for (auto attrib : vertexAttribVars) {
                                glDisableVertexAttribArray(attrib.id);
                        }
                }
                glBindVertexArray(0);

                // unbind
                for (auto target : textureTargets) {
                        glActiveTexture(target);
                        glBindTexture(GL_TEXTURE_2D, 0);
                }
                glActiveTexture(GL_TEXTURE0);
                OGL_TRACE;
        }
        glUseProgram(0);
};
