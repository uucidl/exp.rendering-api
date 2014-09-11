#pragma once

#include "main_types.h"

#include <GL/glew.h>

#include <future>
#include <memory>
#include <string>
#include <vector>

class FileSystem;
class DisplayThreadTasks;

class ShaderProgram
{
public:
        static ShaderProgram create(std::string const& vertex_shader_code,
                                    std::string const& fragment_shader_code);

        void validate() const;
        GLuint ref() const;

        ShaderProgram();
        ~ShaderProgram();
        ShaderProgram(ShaderProgram&& other);
        ShaderProgram& operator=(ShaderProgram&& other);
        ShaderProgram(ShaderProgram& other) = delete;
        ShaderProgram& operator=(ShaderProgram& other) = delete;

        class Impl;
        std::unique_ptr<Impl> impl;
};

class ShaderLoader
{
public:
        ShaderLoader(DisplayThreadTasks& display_tasks, FileSystem& fs) :
                fileLoader(makeFileLoader(fs, display_tasks))
        {}

        void load_shader(std::string vs_path,
                         std::string fs_path,
                         std::function<void(ShaderProgram&&)> bind_shader);

private:
        FileLoaderResource fileLoader;
};

class WithShaderProgramScope
{
public:
        WithShaderProgramScope(ShaderProgram const& program)
        {
                glUseProgram(program.ref());
        }

        ~WithShaderProgramScope()
        {
                glUseProgram(0);
        }
private:
        WithShaderProgramScope(WithShaderProgramScope&) = delete;
        WithShaderProgramScope(WithShaderProgramScope&&) = delete;
        WithShaderProgramScope& operator=(WithShaderProgramScope&) = delete;
        WithShaderProgramScope& operator=(WithShaderProgramScope&&) = delete;
};
