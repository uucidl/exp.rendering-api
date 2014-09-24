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
                meshHeap.firstRecyclableIndex = meshHeap.firstInactiveIndex;
                meshHeap.firstInactiveIndex = 0;
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

        template <typename ResourceDef>
        struct RecyclingHeap {
                size_t firstInactiveIndex;
                size_t firstRecyclableIndex;
                std::vector<ResourceDef>& definitions;
        };

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
                        meshHeap.firstRecyclableIndex++;
                        createAt(def, index);
                }

                auto newIndex = heap.firstInactiveIndex;
                if (newIndex != index) {
                        std::swap(heap.definitions.at(newIndex), heap.definitions.at(index));
                        swap(newIndex, index);
                        heap.firstInactiveIndex++;
                }

                return newIndex;
        }

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
        RecyclingHeap<GeometryDef> meshHeap = { 0, 0, meshDefs };
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
