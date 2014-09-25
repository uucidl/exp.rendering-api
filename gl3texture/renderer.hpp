#pragma once

#include <functional>
#include <string>
#include <vector>

// forward declarations

class FrameSeries;
class BufferResource;

// value types

struct VertexShaderDef {
        std::string source;
};

struct FragmentShaderDef {
        std::string source;
};

struct ProgramDef {
        VertexShaderDef vertexShader;
        FragmentShaderDef fragmentShader;
};

struct TextureDef {
        int width = 0;
        int height = 0;
        void (*pixelFiller)(uint32_t* pixels, int width, int height) = nullptr;
};

struct ProgramInputs {
        struct AttribArrayInput {
                std::string name;
                int componentCount;
        };
        struct TextureInput {
                std::string name;
                TextureDef content;
        };
        struct FloatInput {
                std::string name;
                std::vector<float> values;
        };

        std::vector<AttribArrayInput> attribs;
        std::vector<TextureInput> textures;
        std::vector<FloatInput> floatValues;
};

struct GeometryDef {
        std::vector<char> data;
        size_t arrayCount = 0;

        // @returns indices count
        size_t (*definer)(BufferResource const& elementBuffer,
                          BufferResource const arrays[],
                          char const* data) = nullptr;
};

using FrameSeriesResource =
        std::unique_ptr<FrameSeries, std::function<void(FrameSeries*)>>;
FrameSeriesResource makeFrameSeries();

void drawOne(FrameSeries& output,
             ProgramDef programDef,
             ProgramInputs inputs,
             GeometryDef geometryDef);
