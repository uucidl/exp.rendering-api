#pragma once

#include "renderer.hpp"

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
                reset(meshHeap);
                reset(textureHeap);
                reset(programHeap);
        }

        struct MeshMaterials {
                GLuint vertexArray;
                size_t indicesCount;
                GLuint indicesBuffer;
                std::vector<GLuint> vertexBuffers;
        };

        MeshMaterials mesh(GeometryDef geometryDef)
        {
                auto meshIndex = findOrCreate<GeometryDef>
                                 (meshHeap,
                                  geometryDef,
                [&geometryDef](GeometryDef const& element) {
                        return element.data == geometryDef.data
                               && element.definer == geometryDef.definer
                               && element.arrayCount == element.arrayCount;
                },
                [=](GeometryDef const& def, size_t meshIndex) {
                        meshes.resize(1 + meshIndex);

                        auto& mesh = meshes[meshIndex];
                        if (def.definer) {
                                mesh.vertexBuffers.resize(def.arrayCount);
                                mesh.indicesCount = def.definer
                                                    (mesh.indices,
                                                     &mesh.vertexBuffers.front(),
                                                     &def.data.front());
                        }

                        meshCreations++;
                },
                [=](size_t indexA, size_t indexB) {
                        std::swap(meshes.at(indexA), meshes.at(indexB));
                });

                auto const& mesh = meshes.at(meshIndex);
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
                auto index = findOrCreate<TextureDef>(textureHeap,
                                                      textureDef,
                [&textureDef](TextureDef const& element) {
                        return element.width == textureDef.width
                               && element.height == textureDef.height
                               && element.pixelFiller == textureDef.pixelFiller;
                },
                [=](TextureDef const& def, size_t index) {
                        textures.resize(index + 1);

                        auto& texture = textures[index];
                        withTexture(texture,
                        [&def]() {
                                defineNonMipmappedARGB32Texture(def.width,
                                                                def.height,
                                                                def.pixelFiller);
                        });
                        OGL_TRACE;
                        textureCreations++;
                },
                [=](size_t indexA, size_t indexB) {
                        std::swap(textures[indexA], textures[indexB]);
                });

                return { textures[index].id };
        }

        struct ShaderProgramMaterials {
                GLuint programId;
        };

        ShaderProgramMaterials program(ProgramDef programDef)
        {
                auto index = findOrCreate<ProgramDef>
                             (programHeap,
                              programDef,
                [&programDef](ProgramDef const& element) {
                        return element.fragmentShader.source
                               == programDef.fragmentShader.source
                               && element.vertexShader.source
                               == programDef.vertexShader.source;
                },
                [=](ProgramDef const& def, size_t index) {
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
                },
                [=](size_t indexA, size_t indexB) {
                        std::swap(programs[indexA], programs[indexB]);
                        std::swap(vertexShaders[indexA], vertexShaders[indexB]);
                        std::swap(fragmentShaders[indexA], fragmentShaders[indexB]);
                });

                return { programs[index].id };
        }

private:
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

        template <typename ResourceDef>
        struct RecyclingHeap {
                size_t firstInactiveIndex;
                size_t firstRecyclableIndex;
                std::vector<ResourceDef>& definitions;
        };

        template <typename ResourceDef>
        void reset(RecyclingHeap<ResourceDef>& heap)
        {
                heap.firstRecyclableIndex = heap.firstInactiveIndex;
                heap.firstInactiveIndex = 0;
        }

        template <typename ResourceDef>
        size_t findOrCreate(RecyclingHeap<ResourceDef>& heap,
                            ResourceDef const& def,
                            std::function<bool(ResourceDef const&)> isDef,
                            std::function<void(ResourceDef const&,size_t)> createAt,
                            std::function<void(size_t,size_t)> swap)
        {
                auto index = findOrCreateDef(heap.definitions,
                                             def,
                                             isDef,
                                             heap.firstRecyclableIndex);
                if (index == heap.firstRecyclableIndex) {
                        heap.firstRecyclableIndex++;
                        createAt(def, index);
                }

                auto newIndex = heap.firstInactiveIndex;
                if (newIndex != index) {
                        std::swap(heap.definitions.at(newIndex), heap.definitions.at(index));
                        swap(newIndex, index);
                }

                heap.firstInactiveIndex = newIndex + 1;

                return newIndex;
        }

        struct Mesh {
                VertexArrayResource vertexArray;
                size_t indicesCount = 0;
                BufferResource indices;
                std::vector<BufferResource> vertexBuffers;
        };

        std::vector<Mesh> meshes;
        std::vector<GeometryDef> meshDefs;
        RecyclingHeap<GeometryDef> meshHeap = { 0, 0, meshDefs };
        long meshCreations = 0;

        std::vector<TextureResource> textures;
        std::vector<TextureDef> textureDefs;
        RecyclingHeap<TextureDef> textureHeap = { 0, 0, textureDefs };
        long textureCreations = 0;

        std::vector<VertexShaderResource> vertexShaders;
        std::vector<FragmentShaderResource> fragmentShaders;
        std::vector<ShaderProgramResource> programs;
        std::vector<ProgramDef> programDefs;
        RecyclingHeap<ProgramDef> programHeap = { 0, 0, programDefs };
        long programCreations = 0;
};
