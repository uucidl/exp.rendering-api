#include "../ref/main_types.h"
#include "../ref/mesh.h"
#include "../ref/shader_types.h"
#include "../src/glresource_types.hpp"
#include "../src/gltexturing.hpp"
#include "../src/gldebug.hpp"
#include "../src/glshaders.hpp"

#include <micros/api.h>
#include <GL/glew.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"
#undef STB_PERLIN_IMPLEMENTATION

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#undef STB_IMAGE_WRITE_IMPLEMENTATION

#pragma clang diagnostic pop

static std::string dirname(std::string path)
{
        return path.substr(0, path.find_last_of("/\\"));
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

static void bindUniforms(GLuint programId)
{
        GLint texture1Loc = glGetUniformLocation(programId,
                            "tex0");
        GLint texture2Loc = glGetUniformLocation(programId,
                            "tex1");
        glUniform1i(texture1Loc, 0); // bind to texture unit 0
        glUniform1i(texture2Loc, 1); // bind to texture unit 0
}

extern void render_next_gl3(uint64_t time_micros)
{
        static class Tasks : public DisplayThreadTasks
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

                std::mutex tasks_mtx;
                std::vector<std::packaged_task<bool()>> tasks;
        } tasks;

        static class SrcFileSystem : public FileSystem
        {
        public:
                SrcFileSystem() : base_path(dirname(__FILE__)) {}

                std::ifstream open_file(std::string relpath) const
                {
                        auto stream = std::ifstream(base_path + "/" + relpath);

                        if (stream.fail()) {
                                throw std::runtime_error("could not load file at " + relpath);
                        }

                        return stream;
                }

                std::string base_path;
        } srcFileSystem;

        static struct Resources {
                Resources() :
                        fileLoader(makeFileLoader(srcFileSystem, tasks)),
                        shader_loader(tasks, srcFileSystem)
                {
                        shader_loader.load_shader("main.vs", "main.fs", [=](ShaderProgram&& input) {
                                OGL_TRACE;
                                WithShaderProgramScope withShader(input);
                                OGL_TRACE;
                                mainShader = std::move(input);
                        });

                        loadFilePair(*fileLoader.get(), "main.vs",
                        "main.fs", [=](std::string const& contentVS, std::string const& contentFS) {
                                defineProgram(mainShader2, contentVS, contentFS);
                        });


                        {
                                int const width = 64;
                                int const height = 64;
                                OGL_TRACE;

                                withTexture(noiseTexture,
                                []() {
                                        defineNonMipmappedARGB32Texture(width, height, [](uint32_t* data,
                                        int const width, int const height) {
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

                                        });
                                });
                        }

                }

                ShaderProgram mainShader;
                SimpleShaderProgram mainShader2;
                FileLoaderResource fileLoader;
                ShaderLoader shader_loader;
                TextureResource noiseTexture;
        } resources;

        tasks.run();

        if (resources.mainShader.ref() > 0) {
                Mesh quad;
                quad.defQuad2d(0,
                               -1.0, -1.0, 2.0, 2.0,
                               0.0, 0.0, 1.0, 1.0);
                OGL_TRACE;

                {
                        WithShaderProgramScope withShader(resources.mainShader);
                        bindUniforms(resources.mainShader.ref());
                }

                {
                        auto program = resources.mainShader.ref();
                        GLuint position_attrib = glGetAttribLocation(program, "position");
                        GLuint texcoord_attrib = glGetAttribLocation(program, "texcoord");

                        quad.bind(position_attrib, texcoord_attrib);
                }
                OGL_TRACE;
                glActiveTexture(GL_TEXTURE0);
                OGL_TRACE;

                withTexture(resources.noiseTexture, [&quad]() {
                        auto program = resources.mainShader.ref();
                        glUseProgram(program);
                        quad.draw();
                        glUseProgram(0);
                });
        }
}

int main()
{
        runtime_init();

        return 0;
}

// implementations

#include "../ref/fs.cpp"
#include "../ref/mesh.cpp"
#include "../ref/shaders.cpp"
#include "../src/glresources.cpp"
#include "../src/glshaders.cpp"
#include "../src/gltexturing.cpp"
