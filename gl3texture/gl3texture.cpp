#include "../src/main_types.h"
#include "../src/shaders.cpp"
#include "../src/shader_types.h"
#include "../src/texture_types.h"
#include "../src/mesh.h"
#include "../src/mesh.cpp"

#include <micros/api.h>
#include <GL/glew.h>
#include "../src/debug.h"

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
                Resources() : shader_loader(tasks, srcFileSystem)
                {
                        shader_loader.load_shader("main.vs", "main.fs", [=](ShaderProgram&& input) {
                                OGL_TRACE;
                                WithShaderProgramScope withShader(input);
                                OGL_TRACE;

                                GLint texture1Loc = glGetUniformLocation(input.ref(),
                                                    "tex0");
                                GLint texture2Loc = glGetUniformLocation(input.ref(),
                                                    "tex1");
                                OGL_TRACE;

                                glUniform1i(texture1Loc, 0); // bind to texture unit 0
                                OGL_TRACE;
                                glUniform1i(texture2Loc, 1); // bind to texture unit 0
                                OGL_TRACE;
                                glLinkProgram(input.ref());
                                OGL_TRACE;
                                mainShader = std::move(input);
                        });

                        {
                                int const width = 64;
                                int const height = 64;
                                OGL_TRACE;

                                uint32_t data[width*height];
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

                                glActiveTexture(GL_TEXTURE0);
                                OGL_TRACE;
                                glBindTexture(GL_TEXTURE_2D, noiseTexture.ref);
                                OGL_TRACE;

                                glTexImage2D(GL_TEXTURE_2D,
                                             0,
                                             GL_RGBA,
                                             width,
                                             height,
                                             0,
                                             GL_RGBA,
                                             GL_UNSIGNED_INT_8_8_8_8_REV,
                                             data);
                                stbi_write_png("dump.png", width, height, 4, data, 0);
                                OGL_TRACE;
                                glBindTexture(GL_TEXTURE_2D, 0);
                                OGL_TRACE;
                        }

                }

                ShaderProgram mainShader;
                ShaderLoader shader_loader;
                Texture noiseTexture;
        } resources;

        tasks.run();

        if (resources.mainShader.ref() > 0) {
                Mesh quad;
                quad.defQuad2d(0,
                               -1.0, -1.0, 2.0, 2.0,
                               0.0, 0.0, 1.0, 1.0);
                OGL_TRACE;
                quad.bind(resources.mainShader);
                OGL_TRACE;
                glActiveTexture(GL_TEXTURE0);
                OGL_TRACE;
                glBindTexture(GL_TEXTURE_2D, resources.noiseTexture.ref);
                OGL_TRACE;
                quad.draw();
                OGL_TRACE;
        }
}

int main()
{
        runtime_init();

        return 0;
}
