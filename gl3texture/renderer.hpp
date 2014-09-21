#include <functional>
#include <string>
#include <vector>

// forward declarations

class FrameSeries;
class BufferResource;

// value types

struct VertexShader {
        std::string source;
};

struct FragmentShader {
        std::string source;
};

struct Program {
        VertexShader vertexShader;
        FragmentShader fragmentShader;
};

struct TextureDef {
        int width = 0;
        int height = 0;
        void (*pixelFiller)(uint32_t* pixels, int width, int height) = nullptr;
};

struct ProgramInputs {
        struct AttribArrayInput {
                std::string name;
        };
        struct TextureInput {
                std::string name;
                TextureDef content;
        };

        std::vector<AttribArrayInput> attribs;

        std::vector<TextureInput> textures;
};

struct GeometryDef {
        std::vector<char> data;
        size_t arrayCount = 0;

        // @returns indices count
        size_t (*definer)(BufferResource const& elementBuffer,
                          BufferResource const arrays[],
                          char* data) = nullptr;
};

using FrameSeriesResource =
        std::unique_ptr<FrameSeries, std::function<void(FrameSeries*)>>;
FrameSeriesResource makeFrameSeries();

void drawOne(FrameSeries& output,
             Program programDef,
             ProgramInputs inputs,
             GeometryDef geometryDef);
