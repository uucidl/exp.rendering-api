#include "glresource_types.hpp"

#include <GL/glew.h>

#include <sstream>
#include <vector>

void validate(ShaderProgramResource const& program)
{
        glValidateProgram (program.id);
        GLint status;
        glGetProgramiv (program.id, GL_VALIDATE_STATUS, &status);
        if (status == GL_FALSE) {
                GLint length;
                glGetProgramiv (program.id, GL_INFO_LOG_LENGTH, &length);

                std::vector<char> pinfo;
                pinfo.reserve(length + 1);

                glGetProgramInfoLog (program.id, length, &length, &pinfo.front());

                printf ("ERROR: validating program [%s]\n", &pinfo.front());
        }
}

static std::vector<std::string const> splitLines(std::string const source)
{
        std::vector<std::string const> result;
        std::stringstream ss {source};
        std::string item;
        while (getline(ss, item, '\n')) {
                result.emplace_back(item + "\n");
        }

        return result;
}

static std::vector<char const*> cstrsOf(std::vector<std::string const>& lines)
{
        std::vector<char const*> result;

        for (auto const& str : lines) {
                result.emplace_back(str.c_str());
        }

        return result;
}

static void innerCompile(GLuint id,
                         std::string const& source)
{
        auto lines = splitLines(source);
        auto cstrs = cstrsOf(lines);

        glShaderSource(id, cstrs.size(), &cstrs.front(), NULL);
        glCompileShader(id);

        GLint status;
        glGetShaderiv (id, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
                GLint length;
                glGetShaderiv (id, GL_INFO_LOG_LENGTH, &length);

                std::vector<char> sinfo;
                sinfo.reserve(length + 1);
                glGetShaderInfoLog(id, length, &length, &sinfo.front());

                printf ("ERROR compiling shader [%s] with source [\n", &sinfo.front());
                printf ("%s", source.c_str());
                printf ("]\n");
        }
}

void compile(VertexShaderResource const& shader,
             std::string const& source)
{
        return innerCompile(shader.id, source);
}

void compile(FragmentShaderResource const& shader,
             std::string const& source)
{
        return innerCompile(shader.id, source);
}

void link(ShaderProgramResource const& program,
          VertexShaderResource const& vertexShader,
          FragmentShaderResource const& fragmentShader)
{
        glAttachShader(program.id, vertexShader.id);
        glAttachShader(program.id, fragmentShader.id);
        glLinkProgram(program.id);

        auto id = program.id;
        GLint status;
        glGetProgramiv (id, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
                GLint length;
                glGetShaderiv (id, GL_INFO_LOG_LENGTH, &length);

                std::vector<char> sinfo;
                sinfo.reserve(length + 1);
                glGetShaderInfoLog(id, length, &length, &sinfo.front());

                printf ("ERROR linking shader [%s]\n", &sinfo.front());
        }
}
