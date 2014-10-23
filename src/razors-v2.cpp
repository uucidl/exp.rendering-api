#include "estd.hpp"
#include "inlineshaders.hpp"
#include "razorsV2.hpp"

#include "../gl3companion/glresource_types.hpp"
#include "../gl3texture/quad.hpp"
#include "../gl3texture/renderer.hpp"

#include <cmath>

static const double TAU =
        6.28318530717958647692528676655900576839433879875021;

class RazorsV2
{};

RazorsV2Resource makeRazorsV2()
{
        return estd::make_unique<RazorsV2>();
}

struct FramebufferDef {
        std::string id;
        int width;
        int height;
};

struct Rect {
        float x;
        float y;
        float width;
        float height;
};

struct QuadDefinerParams {
        Rect coords;
        Rect uvcoords;
};

static
size_t quadDefiner(BufferResource const& elementBuffer,
                   BufferResource const arrays[],
                   void const* data)
{
        auto params = static_cast<QuadDefinerParams const*> (data);
        auto& coords = params->coords;
        auto& uvcoords = params->uvcoords;

        size_t indicesCount = define2dQuadIndices(elementBuffer);
        define2dQuadBuffer(arrays[0],
                           coords.x, coords.y,
                           coords.width, coords.height);
        define2dQuadBuffer(arrays[1],
                           uvcoords.x, uvcoords.y,
                           uvcoords.width, uvcoords.height);

        return indicesCount;
}


void draw(RazorsV2& self, double ms)
{
        struct Viewport {
                int width;
                int height;
        };

        auto viewport = Viewport { 100, 100 };

        auto frame1 = FramebufferDef {
                .id = "frame1",
                .width = viewport.width,
                .height = viewport.height,
        };

        auto frame2 = FramebufferDef {
                .id = "frame2",
                .width = viewport.width,
                .height = viewport.height,
        };

        auto framebufferTexture = [](FramebufferDef& framebuffer) {
                auto texture = TextureDef {};

                texture.data.reserve(sizeof framebuffer);
                *(new (&texture.data.front()) FramebufferDef) = framebuffer;
                texture.width = framebuffer.width;
                texture.height = framebuffer.height;
                texture.pixelFiller = NULL;

                return texture;
        };

        auto transparentWhite = [](float alpha) -> std::vector<float> {
                return { alpha*1.0f, alpha*1.0f, alpha*1.0f, alpha };
        };

        auto scaleTransform = [](float scale) -> std::vector<float> {
                return std::vector<float> {
                        scale, 0.01f, 0.0f, 0.0f,
                        -0.01f, scale, 0.0f, 0.0f,
                        0.0f, 0.0f, scale, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f,
                };
        };

        auto quad = [](Rect coords, Rect uvcoords) -> GeometryDef {
                auto geometry = GeometryDef {};

                geometry.arrayCount = 2;
                geometry.data.resize(sizeof(QuadDefinerParams));

                auto params = new (&geometry.data.front()) QuadDefinerParams;
                *params = {
                        .coords = coords,
                        .uvcoords = uvcoords,
                };

                geometry.definer = quadDefiner;

                return geometry;
        };

        auto projectorQuad = [quad]() -> GeometryDef {
                return quad(Rect { -1.f, -1.f, 2.0f, 2.0f }, Rect {0.f, 0.f, 1.f, 1.f });
        };

        auto seedTexture = []() -> TextureDef {
                return TextureDef {};
        };

        auto projector = RenderObjectDef {
                .inputs = ProgramInputs {
                        {
                                { "position", 2 },
                                { "texcoord", 2 },
                        },
                        {
                                {
                                        .name = "tex",
                                        .content = framebufferTexture(frame2),
                                }
                        },
                        {
                                ProgramInputs::FloatInput { .name = "iResolution", .values = { static_cast<float> (viewport.width), static_cast<float> (viewport.height) } },
                                ProgramInputs::FloatInput { .name = "g_color", .values = transparentWhite(0.9998f)},
                                ProgramInputs::FloatInput { .name = "transform", .values = scaleTransform(0.990f + 0.010f * sin(TAU * ms / 5000.0)), .last_row = 3 }
                        }
                },
                .geometry = projectorQuad()
        };

        auto seed = RenderObjectDef {
                .inputs = ProgramInputs {
                        {
                                { "position", 2 }, { "texcoord", 2 }
                        },
                        {
                                { .name = "tex", seedTexture() },
                        },
                        {},
                },
                .geometry = projectorQuad(),
        };


        auto vertexShader = [](std::string content) -> VertexShaderDef {
                return VertexShaderDef { .source = content };
        };

        auto fragmentShader = [](std::string content) -> FragmentShaderDef {
                return FragmentShaderDef { .source = content };
        };

        auto appendTo = [](FramebufferDef const& framebuffer,
        ProgramDef const& program, RenderObjectDef const& object) {
                // do nothing
        };

        appendTo(frame1,
        ProgramDef {
                .vertexShader = vertexShader(defaultVS),
                .fragmentShader = fragmentShader(projectorFS),
        }, {
                projector,
        }
                );
        appendTo(frame1,
        ProgramDef {
                .vertexShader = vertexShader(seedVS),
                .fragmentShader = fragmentShader(seedFS),
        },
        { seed });

        static auto output = makeFrameSeries();

        drawMany(*output, ProgramDef {
                .vertexShader = vertexShader(seedVS),
                .fragmentShader = fragmentShader(seedFS),
        },
        { seed });
}
