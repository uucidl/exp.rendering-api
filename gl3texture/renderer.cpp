#include "renderer.hpp"

#include "renderer_types.hpp"

#include "../gl3companion/glinlines.hpp"
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
        std::vector<GLint> intVectorsUniforms;
};

struct FragmentOperationsScope {
        FragmentOperationsScope(FragmentOperationsDef const& def)
        {
                if (def.flags & FragmentOperationsDef::CLEAR) {
                        glClearColor (def.clearRGBA[0],
                                      def.clearRGBA[0],
                                      def.clearRGBA[0],
                                      def.clearRGBA[0]);
                        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                }

                if (def.flags &
                    FragmentOperationsDef::BLEND_PREMULTIPLIED_ALPHA) {
                        glEnable(GL_BLEND);
                        glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                        glDisable (GL_DEPTH_TEST);
                        glDepthMask (GL_FALSE);
                }

                if (def.flags & FragmentOperationsDef::DEPTH_TEST) {
                        glDepthMask (GL_TRUE);
                        glEnable(GL_DEPTH_TEST);
                }
        }

        ~FragmentOperationsScope()
        {
                glDisable(GL_BLEND);
                glDepthMask (GL_FALSE);
                glDisable(GL_DEPTH_TEST);
        }
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

        std::transform(std::begin(inputs.intValues),
                       std::end(inputs.intValues),
                       std::back_inserter(bindings.intVectorsUniforms),
        [&program](ProgramInputs::IntInput const& element) {
                return glGetUniformLocation(program.programId, element.name.c_str());
        });


        return bindings;
};

static
void innerDrawOne(FrameSeries& output,
                  ProgramDef programDef,
                  ProgramInputs inputs,
                  GeometryDef geometryDef)
{
        // define and draw the content of the frame

        if (programDef.vertexShader.source.empty()
            || programDef.fragmentShader.source.empty()) {
                return;
        }

        auto const& program = output.program(programDef);
        glUseProgram(program.programId);
        {
                auto vars = programBindings(program, inputs);

                auto bindTextureUnits = [&inputs,&vars,&output]() {
                        // and return the active texture targets
                        auto textureUnits = std::vector<std::pair<GLenum,GLenum>> {};

                        auto i = 0;
                        for (auto& textureInput : inputs.textures) {
                                auto unitIndex = i;
                                auto uniformId = vars.textureUniforms[i];
                                i++;

                                if (uniformId < 0) {
                                        continue;
                                }
                                auto const& texture = output.texture(textureInput.content);
                                if (texture.target == 0) {
                                        printf("texture target is 0, ignoring texture\n");
                                        continue;
                                }
                                auto unit = GL_TEXTURE0 + unitIndex;
                                textureUnits.emplace_back(unit, texture.target);
                                glActiveTexture(unit);
                                glBindTexture(texture.target, texture.textureId);
                                glUniform1i(uniformId, unitIndex);
                        }

                        OGL_TRACE;

                        return textureUnits;
                };

                auto unbindTextureUnits = [](std::vector<std::pair<GLenum,GLenum>> const&
                activeTextureUnits) {
                        // unbind
                        for (auto pair : activeTextureUnits) {
                                glActiveTexture(pair.first);
                                glBindTexture(pair.second, 0);
                        }
                        glActiveTexture(GL_TEXTURE0);
                };

                auto bindFloatUniforms = [&inputs,&vars]() {
                        auto i = 0;
                        for (auto& floatInput : inputs.floatValues) {
                                auto uniformId = vars.floatVectorsUniforms[i];
                                i++;

                                if (uniformId < 0) {
                                        continue;
                                }

                                auto const& values = floatInput.values;
                                auto const width  = values.size();
                                auto const last_row = floatInput.last_row;
                                if (last_row == 0) {
                                        void (*set_many_vecn)(GLint location, GLsizei count,
                                                              const GLfloat *value) = nullptr;

                                        switch (width) {
                                        case 4:
                                                set_many_vecn = glUniform4fv;
                                                break;
                                        case 3:
                                                set_many_vecn = glUniform3fv;
                                                break;
                                        case 2:
                                                set_many_vecn = glUniform2fv;
                                                break;
                                        case 1:
                                                set_many_vecn = glUniform1fv;
                                                break;
                                        }

                                        if (!set_many_vecn) {
                                                printf("invalid number of float inputs: %lu\n", width);
                                        }

                                        set_many_vecn(uniformId, 1, &values.front());
                                } else {
                                        void (*set_many_matnm)(GLint location, GLsizei count, GLboolean tranpose,
                                                               const GLfloat* value) = nullptr;

                                        auto const rows = 1+last_row;
                                        auto const columns = width / rows;

                                        auto dim = (rows & 0xff) | ((columns & 0xff) << 8);
                                        switch(dim) {
                                        case 0x0202:
                                                set_many_matnm = glUniformMatrix2fv;
                                                break;
                                        case 0x0303:
                                                set_many_matnm = glUniformMatrix3fv;
                                                break;
                                        case 0x0404:
                                                set_many_matnm = glUniformMatrix4fv;
                                                break;
                                        case 0x0402:
                                                set_many_matnm = glUniformMatrix4x2fv;
                                                break;
                                        case 0x0302:
                                                set_many_matnm = glUniformMatrix3x2fv;
                                                break;
                                        case 0x0403:
                                                set_many_matnm = glUniformMatrix4x3fv;
                                                break;
                                        case 0x0203:
                                                set_many_matnm = glUniformMatrix2x3fv;
                                                break;
                                        case 0x0304:
                                                set_many_matnm = glUniformMatrix3x4fv;
                                                break;
                                        case 0x0204:
                                                set_many_matnm = glUniformMatrix2x4fv;
                                                break;

                                        }

                                        if (!set_many_matnm) {
                                                printf("invalid number of float inputs: %lu, rows: %u\n", width, 1+last_row);
                                        }

                                        set_many_matnm(uniformId, 1, GL_TRUE, &values.front());
                                }
                                OGL_TRACE;
                        }
                };

                auto bindIntUniforms = [&inputs,&vars]() {
                        auto i = 0;
                        for (auto& intInput : inputs.intValues) {
                                auto uniformId = vars.intVectorsUniforms[i];
                                i++;

                                if (uniformId < 0) {
                                        continue;
                                }

                                auto const& values = intInput.values;
                                auto const width  = values.size();

                                void (*set_many_vecn)(GLint location, GLsizei count,
                                                      const GLint *value) = nullptr;

                                switch (width) {
                                case 4:
                                        set_many_vecn = glUniform4iv;
                                        break;
                                case 3:
                                        set_many_vecn = glUniform3iv;
                                        break;
                                case 2:
                                        set_many_vecn = glUniform2iv;
                                        break;
                                case 1:
                                        set_many_vecn = glUniform1iv;
                                        break;
                                }

                                if (!set_many_vecn) {
                                        printf("invalid number of int inputs: %lu\n", width);
                                }

                                set_many_vecn(uniformId, 1, &values.front());
                                OGL_TRACE;
                        }
                };

                auto activeTextureUnits = bindTextureUnits();
                bindFloatUniforms();
                bindIntUniforms();

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

                unbindTextureUnits(activeTextureUnits);
                OGL_TRACE;
        }
        glUseProgram(0);
};

static
void innerDrawMany(FrameSeries& output, ProgramDef program,
                   std::vector<RenderObjectDef> objects)
{
        for (auto const& object : objects) {
                innerDrawOne(output, program, object.inputs, object.geometry);
        }

}

void beginFrame(FrameSeries& output)
{
        output.beginFrame();
}

void drawMany(FrameSeries& output,
              FragmentOperationsDef fragmentOperations,
              ProgramDef program,
              std::vector<RenderObjectDef> objects)
{
        FragmentOperationsScope withFO(fragmentOperations);
        innerDrawMany(output, program, objects);
}

TextureDef drawManyIntoTexture(FrameSeries& output,
                               TextureDef spec,
                               FragmentOperationsDef fragmentOperations,
                               ProgramDef program,
                               std::vector<RenderObjectDef> objects)
{
        auto resolution = viewport();
        auto fb = output.framebuffer(spec);

        glBindFramebuffer(GL_FRAMEBUFFER, fb.framebufferId);
        glDrawBuffer (GL_COLOR_ATTACHMENT0);
        glReadBuffer (GL_COLOR_ATTACHMENT0);
        glViewport (0, 0, fb.textureDef.width, fb.textureDef.height);

        {
                FragmentOperationsScope withFO(fragmentOperations);

                innerDrawMany(output, program, objects);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glReadBuffer (GL_BACK);
        glDrawBuffer (GL_BACK);
        glViewport(0, 0, resolution.first, resolution.second);

        return fb.textureDef;
}

void drawOne(FrameSeries& output,
             FragmentOperationsDef fragmentOperations,
             ProgramDef programDef,
             ProgramInputs inputs,
             GeometryDef geometryDef)
{
        FragmentOperationsScope withFO(fragmentOperations);
        innerDrawOne(output, programDef, inputs, geometryDef);
}
