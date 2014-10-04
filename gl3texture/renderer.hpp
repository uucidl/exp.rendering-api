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

using TextureDefFn = void (*)(uint32_t*,int,int, void const* data);

struct TextureDef {
        std::vector<char> data;
        int width = 0;
        int height = 0;
        TextureDefFn pixelFiller = nullptr;
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
                          void const* data) = nullptr;
};

using FrameSeriesResource =
        std::unique_ptr<FrameSeries, std::function<void(FrameSeries*)>>;
FrameSeriesResource makeFrameSeries();

void drawOne(FrameSeries& output,
             ProgramDef programDef,
             ProgramInputs inputs,
             GeometryDef geometryDef);
