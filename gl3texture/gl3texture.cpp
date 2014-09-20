#include "../ref/main_types.h"
#include "../src/glresource_types.hpp"
#include "../src/gltexturing.hpp"
#include "../src/gldebug.hpp"
#include "../src/glshaders.hpp"

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
                   char* data)
{
        auto params = reinterpret_cast<QuadDefinerParams*> (data);
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
                        return { .id = loaded - &loadedFiles.front() };
                }

                auto loading = lookupFC(loadingFiles, path);
                if (!loading)
                {
                        loadingFiles.push_back({ .state = -1, .path = path });
                        loadFile(*fileLoader.get(), path,
                        [=](std::string const& content) {
                                auto loading = lookupFC(loadingFiles, path);
                                loading->content = content;
                                loading->state = 0;
                        });
                        return { .id = -1 };
                }

                if (loading->state == 0)
                {
                        loadedFiles.push_back(*loading);
                        return { .id = static_cast<long>(loadedFiles.size() - 1) };
                }

                return { .id = -1 };
        };

        // prototype for a new render code

        // value types
        struct VertexShader {
                std::string source;
        };
        struct FragmentShader {
                std::string source;
        };
        struct Program {
                VertexShader vertexShader;
                FragmentShader fragmentShader;
        };
        struct TextureDef {
                int width = 0;
                int height = 0;
                void (*pixelFiller)(uint32_t* pixels, int width, int height) = nullptr;
        };

        struct ProgramInputs {
                std::vector<TextureDef> textures;
        };
        struct GeometryDef {
                std::vector<char> data;
                size_t arrayCount = 0;

                // @returns indices count
                size_t (*definer)(BufferResource const& elementBuffer,
                                  BufferResource const arrays[],
                                  char* data) = nullptr;
        };

        // constructors
        auto vertexShaderFromFile = [=](std::string filename) {
                auto fh = loadFileContent(filename);
                if (fh.id < 0) {
                        return VertexShader {};
                }

                return VertexShader { .source = loadedFiles[fh.id].content };
        };

        auto fragmentShaderFromFile = [=](std::string filename) {
                auto fh = loadFileContent(filename);
                if (fh.id < 0) {
                        return FragmentShader {};
                }

                return FragmentShader { .source = loadedFiles[fh.id].content };
        };

        // drawing infrastructure

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

        auto draw = [](FrameSeries& output, Program programDef, ProgramInputs inputs,
        GeometryDef geometryDef) {
                OGL_TRACE;
                output.beginFrame();

                // define and draw the content of the frame

                if (programDef.vertexShader.source.empty()
                    || programDef.fragmentShader.source.empty()) {
                        return;
                }

                auto const& program = output.program(programDef);
                withShaderProgram(program,
                [&output,&inputs,&program,&geometryDef]() {
                        auto const vars = getMainShaderVariables(program);
                        auto textureTargets = std::vector<GLenum> {};
                        {
                                auto i = 0;
                                for (auto& textureDef : inputs.textures) {
                                        auto unit = i;
                                        auto& texture = output.texture(textureDef);
                                        auto target = GL_TEXTURE0 + unit;

                                        textureTargets.emplace_back(target);
                                        OGL_TRACE;
                                        glActiveTexture(target);
                                        glBindTexture(GL_TEXTURE_2D, texture.id);
                                        glUniform1i(vars.textureUniforms[unit], unit);
                                        i++;
                                        OGL_TRACE;
                                }
                        }

                        auto const& mesh = output.mesh(geometryDef);

                        // draw here
                        withVertexArray(mesh.vertexArray, [&program,&vars,&mesh]() {
                                auto vertexAttribVars = std::vector<GLint> {
                                        vars.positionAttrib,
                                        vars.texcoordAttrib,
                                };

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

        // user defined primitives library
        auto quad = [](Rect coords, Rect uvcoords) -> GeometryDef {
                auto geometry = GeometryDef {};

                geometry.arrayCount = 2;
                geometry.data.resize(sizeof(QuadDefinerParams));

                auto params = reinterpret_cast<QuadDefinerParams*> (&geometry.data.front());
                *params = { .coords = coords, .uvcoords = uvcoords };

                geometry.definer = quadDefiner;

                return geometry;
        };

        // user code

        static FrameSeries output;

        auto texture = [](int width, int height, void (*pixelFiller)(uint32_t*,int,
        int)) {
                auto textureDef = TextureDef {};
                textureDef.width = width;
                textureDef.height = height;
                textureDef.pixelFiller = pixelFiller;

                return textureDef;
        };

        draw(output, {
                .vertexShader = vertexShaderFromFile("main.vs"),
                .fragmentShader = fragmentShaderFromFile("main.fs")
        }, {
                .textures = {
                        texture(128, 128, perlinNoisePixelFiller)
                }
        },
        quad({ .x = (float)(-0.80 + 0.15 * sin(time_micros / 100000.0)), .y = -.80, .width = 1.6, .height = 1.6 },
        { .x = 0.0, .y = 0.0, .width = 1.0, .height = 1.0 })
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
