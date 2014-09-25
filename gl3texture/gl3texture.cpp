#include "renderer.hpp"

#include "../ref/main_types.h"
#include "../src/gldebug.hpp"
#include "../src/glresource_types.hpp"
#include "../src/glshaders.hpp"
#include "../src/gltexturing.hpp"
#include "../src/hstd.hpp"

#include <micros/api.h>

#include "stb_perlin.h"
#include "stb_image_write.h"

#include <GL/glew.h>

#include <vector>
#include <future>
#include <cmath>

static std::string dirname(std::string path)
{
        return path.substr(0, path.find_last_of("/\\"));
}

namespace
{
class RootDirFileSystem : public FileSystem
{
public:
        RootDirFileSystem(std::string const& basePath) : base_path(basePath) {}

        std::ifstream open_file(std::string relpath) const
        {
                auto stream = std::ifstream(base_path + "/" + relpath);

                if (stream.fail()) {
                        throw std::runtime_error("could not load file at " + relpath);
                }

                return stream;
        }

private:
        std::string const base_path;
};

class Tasks : public DisplayThreadTasks
{
public:
        void add_task(std::function<bool()>&& task)
        {
                std::lock_guard<std::mutex> lock(tasks_mtx);
                tasks.emplace_back(task);
        }

        void run()
        {
                std::lock_guard<std::mutex> lock(tasks_mtx);
                for (auto& task : tasks) {
                        std::future<bool> future = task.get_future();
                        task();
                        future.get();
                }
                tasks.clear();
        }

private:
        std::mutex tasks_mtx;
        std::vector<std::packaged_task<bool()>> tasks;
};
}

extern void render_next_2chn_48khz_audio(uint64_t time_micros,
                int const sample_count, double left[/*sample_count*/],
                double right[/*sample_count*/])
{
        // silence is soothing
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

static size_t define2dQuadIndices(BufferResource const& buffer)
{
        size_t count = 0;
        withElementBuffer(buffer,
        [&count]() {
                GLuint data[] = {
                        0, 1, 2, 2, 3, 0,
                };
                count = sizeof data / sizeof data[0];

                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof data, data,
                             GL_STREAM_DRAW);
        });

        return count;
}

static GLvoid* define2dQuadBuffer(BufferResource const& buffer, float xmin,
                                  float ymin, float width, float height)
{
        withArrayBuffer(buffer,
        [=]() {
                float data[] = {
                        xmin, ymin,
                        xmin, ymin + height,
                        xmin + width, ymin + height,
                        xmin + width, ymin,
                };

                glBufferData(GL_ARRAY_BUFFER, sizeof data, data, GL_STREAM_DRAW);
        });

        return 0;
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

static
void perlinNoisePixelFiller (uint32_t* data, int width, int height)
{
        for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                        uint8_t values[4];
                        for (size_t i = 0; i < sizeof values / sizeof values[0]; i++) {
                                float val = 0.5 + stb_perlin_noise3(13.0 * x / width, 17.0 * y / height, 0.0);
                                if (val > 1.0) {
                                        val = 1.0;
                                } else if (val < 0.0) {
                                        val = 0.0;
                                }

                                values[i] = (int) (255 * val) & 0xff;
                        }

                        data[x + y*width] = (values[0] << 24)
                                            | (values[1] << 16)
                                            | (values[2] << 8)
                                            | values[3];
                }
        }
}

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
                   char const* data)
{
        auto params = reinterpret_cast<QuadDefinerParams const*> (data);
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

extern void render(uint64_t time_micros)
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

                auto params = reinterpret_cast<QuadDefinerParams*> (&geometry.data.front());
                *params = {
                        HSTD_DFIELD(coords, coords),
                        HSTD_DFIELD(uvcoords, uvcoords)
                };

                geometry.definer = quadDefiner;

                return geometry;
        };

        // user code

        static auto output = makeFrameSeries();

        auto texture = [](int width, int height, void (*pixelFiller)(uint32_t*,int,
        int)) {
                auto textureDef = TextureDef {};
                textureDef.width = width;
                textureDef.height = height;
                textureDef.pixelFiller = pixelFiller;

                return textureDef;
        };

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
                                HSTD_DFIELD(content, texture(128, 128, perlinNoisePixelFiller))
                        }
                }
        }
        ,
        quad({
                HSTD_DFIELD(x, (float)(-0.80 + 0.15 * sin(time_micros / 100000.0))),
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

extern void render_next_gl3(uint64_t time_micros)
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

        render(time_micros);
}

int main()
{
        runtime_init();

        return 0;
}
