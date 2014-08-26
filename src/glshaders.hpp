#pragma once

#include <string>

class FragmentShaderResource;
class ShaderProgramResource;
class VertexShaderResource;

void compile(VertexShaderResource const& shader,
             std::string const& source);
void compile(FragmentShaderResource const& shader,
             std::string const& source);
void validate(ShaderProgramResource const& program);
