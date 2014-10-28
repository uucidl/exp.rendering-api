#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// forward declarations

class FrameSeries;
class BufferResource;

// value types

struct FragmentOperationsDef {
        enum {
                DEPTH_TEST = 1 << 0,
                BLEND_PREMULTIPLIED_ALPHA = 1 << 1,
                CLEAR = 1 << 2,
        };

        int flags;
        std::array<float,4> clearRGBA;
};

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

using TextureDefFn = void (*)(uint32_t*, int width, int height, int depth,
                              void const* data);

struct TextureDef {
        std::vector<char> data;
        int width;
        int height;
        int depth;
        TextureDefFn pixelFiller;
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
                /// index of the last row
                int last_row;
        };

        struct IntInput {
                std::string name;
                std::vector<int32_t> values;
        };

        std::vector<AttribArrayInput> attribs;
        std::vector<TextureInput> textures;
        std::vector<FloatInput> floatValues;
        std::vector<IntInput> intValues;
};

struct GeometryDef {
        std::vector<char> data;
        size_t arrayCount = 0;

        // @returns indices count
        size_t (*definer)(BufferResource const& elementBuffer,
                          BufferResource const arrays[],
                          void const* data) = nullptr;
};

struct RenderObjectDef {
        ProgramInputs inputs;
        GeometryDef geometry;
};

using FrameSeriesResource =
        std::unique_ptr<FrameSeries, std::function<void(FrameSeries*)>>;
FrameSeriesResource makeFrameSeries();

void beginFrame(FrameSeries& output);

void drawOne(FrameSeries& output,
             FragmentOperationsDef fragmentOperationsDef,
             ProgramDef programDef,
             ProgramInputs inputs,
             GeometryDef geometryDef);

void drawMany(FrameSeries& output,
              FragmentOperationsDef fragmentOperationsDef,
              ProgramDef program,
              std::vector<RenderObjectDef> objects);

TextureDef drawManyIntoTexture(FrameSeries& output,
                               TextureDef spec,
                               FragmentOperationsDef fragmentOperationsDef,
                               ProgramDef program,
                               std::vector<RenderObjectDef> objects);
