#include "main_types.hpp"
#include "quad.hpp"
#include "renderer.hpp"

#include "../gl3companion/glresource_types.hpp"
#include "../src/hstd.hpp"

#include <cmath>

struct Rect {
        float x;
        float y;
        float width;
        float height;
};

struct QuadDefinerParams {
        Rect coords;
        Rect uvcoords;
};

static
size_t quadDefiner(BufferResource const& elementBuffer,
                   BufferResource const arrays[],
                   void const* data)
{
        auto params = static_cast<QuadDefinerParams const*> (data);
        auto& coords = params->coords;
        auto& uvcoords = params->uvcoords;

        size_t indicesCount = define2dQuadIndices(elementBuffer);
        define2dQuadBuffer(arrays[0],
                           coords.x, coords.y,
                           coords.width, coords.height);
        define2dQuadBuffer(arrays[1],
                           uvcoords.x, uvcoords.y,
                           uvcoords.width, uvcoords.height);

        return indicesCount;
}

static void perlinTexture(uint32_t* pixels, int width, int height, int depth,
                          void const* data)
{
        perlinNoisePixelFiller(pixels, width, height);
}

extern void render_textured_quad_v2(uint64_t time_micros)
{
        // infrastructure
        static Tasks tasks;
        static RootDirFileSystem codeBaseFS { dirname(__FILE__) };
        static FileLoaderResource fileLoader = makeFileLoader(codeBaseFS, tasks);

        tasks.run();

        // caches
        struct FileContent {
                int state;
                std::string path;
                std::string content;
        };
        struct FileHandle {
                long id;
        };
        static auto loadingFiles = std::vector<FileContent> {};
        static auto loadedFiles = std::vector<FileContent> {};
        auto loadFileContent = [](std::string const& path) -> FileHandle {
                auto lookupFC = [](std::vector<FileContent>& cache,
                std::string const& path) -> FileContent* {
                        auto existing = std::find_if(std::begin(cache), std::end(cache), [=](FileContent const& fc)
                        {
                                return path == fc.path;
                        });
                        if (existing == std::end(cache))
                        {
                                return nullptr;
                        }

                        return &(*existing);
                };

                auto loaded = lookupFC(loadedFiles, path);
                if (loaded)
                {
                        return { HSTD_DFIELD(id, loaded - &loadedFiles.front()) };
                }

                auto loading = lookupFC(loadingFiles, path);
                if (!loading)
                {
                        loadingFiles.push_back({
                                HSTD_DFIELD(state, -1),
                                HSTD_DFIELD(path, path),
                                HSTD_DFIELD(content,"")
                        });
                        loadFile(*fileLoader.get(), path,
                        [=](std::string const& content) {
                                auto loading = lookupFC(loadingFiles, path);
                                loading->content = content;
                                loading->state = 0;
                        });
                        return { HSTD_DFIELD(id, -1) };
                }

                if (loading->state == 0)
                {
                        loadedFiles.push_back(*loading);
                        return { HSTD_DFIELD(id, static_cast<long>(loadedFiles.size() - 1)) };
                }

                return { HSTD_DFIELD(id, -1) };
        };

        // prototype for a new render code

        // constructors
        auto vertexShaderFromFile = [=](std::string filename) {
                auto fh = loadFileContent(filename);
                if (fh.id < 0) {
                        return VertexShaderDef {};
                }

                return VertexShaderDef { HSTD_DFIELD(source, loadedFiles[fh.id].content) };
        };

        auto fragmentShaderFromFile = [=](std::string filename) {
                auto fh = loadFileContent(filename);
                if (fh.id < 0) {
                        return FragmentShaderDef {};
                }

                return FragmentShaderDef { HSTD_DFIELD(source, loadedFiles[fh.id].content) };
        };

        // drawing infrastructure

        // user defined primitives library
        auto quad = [](Rect coords, Rect uvcoords) -> GeometryDef {
                auto geometry = GeometryDef {};

                geometry.arrayCount = 2;
                geometry.data.resize(sizeof(QuadDefinerParams));

                auto params = new (&geometry.data.front()) QuadDefinerParams;
                *params = {
                        HSTD_DFIELD(coords, coords),
                        HSTD_DFIELD(uvcoords, uvcoords)
                };

                geometry.definer = quadDefiner;

                return geometry;
        };

        // user code

        static auto output = makeFrameSeries();

        auto texture = [](int width, int height, TextureDefFn const& fn) {
                auto textureDef = TextureDef {};
                textureDef.width = width;
                textureDef.height = height;
                textureDef.pixelFiller = fn;
                return textureDef;
        };

        beginFrame(*output);

        drawOne(*output, {
                HSTD_DFIELD(vertexShader, vertexShaderFromFile("main.vs")),
                HSTD_DFIELD(fragmentShader, fragmentShaderFromFile("main.fs"))
        }, {
                {
                        { "position", 2 },
                        { "texcoord", 2 },
                },
                {
                        {
                                HSTD_DFIELD(name, "tex0"),
                                HSTD_DFIELD(content, texture(128, 128, perlinTexture))
                        }
                },
                {
                        {
                                "g_color", { 0.2, 0.4, (float)(0.1 + 0.3 * sin(time_micros / 1000000.0)), 0.0 }, 0,
                        },
                        {
                                "translation", { (float)(0.15 * sin(time_micros / 100000.0)), 0.0, 0.0, 0.0 }, 0
                        },
                        {
                                "transform", {
                                        1.0, 0.0, 0.0, 0.0,
                                        0.0, (float) (1.0 + 0.35*sin(time_micros/70000.0)), 0.0, 0.0,
                                        0.0, 0.0, 1.0, 0.0,
                                        0.0, 0.0, 0.0, 1.0
                                }, 3
                        },
                },
                {},

        }
        ,
        quad({
                HSTD_DFIELD(x, -0.80),
                HSTD_DFIELD(y, -.80),
                HSTD_DFIELD(width, 1.6),
                HSTD_DFIELD(height, 1.6)
        }, {
                HSTD_DFIELD(x, 0.0),
                HSTD_DFIELD(y, 0.0),
                HSTD_DFIELD(width, 1.0),
                HSTD_DFIELD(height, 1.0)
        })
               );
}
