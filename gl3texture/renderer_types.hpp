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
                firstRecyclableMesh = firstInactiveMesh;
                firstInactiveMesh = 0;
        }

        struct MeshMaterials {
                GLuint vertexArray;
                size_t indicesCount;
                GLuint indicesBuffer;
                std::vector<GLuint> vertexBuffers;
        };

        // returns index to use (and create an entry if missing)
        template <typename ResourceDef>
        size_t findOrCreateDef(std::vector<ResourceDef>& definitions,
                               ResourceDef const& def,
                               std::function<bool(ResourceDef const&)> isDef,
                               size_t firstRecyclableIndex)
        {
                auto existing =
                        std::find_if(std::begin(definitions),
                                     std::end(definitions),
                                     isDef);

                if (existing != std::end(definitions)) {
                        return &(*existing) - &definitions.front();
                } else {
                        auto index = firstRecyclableIndex;
                        if (index >= definitions.size()) {
                                definitions.resize(1 + index);

                        }
                        definitions[index] = def;
                        return firstRecyclableIndex;
                }
        }

        MeshMaterials mesh(GeometryDef geometryDef)
        {
                auto meshIndex = findOrCreateDef<GeometryDef>
                                 (meshDefs,
                                  geometryDef,
                [&geometryDef](GeometryDef const& element) {
                        return element.data == geometryDef.data
                               && element.definer == geometryDef.definer
                               && element.arrayCount == element.arrayCount;
                },
                firstRecyclableMesh);

                if (meshIndex == firstRecyclableMesh) {
                        firstRecyclableMesh++;
                        meshes.resize(1 + meshIndex);

                        auto& mesh = meshes[meshIndex];
                        if (geometryDef.definer) {
                                mesh.vertexBuffers.resize(geometryDef.arrayCount);
                                mesh.indicesCount = geometryDef.definer
                                                    (mesh.indices,
                                                     &mesh.vertexBuffers.front(),
                                                     &geometryDef.data.front());
                        }

                        meshCreations++;
                }

                std::swap(meshes.at(firstInactiveMesh), meshes.at(meshIndex));
                std::swap(meshDefs.at(firstInactiveMesh), meshDefs.at(meshIndex));
                firstInactiveMesh++;

                auto const& mesh = meshes.at(firstInactiveMesh - 1);
                auto vertexBufferIds = std::vector<GLuint> {};
                std::transform(std::begin(mesh.vertexBuffers),
                               std::end(mesh.vertexBuffers),
                               std::back_inserter(vertexBufferIds),
                [](BufferResource const& element) {
                        return element.id;
                });

                return {
                        mesh.vertexArray.id,
                        mesh.indicesCount,
                        mesh.indices.id,
                        vertexBufferIds
                };
        }

        struct TextureMaterials {
                GLuint textureId;
        };

        TextureMaterials texture(TextureDef textureDef)
        {
                auto firstRecyclable = textureDefs.size();
                auto index = findOrCreateDef<TextureDef>(textureDefs,
                                                      textureDef,
                [&textureDef](TextureDef const& element) {
                        return element.width == textureDef.width
                               && element.height == textureDef.height
                               && element.pixelFiller == textureDef.pixelFiller;
                },
                firstRecyclable);

                if (index >= firstRecyclable) {
                        textures.resize(index + 1);

                        auto& texture = textures[index];
                        withTexture(texture,
                        [&textureDef]() {
                                defineNonMipmappedARGB32Texture(textureDef.width, textureDef.height,
                                                                textureDef.pixelFiller);
                        });

                        OGL_TRACE;
                        textureCreations++;
                }

                return { textures[index].id };
        }

        struct ShaderProgramMaterials {
                GLuint programId;
        };

        ShaderProgramMaterials program(Program programDef)
        {
                auto firstRecyclable = programDefs.size();
                auto index = findOrCreateDef<Program>
                             (programDefs,
                              programDef,
                [&programDef](Program const& element) {
                        return element.fragmentShader.source
                               == programDef.fragmentShader.source
                               && element.vertexShader.source
                               == programDef.vertexShader.source;
                },
                firstRecyclable);

                if (index >= firstRecyclable) {
                        programs.resize(index + 1);
                        vertexShaders.resize(index + 1);
                        fragmentShaders.resize(index + 1);

                        auto& program = programs[index];
                        auto& vertexShader = vertexShaders[index];
                        auto& fragmentShader = fragmentShaders[index];

                        compile(vertexShader, programDef.vertexShader.source);
                        compile(fragmentShader, programDef.fragmentShader.source);
                        link(program, vertexShader, fragmentShader);

                        OGL_TRACE;
                        programCreations++;
                }

                return { programs[index].id };
        }

private:
        struct Mesh {
                VertexArrayResource vertexArray;
                size_t indicesCount = 0;
                BufferResource indices;
                std::vector<BufferResource> vertexBuffers;
        };

        std::vector<Mesh> meshes;
        std::vector<GeometryDef> meshDefs;
        long meshCreations = 0;
        size_t firstInactiveMesh = 0;
        size_t firstRecyclableMesh = 0;

        std::vector<TextureResource> textures;
        std::vector<TextureDef> textureDefs;
        long textureCreations = 0;

        std::vector<VertexShaderResource> vertexShaders;
        std::vector<FragmentShaderResource> fragmentShaders;
        std::vector<ShaderProgramResource> programs;
        std::vector<Program> programDefs;
        long programCreations = 0;
};
