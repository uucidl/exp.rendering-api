#include "razors.h"

#include "framebuffer.h"
#include "main_types.h"
#include "material.h"
#include "mesh.h"
#include "shader_types.h"
#include "texture.h"
#include "texture_types.h"
#include "matrix.hpp"

#include <micros/api.h>

#include <GL/glew.h>
#include "debug.h"

#include <math.h>
#include <stdio.h> // for printf
#include <vector>

extern void render_next_2chn_48khz_audio(uint64_t time_micros,
                int const sample_count, double left[/*sample_count*/],
                double right[/*sample_count*/])
{
        // silence is soothing
}

class DisplayFrameImpl
{};

std::string dirname(std::string path)
{
        return path.substr(0, path.find_last_of("/\\"));
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
                        OGL_TRACE;
                        vector4 argb;
                        vector4_make(argb,
                                     0.99997f, 1.0f, 1.0f, 1.0f
                                    );
                        classyWhite.commitWith(MF_NO_DEPTH_TEST | MF_BLEND | MF_TEXTURE_REPEAT,
                                               argb);
                        vector4_make(argb,
                                     0.99997f, 1.0f, 0.90f, 0.91f
                                    );
                        reddishWhite.commitWith(MF_NO_DEPTH_TEST | MF_BLEND | MF_TEXTURE_REPEAT,
                                                argb);
                        vector4_make(argb,
                                     1.0f, 1.0f, 1.0f, 1.0f
                                    );
                        brightWhite.commitWith(MF_NO_DEPTH_TEST | MF_BLEND | MF_TEXTURE_REPEAT,
                                               argb);

                        OGL_TRACE;

                        shader_loader.load_shader("pink.vs", "pink.fs", [=](ShaderProgram&& input) {
                                OGL_TRACE;
                                pinkShader = std::move(input);
                        });
                        shader_loader.load_shader("main.vs", "main.fs", [=](ShaderProgram&& input) {
                                OGL_TRACE;
                                mainShader = std::move(input);
                                OGL_TRACE;
                                WithShaderProgramScope withShader(mainShader);
                                GLint texture1Loc = glGetUniformLocation(mainShader.ref(),
                                                    "tex");


                                glUniform1i(texture1Loc, 0); // bind to texture unit 0
                                OGL_TRACE;
                        });
                }

                Material classyWhite;
                Material brightWhite;
                Material reddishWhite;
                ShaderProgram mainShader;
                ShaderProgram pinkShader;
                ShaderLoader shader_loader;
                GLuint position_attr;
                Framebuffer framebuffers[3];
        } resources;

        OGL_TRACE;
        tasks.run();
        OGL_TRACE;

        double const phase = 6.30 * time_micros / 1e6 / 1.0;
        float sincos[2] = {
                static_cast<float>(0.49 * sin(phase)),
                static_cast<float>(0.49 * cos(phase)),
        };
        float const argb[4] = {
                0.0f, 0.31f + 0.39f * sincos[0], 0.27f + 0.39f * sincos[1], 0.29f
        };
        glClearColor (argb[1], argb[2], argb[3], argb[0]);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        OGL_TRACE;


        DisplayFrameImpl frame;
        Framebuffer* framebuffers[] = {
                &resources.framebuffers[0],
                &resources.framebuffers[1],
                &resources.framebuffers[2],
        };

        if (resources.mainShader.ref() && resources.pinkShader.ref()) {
                razors(&frame, 1.0 * time_micros / 1.0e5,
                       resources.classyWhite, resources.mainShader,
                       framebuffers, 55.7f, 7.0f, 0.006f, sincos[0] > 0.44 ? 1 : 0,
                       resources.brightWhite, resources.pinkShader);
        }
}

int main (int argc, char** argv)
{
        (void) argc;
        (void) argv;

        runtime_init();

        return 0;
}
