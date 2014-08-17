#include "razors.h"

#include "framebuffer.h"
#include "main_types.h"
#include "material.h"
#include "mesh.h"
#include "shader_types.h"

#include <micros/api.h>

#include <GL/glew.h>

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
                        classyWhite.commitWith(MF_NO_DEPTH_TEST | MF_BLEND | MF_TEXTURE_REPEAT,
                        (float[4]) {
                                0.99997f, 1.0f, 1.0f, 1.0f
                        });
                        brightWhite.commitWith(MF_NO_DEPTH_TEST | MF_BLEND | MF_TEXTURE_REPEAT,
                        (float[4]) {
                                1.0f, 1.0f, 1.0f, 1.0f
                        });

                        shader_loader.load_shader("main.vs", "main.fs", [=](ShaderProgram&& input) {
                                mainShader = std::move(input);
                        });

                }

                Material classyWhite;
                Material brightWhite;
                ShaderProgram mainShader;
                ShaderLoader shader_loader;
                GLuint position_attr;
                Framebuffer framebuffers[3];
        } resources;

        tasks.run();

        double const phase = 6.30 * time_micros / 1e6 / 11.0;
        float sincos[2] = {
                static_cast<float>(0.49 * sin(phase)),
                static_cast<float>(0.49 * cos(phase)),
        };
        float const argb[4] = {
                0.0f, 0.31f + 0.39f * sincos[0], 0.27f + 0.39f * sincos[1], 0.29f
        };
        glClearColor (argb[1], argb[2], argb[3], argb[0]);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode (GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(-1.0, 1.0, 1.0, -1.0, +1.0, -1.0);

        DisplayFrameImpl frame;
        Framebuffer* framebuffers[] = {
                &resources.framebuffers[0],
                &resources.framebuffers[1],
                &resources.framebuffers[2],
        };
        razors(&frame, time_micros / 1e3, resources.classyWhite, framebuffers, 1.0,
               0.0, 0.0, 1, resources.classyWhite);

        Mesh mesh;
        mesh.defQuad2d(0, -0.5, -0.7, 1.0, 1.4, 0.0, 0.0, 1.0, 1.0);
        mesh.bind(resources.mainShader);
        mesh.draw();

        glMatrixMode (GL_PROJECTION);
        glPopMatrix();
}

int main (int argc, char** argv)
{
        (void) argc;
        (void) argv;

        runtime_init();

        return 0;
}
