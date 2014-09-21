#pragma once

#include "../src/gldebug.hpp"
#include "../src/glresource_types.hpp"
#include "../src/glshaders.hpp"
#include "../src/gltexturing.hpp"

#include <vector>

// persistent datastructure... the core of the infrastructure
class FrameSeries
{
public:
        ~FrameSeries()
        {
                printf("summary:\n");
                printf("program creations: %ld\n", programCreations);
                printf("texture creations: %ld\n", textureCreations);
                printf("mesh creations: %ld\n"
                       "meshes: %ld\n",
                       meshCreations,
                       meshes.size());
        }

        void beginFrame()
        {
                // we should invalidate arrays so as to garbage
                // collect / recycle the now un-needed definitions
        }

        struct Mesh {
                VertexArrayResource vertexArray;
                size_t indicesCount = 0;
                BufferResource indices;
                std::vector<BufferResource> vertexBuffers;
        };

        Mesh const& mesh(GeometryDef geometryDef)
        {
                auto existing =
                        std::find_if(std::begin(meshDefs), std::end(meshDefs),
                [&geometryDef](GeometryDef const& element) {
                        return element.data == geometryDef.data
                               && element.definer == geometryDef.definer
                               && element.arrayCount == element.arrayCount;
                });
                if (existing != std::end(meshDefs)) {
                        return meshes.at(&(*existing) - &meshDefs.front());
                }

                meshDefs.push_back(geometryDef);
                meshes.emplace_back();
                auto& mesh = meshes.back();

                if (geometryDef.definer) {
                        mesh.vertexBuffers.resize(geometryDef.arrayCount);
                        mesh.indicesCount = geometryDef.definer
                                            (mesh.indices,
                                             &mesh.vertexBuffers.front(),
                                             &geometryDef.data.front());
                        OGL_TRACE;
                }

                OGL_TRACE;
                meshCreations++;

                return meshes.back();
        }

        TextureResource& texture(TextureDef textureDef)
        {
                auto existing =
                        std::find_if(std::begin(textureDefs), std::end(textureDefs),
                [&textureDef](TextureDef const& element) {
                        return element.width == textureDef.width
                               && element.height == textureDef.height
                               && element.pixelFiller == textureDef.pixelFiller;
                });
                if (existing != std::end(textureDefs)) {
                        return textures.at(&(*existing) - &textureDefs.front());
                }

                textures.emplace_back();
                textureDefs.push_back(textureDef);

                auto& texture = textures.back();
                withTexture(texture,
                [&textureDef]() {
                        defineNonMipmappedARGB32Texture(textureDef.width, textureDef.height,
                                                        textureDef.pixelFiller);
                });

                OGL_TRACE;
                textureCreations++;
                return texture;
        }

        ShaderProgramResource& program(Program programDef)
        {
                auto existing =
                        std::find_if(std::begin(programDefs), std::end(programDefs),
                [&programDef](Program const& element) {
                        return element.fragmentShader.source
                               == programDef.fragmentShader.source
                               && element.vertexShader.source
                               == programDef.vertexShader.source;
                });
                if (existing != std::end(programDefs)) {
                        return programs.at(&(*existing) - &programDefs.front());
                }

                programs.emplace_back();
                programDefs.push_back(programDef);
                vertexShaders.emplace_back();
                fragmentShaders.emplace_back();

                auto& program = programs.back();
                auto& vertexShader = vertexShaders.back();
                auto& fragmentShader = fragmentShaders.back();

                compile(vertexShader, programDef.vertexShader.source);
                compile(fragmentShader, programDef.fragmentShader.source);
                link(program, vertexShader, fragmentShader);

                OGL_TRACE;
                programCreations++;

                return program;
        }

private:
        std::vector<Mesh> meshes;
        std::vector<GeometryDef> meshDefs;
        long meshCreations = 0;

        std::vector<TextureResource> textures;
        std::vector<TextureDef> textureDefs;
        long textureCreations = 0;

        std::vector<VertexShaderResource> vertexShaders;
        std::vector<FragmentShaderResource> fragmentShaders;
        std::vector<ShaderProgramResource> programs;
        std::vector<Program> programDefs;
        long programCreations = 0;
};
